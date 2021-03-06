/* PIPS
   Authors: Cosmin Petra
   See license and copyright information in the documentation */

#include <memory>
#include "DistributedFactory.hpp"
#include "DistributedProblem.hpp"
#include "DistributedTreeCallbacks.h"
#include "DistributedInputTree.h"
#include "DistributedSymmetricMatrix.h"
#include "DistributedMatrix.h"
#include "DistributedVector.h"
#include "DistributedVariables.h"
#include "DistributedResiduals.hpp"
#include "DistributedRootLinearSystem.h"
#include "DistributedLeafLinearSystem.h"
#include "PIPSIPMppOptions.h"
#include "mpi.h"
#include "sLinsysRootAug.h"
#include "sLinsysRootAugHierInner.h"
#include "sLinsysRootBordered.h"

#include "IpoptRegularization.hpp"
#include "FriedlanderOrbanRegularization.hpp"

#ifdef WITH_MA57

#include "Ma57Solver.h"

#endif

#ifdef WITH_MA27

#include "Ma27Solver.h"

#endif

#ifdef WITH_PARDISO

#include "sLinsysLeafSchurSlv.h"
#include "PardisoProjectSolver.h"
#include "../LinearSolvers/PardisoSolver/PardisoSchurSolver/PardisoProjectSchurSolver.h"

#endif

#ifdef WITH_MKL_PARDISO
#include "sLinsysLeafSchurSlv.h"
#include "PardisoMKLSolver.h"
#include "../LinearSolvers/PardisoSolver/PardisoSchurSolver/PardisoMKLSchurSolver.h"
#endif

#ifdef WITH_MUMPS
#include "sLinsysLeafMumps.h"
#include "../LinearSolvers/MumpsSolver/MumpsSolverLeaf.h"
#endif


DistributedFactory::DistributedFactory(DistributedInputTree* inputTree, MPI_Comm comm) : tree(
   new DistributedTreeCallbacks(inputTree)) {
   tree->assignProcesses(comm);
   tree->computeGlobalSizes();
   // now the sizes of the problem are available, set them for the parent class
   tree->getGlobalSizes(nx, my, mz);
}

std::unique_ptr<DoubleLinearSolver> DistributedFactory::make_leaf_solver(const AbstractMatrix* kkt_) {
   const auto& kkt = dynamic_cast<const SparseSymmetricMatrix&>(*kkt_);

   const SolverType leaf_solver = pipsipmpp_options::get_solver_leaf();

   if (!pipsipmpp_options::get_bool_parameter("SC_COMPUTE_BLOCKWISE")) {
      if (leaf_solver == SolverType::SOLVER_MUMPS) {
#ifdef WITH_MUMPS
         return std::make_unique<MumpsSolverLeaf>(kkt);
#endif
      } else if (leaf_solver == SolverType::SOLVER_PARDISO) {
#ifdef WITH_PARDISO
         return std::make_unique<PardisoProjectSchurSolver>(kkt);
#endif
      } else if (leaf_solver == SolverType::SOLVER_MKL_PARDISO) {
#ifdef WITH_MKL_PARDISO
         return std::make_unique<PardisoMKLSchurSolver>(kkt);
#endif
      }
      PIPS_MPIabortIf(true, "No leaf solver for Schur Complement computation could be found - should not happen..");
   } else {
      if (leaf_solver == SolverType::SOLVER_PARDISO) {
#ifdef WITH_PARDISO
         return std::make_unique<PardisoProjectSolver>(kkt);
#endif
      } else if (leaf_solver == SolverType::SOLVER_MKL_PARDISO) {
#ifdef WITH_MKL_PARDISO
         return std::make_unique<PardisoMKLSolver>(kkt);
#endif
      } else if (leaf_solver == SolverType::SOLVER_MA57) {
#ifdef WITH_MA57
         return std::make_unique<Ma57Solver>(kkt);
#endif
      } else if (leaf_solver == SolverType::SOLVER_MA27) {
#ifdef WITH_MA27
         return std::make_unique<Ma27Solver>(kkt);
#endif
      } else if (leaf_solver == SolverType::SOLVER_MUMPS) {
#ifdef WITH_MUMPS
         return std::make_unique<MumpsSolverLeaf>(kkt);
#endif
      }
      PIPS_MPIabortIf(true,
         "No leaf solver for Blockwise Schur Complement computation could be found - should not happen..");
   }
   return nullptr;
}

