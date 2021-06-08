//
// Created by charlie on 26.04.21.
//

#ifndef MEHROTRAHEURISTIC_H
#define MEHROTRAHEURISTIC_H

#include <memory>
#include "Statistics.hpp"
#include "TerminationStatus.h"
#include "Observer.h"
#include "MehrotraStrategyType.h"
#include "FilterLineSearch.hpp"

class Problem;

class Variables;

class Residuals;

class AbstractLinearSystem;

class DistributedFactory;

class Scaler;

class MehrotraStrategy : public Observer {
public:
   MehrotraStrategy(DistributedFactory& factory, Problem& problem, double dnorm, const Scaler* scaler = nullptr);
   virtual void corrector_predictor(Problem& problem, Variables& iterate, Residuals& residuals, Variables& step, AbstractLinearSystem& linear_system,
         int iteration) = 0;
   virtual void fraction_to_boundary_rule(Variables& iterate, Variables& step) = 0;
   virtual double compute_centering_parameter(Variables& iterate, Variables& step) = 0;
   virtual void
   print_statistics(const Problem* problem, const Variables* iterate, const Residuals* residuals, double dnorm, double sigma, int i, double mu,
         int stop_code, int level) = 0;
   virtual void take_step(Variables& iterate, Variables& step) = 0;
   virtual void
   gondzio_correction_loop(Problem& problem, Variables& iterate, Residuals& residuals, Variables& step, AbstractLinearSystem& linear_system,
         int iteration, double sigma, double mu, bool& small_corr, bool& numerical_troubles) = 0;
   virtual void mehrotra_step_length(Variables& iterate, Variables& step) = 0;
   virtual bool is_poor_step(bool& pure_centering_step, bool precond_decreased) const = 0;
   double compute_probing_factor(Problem& problem, Variables& iterate, Residuals& residuals, Variables& step);
   virtual void do_probing(Problem& problem, Variables& iterate, Residuals& residuals, Variables& step) = 0;
   virtual void compute_probing_step(Variables& probing_step, const Variables& iterate, const Variables& step) const = 0;
   virtual std::pair<double, double> get_step_lengths() const = 0;
   void set_BiCGStab_tolerance(int iteration) const;
   void register_observer(AbstractLinearSystem* linear_system);
   virtual ~MehrotraStrategy() = default;

   double sigma;

protected:
   /** norm of problem data */
   double dnorm{0.};
   std::unique_ptr<Variables> corrector_step;
   /** storage for residual vectors */
   std::unique_ptr<Residuals> corrector_residuals;
   unsigned int n_linesearch_points;
   std::unique_ptr<Variables> temp_step;
   Statistics statistics;
   FilterLineSearch filter_line_search;

   /** termination parameters */
   double mutol{1.e-6};

   /* observer stuff for checking convergence of BiCGStab */
   bool bicgstab_skipped;
   bool bicgstab_converged;
   double bigcstab_norm_res_rel;
   int bicg_iterations;

   /** should a BiCGDependent Schedule for the number of Gondzio Correctors be used */
   const bool dynamic_corrector_schedule;
   /** should additional corrector steps for small complementarity pairs be applied */
   const bool additional_correctors_small_comp_pairs;
   /** should additional corrector steps for small complementarity pairs be applied */
   const int max_additional_correctors;
   /** first iteration at which to look for small corrector steps */
   const int first_iter_small_correctors;
   /** alpha must be lower equal to this value for the IPM to try and apply small corrector steps */
   const double max_alpha_small_correctors;
   int number_small_correctors;

   /** maximum number of Gondzio corrector steps */
   int maximum_correctors;
   /** actual number of Gondzio corrections needed */
   int number_gondzio_corrections;

   /** various parameters associated with Gondzio correction */
   double step_factor0, step_factor1, acceptance_tolerance, beta_min, beta_max;

   // controls whether setBiCGTol applies an dynamic schedule for the BiCGStab tolerance or just uses the user defined input (OUTER_BICG_TOL)
   bool dynamic_bicg_tol;

   /** exponent in Mehrotra's centering parameter, which is usually chosen to me (muaff/mu)^tsig, where muaff is the predicted
 *  complementarity gap obtained from an affine-scaling step, while mu is the current complementarity gap */
   double tsig;

   /** parameters associated with the step length heuristic */
   double gamma_f{0.99};
   double gamma_a{1. / (1. - 0.99)};
   /** number in (0,1) with which the step length is multiplied */
   double steplength_factor{0.99999999};

   bool pure_centering_step;
   bool numerical_troubles;
   bool precond_decreased;

