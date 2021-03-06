/*
 * PardisoIndefSolver.h
 *
 *  Created on: 21.03.2018
 *      Author: Daniel Rehfeldt
 */

#ifndef _PARDISOINDEFSOLVER_H_
#define _PARDISOINDEFSOLVER_H_

#include <memory>
#include "../../DoubleLinearSolver.h"
#include "../../../LinearAlgebra/Dense/DenseVector.hpp"
#include "../../../LinearAlgebra/Dense/DenseSymmetricMatrix.h"
#include "../../../LinearAlgebra/Sparse/SparseSymmetricMatrix.h"

/* root pardiso solver */
class PardisoIndefSolver : public DoubleLinearSolver {

protected:
   const DenseStorage* mStorage{};
   const SparseStorage* mStorageSparse{};

   constexpr static double precondDiagDomBound = 0.0001;
   constexpr static int pivotPerturbationExpDefault = 8; // was 8
   constexpr static int nIterativeRefinsDefault = 8;
   constexpr static bool highAccuracyDefault = true;
   constexpr static bool useSparseRhsDefault = true;
   constexpr static bool parallelForwardBackwardDefault = true;
   constexpr static bool factorizationTwoLevelDefault = true;

   /** buffer for solution of solve phase */
   std::vector<double> sol;
   /** buffer for non-zero right hand sides - PARDISO cannot porperly deal with 0 rhs when solving multiple rhss at once */
   std::vector<double> rhss_nonzero;
   /** maps a non-zero rhs to its original index */
   std::vector<int> map_rhs_nonzero_original;

   MPI_Comm mpi_comm{MPI_COMM_NULL};

   double* x{}; /* solution vector */

   int mtype{-2};

   int n; /* size of the matrix */

   int nrhs{1}; /* Number of right hand sides. */

   void* pt[64]{};  /* Internal solver memory pointer pt */

   /* Pardiso control parameters. */
   int iparm[64]{};

   int maxfct{1};
   int mnum{1};
   int phase{11};
   int msglvl{0};

   std::vector<int> ia;
   std::vector<int> ja{};
   std::vector<double> a{};
   int idum{-1};
   double ddum{-1.0};
   int pivotPerturbationExp; // 10^-exp
   int nIterativeRefins;
   bool highAccuracy{highAccuracyDefault};
   bool parallelForwardBackward{parallelForwardBackwardDefault};
   bool factorizationTwoLevel{factorizationTwoLevelDefault};
   bool useSparseRhs{useSparseRhsDefault};
   bool solve_in_parallel;


   virtual void
   pardisoCall(void* pt, int* maxfct, int* mnum, int* mtype, int* phase, int* n, double* M, int* krowM, int* jcolM, int* perm, int* nrhs, int* iparm,
         int* msglvl, double* rhs, double* sol, int* error) = 0;
   virtual void checkMatrix() = 0;
   virtual void getIparm(int* iparm) const = 0;
public:
   PardisoIndefSolver(const DenseSymmetricMatrix& matrix, bool solve_in_parallel, MPI_Comm mpi_comm);
   PardisoIndefSolver(const SparseSymmetricMatrix& matrix, bool solve_in_parallel, MPI_Comm mpi_comm);
   void diagonalChanged(int idiag, int extent) override;
   void matrixChanged() override;
   void matrixRebuild(const AbstractMatrix& matrixNew) override;

   using DoubleLinearSolver::solve;
   void solve(Vector<double>& vec) override;
   void solve(GeneralMatrix&) override { assert(0 && "not supported"); };
   void solve(int nrhss, double* rhss, int* /*colSparsity*/ ) override;

   void solveSynchronized(Vector<double>& vec) override;
   ~PardisoIndefSolver() override;

   [[nodiscard]] bool reports_inertia() const override;
   [[nodiscard]] std::tuple<unsigned int, unsigned int, unsigned int> get_inertia() const override;

private:
   void initPardiso();
   void factorizeFromSparse();
   void factorizeFromSparse(const SparseSymmetricMatrix& matrix_fortran);
   void factorizeFromDense();
   void factorize();

   bool iparmUnchanged();

};

#endif /* _PARDISOINDEFSOLVER_H_ */