std::unique_ptr<DistributedLeafLinearSystem>
DistributedFactory::make_linear_system_leaf(DistributedProblem* problem, std::shared_ptr<Vector<double>> primal_diagonal,
   std::shared_ptr<Vector<double>> dq,
   std::shared_ptr<Vector<double>> nomegaInv,
   std::shared_ptr<Vector<double>> primal_regularization, std::shared_ptr<Vector<double>> dual_equality_regularization,
   std::shared_ptr<Vector<double>> dual_inequality_regularization,
   std::shared_ptr<Vector<double>> rhs) const {
   assert(problem);
   assert(primal_diagonal);
   static bool printed = false;
   const SolverType leaf_solver = pipsipmpp_options::get_solver_leaf();

   if (!pipsipmpp_options::get_bool_parameter("SC_COMPUTE_BLOCKWISE")) {
      if (PIPS_MPIgetRank() == 0 && !printed)
         std::cout << "Using " << leaf_solver << " for leaf schur complement computation - sFactory\n";
      printed = true;

      if (leaf_solver == SolverType::SOLVER_MUMPS) {
#ifdef WITH_MUMPS
         return std::make_unique<sLinsysLeafMumps>(*this, problem, primal_diagonal, dq, nomegaInv, primal_regularization, dual_equality_regularization, dual_inequality_regularization, rhs);
#endif
      } else if (leaf_solver == SolverType::SOLVER_PARDISO || leaf_solver == SolverType::SOLVER_MKL_PARDISO) {
#if defined(WITH_PARDISO) or defined(WITH_MKL_PARDISO)
         return std::make_unique<sLinsysLeafSchurSlv>(*this, problem, std::move(primal_diagonal), std::move(dq), std::move(nomegaInv),
            std::move(primal_regularization),
            std::move(dual_equality_regularization),
            std::move(dual_inequality_regularization), std::move(rhs));
#endif
      } else {
         if (PIPS_MPIgetRank() == 0) {
            std::cout << "WARNING: did not specify SC_COMPUTE_BLOCKWISE but " << leaf_solver
                      << " which can only compute the Schur Complement blockwise\n";
            std::cout << "WARNING: checking for PARDISO or MUMPS instead...\n";
         }

         SolverType solver = SolverType::SOLVER_NONE;
         if (pipsipmpp_options::is_solver_available(SolverType::SOLVER_PARDISO))
            solver = SolverType::SOLVER_PARDISO;
         else if (pipsipmpp_options::is_solver_available(SolverType::SOLVER_MKL_PARDISO))
            solver = SolverType::SOLVER_MKL_PARDISO;
         else if (pipsipmpp_options::is_solver_available(SolverType::SOLVER_MUMPS))
            solver = SolverType::SOLVER_MUMPS;

         if (solver != SolverType::SOLVER_NONE) {
            if (PIPS_MPIgetRank() == 0)
               std::cout << " Found solver " << solver << " - using that for leaf computations\n";
            pipsipmpp_options::set_int_parameter("LINEAR_LEAF_SOLVER", solver);
#if defined(WITH_PARDISO) or defined(WITH_MKL_PARDISO)
            return std::make_unique<sLinsysLeafSchurSlv>(*this, problem, std::move(primal_diagonal), std::move(dq),
               std::move(nomegaInv), std::move(primal_regularization),
               std::move(dual_equality_regularization),
               std::move(dual_inequality_regularization), std::move(rhs));
#endif
         }

         PIPS_MPIabortIf(true, "Error: Could not find suitable solver - please specify SC_COMPUTE_BLOCKWISE");
         return nullptr;
      }
   } else {
      if (PIPS_MPIgetRank() == 0 && !printed)
         std::cout << "Using " << leaf_solver
                   << " for blockwise Schur Complement computation - deactivating distributed preconditioner - sFactory\n";
      pipsipmpp_options::set_bool_parameter("PRECONDITION_DISTRIBUTED", false);
      printed = true;

      if (leaf_solver == SolverType::SOLVER_MUMPS) {
#ifdef WITH_MUMPS
         return std::make_unique<sLinsysLeafMumps>(this, problem, primal_diagonal, dq, nomegaInv, primal_regularization, dual_equality_regularization, dual_inequality_regularization, rhs);
#endif
      } else
         return std::make_unique<DistributedLeafLinearSystem>(*this, problem, std::move(primal_diagonal), std::move(dq),
            std::move(nomegaInv), std::move(primal_regularization),
            std::move(dual_equality_regularization),
            std::move(dual_inequality_regularization), std::move(rhs));
   }
   return nullptr;
}