   static void compute_predictor_step(Variables& iterate, Residuals& residuals, AbstractLinearSystem& linear_system, Variables& step);
   void compute_corrector_step(Variables& iterate, AbstractLinearSystem& linear_system, Variables& step, double sigma, double mu);
   void compute_gondzio_corrector(Variables& iterate, AbstractLinearSystem& linear_system, double rmin, double rmax, bool small_corr);
   std::pair<double, double>
   calculate_alpha_weight_candidate(Variables& iterate, Variables& predictor_step, Variables& corrector_step, double alpha_predictor);
   std::tuple<double, double, double, double>
   calculate_alpha_pd_weight_candidate(Variables& iterate, Variables& predictor_step, Variables& corrector_step, double alpha_primal,
         double alpha_dual);
   bool is_poor_step(bool& pure_centering_step, bool precond_decreased, double alpha_max) const;
   static double compute_step_factor_probing(double resids_norm_last, double resids_norm_probing, double mu_last, double mu_probing);
   static bool decrease_preconditioner_impact(AbstractLinearSystem* sys);
   void adjust_limit_gondzio_correctors();
   void check_numerical_troubles(Residuals* residuals, bool& numerical_troubles, bool& small_corr) const;
   void print_statistics(const Problem* problem, const Variables* iterate, const Residuals* residuals, double dnorm, double alpha_primal,
         double alpha_dual, double sigma, int i, double mu, int stop_code, int level);
   void notify_from_subject() override;
};

class PrimalMehrotraStrategy : public MehrotraStrategy {
public:
   PrimalMehrotraStrategy(DistributedFactory& factory, Problem& problem, double dnorm, const Scaler* scaler);
   void corrector_predictor(Problem& problem, Variables& iterate, Residuals& residuals, Variables& step, AbstractLinearSystem& linear_system,
         int iteration) override;
   void fraction_to_boundary_rule(Variables& iterate, Variables& step) override;
   double compute_centering_parameter(Variables& iterate, Variables& step) override;
   void take_step(Variables& iterate, Variables& step) override;
   void gondzio_correction_loop(Problem& problem, Variables& iterate, Residuals& residuals, Variables& step, AbstractLinearSystem& linear_system,
         int iteration, double sigma, double mu, bool& small_corr, bool& numerical_troubles) override;
   void mehrotra_step_length(Variables& iterate, Variables& step) override;
   bool is_poor_step(bool& pure_centering_step, bool precond_decreased) const override;
   void do_probing(Problem& problem, Variables& iterate, Residuals& residuals, Variables& step) override;
   void compute_probing_step(Variables& probing_step, const Variables& iterate, const Variables& step) const override;
   [[nodiscard]] std::pair<double, double> get_step_lengths() const override;
   void print_statistics(const Problem* problem, const Variables* iterate, const Residuals* residuals, double dnorm, double sigma, int i, double mu,
         int stop_code, int level) override;

protected:
   double primal_step_length;
   double alpha_candidate, weight_candidate;
};

class PrimalDualMehrotraStrategy : public MehrotraStrategy {
public:
   PrimalDualMehrotraStrategy(DistributedFactory& factory, Problem& problem, double dnorm, const Scaler* scaler);
   void corrector_predictor(Problem& problem, Variables& iterate, Residuals& residuals, Variables& step, AbstractLinearSystem& linear_system,
         int iteration) override;
   void fraction_to_boundary_rule(Variables& iterate, Variables& step) override;
   double compute_centering_parameter(Variables& iterate, Variables& step) override;
   void take_step(Variables& iterate, Variables& step) override;
   void gondzio_correction_loop(Problem& problem, Variables& iterate, Residuals& residuals, Variables& step, AbstractLinearSystem& linear_system,
         int iteration, double sigma, double mu, bool& small_corr, bool& numerical_troubles) override;
   void mehrotra_step_length(Variables& iterate, Variables& step) override;
   bool is_poor_step(bool& pure_centering_step, bool precond_decreased) const override;
   void do_probing(Problem& problem, Variables& iterate, Residuals& residuals, Variables& step) override;
   void compute_probing_step(Variables& probing_step, const Variables& iterate, const Variables& step) const override;
   std::pair<double, double> get_step_lengths() const override;
   void print_statistics(const Problem* problem, const Variables* iterate, const Residuals* residuals, double dnorm, double sigma, int i, double mu,
         int stop_code, int level) override;

protected:
   double primal_step_length;
   double dual_step_length;
   double alpha_primal_candidate, alpha_dual_candidate, weight_primal_candidate, weight_dual_candidate;
   double alpha_primal_enhanced, alpha_dual_enhanced;
};

class MehrotraFactory {
public:
   static std::unique_ptr<MehrotraStrategy>
   create(DistributedFactory& factory, Problem& problem, double dnorm, MehrotraStrategyType mehrotra_heuristic, const Scaler* scaler = nullptr);
};


#endif //MEHROTRAHEURISTIC_H