std::unique_ptr<Problem> DistributedFactory::make_problem() const {
#ifdef TIMING
   double t2 = MPI_Wtime();
#endif

   std::shared_ptr<GeneralMatrix> A(tree->createA());
   std::shared_ptr<DistributedVector<double>> b(tree->createb());

   std::shared_ptr<GeneralMatrix> C(tree->createC());
   std::shared_ptr<DistributedVector<double>> clow(tree->createclow());
   std::shared_ptr<DistributedVector<double>> iclow(tree->createiclow());
   std::shared_ptr<DistributedVector<double>> cupp(tree->createcupp());
   std::shared_ptr<DistributedVector<double>> icupp(tree->createicupp());

   std::shared_ptr<SymmetricMatrix> Q(tree->createQ());
   std::shared_ptr<DistributedVector<double>> c(tree->createc());

   std::shared_ptr<DistributedVector<double>> xlow(tree->createxlow());
   std::shared_ptr<DistributedVector<double>> ixlow(tree->createixlow());
   std::shared_ptr<DistributedVector<double>> xupp(tree->createxupp());
   std::shared_ptr<DistributedVector<double>> ixupp(tree->createixupp());

#ifdef TIMING
   MPI_Barrier( tree->getCommWrkrs() );
   t2 = MPI_Wtime() - t2;
   if ( PIPS_MPIgetRank(tree->getCommWorkers() == 0) )
         std::cout << "IO second part took " << t2 << " sec\n";
#endif

   return std::make_unique<DistributedProblem>(tree.get(), std::move(c), std::move(Q), std::move(xlow), std::move(ixlow), std::move(xupp),
      std::move(ixupp), std::move(A), std::move(b), std::move(C), std::move(clow), std::move(iclow), std::move(cupp),
      std::move(icupp));
}

std::unique_ptr<Variables> DistributedFactory::make_variables(const Problem& problem) const {

   std::unique_ptr<Vector<double>> x(make_primal_vector());
   std::unique_ptr<Vector<double>> s(make_inequalities_dual_vector());
   std::unique_ptr<Vector<double>> y(make_equalities_dual_vector());
   std::unique_ptr<Vector<double>> z(make_inequalities_dual_vector());
   std::unique_ptr<Vector<double>> v(make_primal_vector());
   std::unique_ptr<Vector<double>> gamma(make_primal_vector());
   std::unique_ptr<Vector<double>> w(make_primal_vector());
   std::unique_ptr<Vector<double>> phi(make_primal_vector());
   std::unique_ptr<Vector<double>> t(make_inequalities_dual_vector());
   std::unique_ptr<Vector<double>> lambda(make_inequalities_dual_vector());
   std::unique_ptr<Vector<double>> u(make_inequalities_dual_vector());
   std::unique_ptr<Vector<double>> pi(make_inequalities_dual_vector());

   assert(problem.primal_lower_bound_indicators && problem.primal_upper_bound_indicators && problem.inequality_lower_bound_indicators && problem.inequality_upper_bound_indicators);
   return std::make_unique<DistributedVariables>(tree.get(), std::move(x), std::move(s), std::move(y), std::move(z),
      std::move(v), std::move(gamma), std::move(w), std::move(phi), std::move(t), std::move(lambda), std::move(u),
      std::move(pi), problem.primal_lower_bound_indicators, problem.primal_lower_bound_indicators->number_nonzeros(), problem.primal_upper_bound_indicators, problem.primal_upper_bound_indicators->number_nonzeros(),
      problem.inequality_lower_bound_indicators, problem.inequality_lower_bound_indicators->number_nonzeros(), problem.inequality_upper_bound_indicators, problem.inequality_upper_bound_indicators->number_nonzeros());
}

std::unique_ptr<Residuals> DistributedFactory::make_residuals(const Problem& problem) const {

   std::unique_ptr<Vector<double>> lagrangian_gradient{make_primal_vector()};

   std::unique_ptr<Vector<double>> rA{make_equalities_dual_vector()};
   std::unique_ptr<Vector<double>> rC{make_inequalities_dual_vector()};

   std::unique_ptr<Vector<double>> rz{make_inequalities_dual_vector()};

   const bool mclow_empty = problem.number_inequality_lower_bounds <= 0;
   std::unique_ptr<Vector<double>> rt{tree->new_inequalities_dual_vector<double>(mclow_empty)};
   std::unique_ptr<Vector<double>> rlambda{tree->new_inequalities_dual_vector<double>(mclow_empty)};

   const bool mcupp_empty = problem.number_inequality_upper_bounds <= 0;
   std::unique_ptr<Vector<double>> ru{tree->new_inequalities_dual_vector<double>(mcupp_empty)};
   std::unique_ptr<Vector<double>> rpi{tree->new_inequalities_dual_vector<double>(mcupp_empty)};

   const bool nxlow_empty = problem.number_primal_lower_bounds <= 0;
   std::unique_ptr<Vector<double>> rv{tree->new_primal_vector<double>(nxlow_empty)};
   std::unique_ptr<Vector<double>> rgamma{tree->new_primal_vector<double>(nxlow_empty)};

   const bool nxupp_empty = problem.number_primal_upper_bounds <= 0;
   std::unique_ptr<Vector<double>> rw{tree->new_primal_vector<double>(nxupp_empty)};
   std::unique_ptr<Vector<double>> rphi{tree->new_primal_vector<double>(nxupp_empty)};

   assert(problem.primal_lower_bound_indicators && problem.primal_upper_bound_indicators && problem.inequality_lower_bound_indicators && problem.inequality_upper_bound_indicators);
   return std::make_unique<DistributedResiduals>(std::move(lagrangian_gradient), std::move(rA), std::move(rC), std::move(rz),
      std::move(rt), std::move(rlambda), std::move(ru), std::move(rpi), std::move(rv), std::move(rgamma), std::move(rw),
      std::move(rphi), problem.primal_lower_bound_indicators, problem.primal_upper_bound_indicators, problem.inequality_lower_bound_indicators, problem.inequality_upper_bound_indicators);
}

std::unique_ptr<AbstractLinearSystem> DistributedFactory::make_linear_system(Problem& problem_in) const {
   auto* problem = dynamic_cast<DistributedProblem*>(&problem_in);
   if (pipsipmpp_options::get_bool_parameter("HIERARCHICAL"))
      return make_root_hierarchical_linear_system(problem);
   else
      return make_linear_system_root(problem);
}

std::unique_ptr<Vector<int>> DistributedFactory::make_primal_integral_vector() const {
   return tree->new_primal_vector<int>();
}

std::unique_ptr<Vector<int>> DistributedFactory::make_equalities_dual_integral_vector() const {
   return tree->new_equalities_dual_vector<int>();
}

std::unique_ptr<Vector<int>> DistributedFactory::make_inequalities_dual_integral_vector() const {
   return tree->new_inequalities_dual_vector<int>();
}

std::unique_ptr<RegularizationStrategy> DistributedFactory::make_regularization_strategy(unsigned int positive_eigenvalues, unsigned int negative_eigenvalues) const {
   if (pipsipmpp_options::get_int_parameter("REGULARIZATION_STRATEGY") == 0) {
      return std::make_unique<IpoptRegularization>(positive_eigenvalues, negative_eigenvalues);
   } else if (pipsipmpp_options::get_int_parameter("REGULARIZATION_STRATEGY") == 1) {
      return std::make_unique<FriedlanderOrbanRegularization>(positive_eigenvalues, negative_eigenvalues);
   } else {
      assert(false && "Regularization defined in option REGULARIZATION_STRATEGY strategy does not exist");
      return nullptr;
   }
}

std::unique_ptr<Vector<double>> DistributedFactory::make_primal_vector() const {
   return tree->new_primal_vector<double>();
}

std::unique_ptr<Vector<double>> DistributedFactory::make_equalities_dual_vector() const {
   return tree->new_equalities_dual_vector<double>();
}

std::unique_ptr<Vector<double>> DistributedFactory::make_inequalities_dual_vector() const {
   return tree->new_inequalities_dual_vector<double>();
}

std::unique_ptr<Vector<double>> DistributedFactory::make_right_hand_side() const {
   return tree->new_right_hand_side();
}

void DistributedFactory::iterate_started() {
   timer.start();
   tree->startMonitors();
}

void DistributedFactory::iterate_ended() {
   tree->stopMonitors();

   if (tree->balanceLoad()) {
      printf("Should not get here! OMG OMG OMG\n");
   }
   //logging and monitoring
   timer.stop();
   total_time += timer.end_time;

   if (PIPS_MPIgetRank() == 0) {
#ifdef TIMING
      extern double g_iterNumber;
      printf("TIME %g SOFAR %g ITER %d\n", iterTmMonitor.tmIterate, m_tmTotal, (int)g_iterNumber);
      //#elseif STOCH_TESTING
      //printf("ITERATION WALLTIME: iter=%g  Total=%g\n", iterTmMonitor.tmIterate, m_tmTotal);
#endif
   }
}

std::unique_ptr<DistributedRootLinearSystem> DistributedFactory::make_linear_system_root(DistributedProblem* problem) const {
   assert(problem);
   return std::make_unique<sLinsysRootAug>(*this, problem);
}

std::unique_ptr<DistributedRootLinearSystem>
DistributedFactory::make_linear_system_root(DistributedProblem* prob, std::shared_ptr<Vector<double>> primal_diagonal,
   std::shared_ptr<Vector<double>> dq,
   std::shared_ptr<Vector<double>> nomegaInv,
   std::shared_ptr<Vector<double>> primal_regularization, std::shared_ptr<Vector<double>> dual_equality_regularization,
   std::shared_ptr<Vector<double>> dual_inequality_regularization,
   std::shared_ptr<Vector<double>> rhs) const {
   if (prob->isHierarchyInnerLeaf())
      return std::make_unique<sLinsysRootAugHierInner>(*this, prob, std::move(primal_diagonal), std::move(dq), std::move(nomegaInv),
         std::move(primal_regularization),
         std::move(dual_equality_regularization),
         std::move(dual_inequality_regularization), std::move(rhs));
   else
      return std::make_unique<sLinsysRootAug>(*this, prob, std::move(primal_diagonal), std::move(dq), std::move(nomegaInv),
         std::move(primal_regularization),
         std::move(dual_equality_regularization),
         std::move(dual_inequality_regularization), rhs, false);
}

std::unique_ptr<DistributedRootLinearSystem>
DistributedFactory::make_root_hierarchical_linear_system(DistributedProblem* problem) const {
   return std::make_unique<sLinsysRootBordered>(*this, problem);
}

DistributedProblem* DistributedFactory::switchToHierarchicalData(DistributedProblem* problem) {
   hier_tree_swap = tree->clone();

   tree = tree->switchToHierarchicalTree(problem, std::move(tree));

   assert(tree->getChildren().size() == 1);
   assert(tree->isHierarchicalRoot());

   assert(problem->isHierarchyRoot());
   return problem;
}

void DistributedFactory::switchToOriginalTree() {
   assert(hier_tree_swap);

   std::unique_ptr<DistributedTree> tmp = std::move(tree);
   tree = std::move(hier_tree_swap);
   hier_tree_swap = std::move(tmp);
}
