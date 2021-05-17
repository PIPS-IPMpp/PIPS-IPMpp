#include <iostream>
#include <cmath>
#include "FilterStrategy.hpp"
#include "Filter.hpp"
#include "Residuals.h"
#include "Variables.h"

FilterStrategy::FilterStrategy(FilterStrategyParameters& filter_strategy_parameters, FilterParameters& filter_parameters) :
filter(filter_parameters), parameters(filter_strategy_parameters) {
}

FilterStrategy::FilterStrategy() : filter(), parameters({0.1, 0.999, 1e2, 1.25}) {
}

void FilterStrategy::initialize(Residuals& initial_residuals) {
   /* set the filter upper bound */
   double upper_bound = std::max(this->parameters.ubd, this->parameters.fact * initial_residuals.residual_norm());
   this->filter.upper_bound = upper_bound;
   return;
}

/* check acceptability of step(s) (filter & sufficient reduction)
 * precondition: feasible step
 * */
bool FilterStrategy::check_acceptance(Variables& current_iterate, Residuals& current_residuals, Variables& trial_iterate, Residuals& trial_residuals,
      double predicted_reduction, double step_length) {
   double current_mu = current_iterate.mu();
   const double current_feasibility = current_residuals.feasibility_measure(current_mu);
   const double current_optimality = current_residuals.optimality_measure(current_mu);
   double trial_mu = trial_iterate.mu();
   const double trial_feasibility = trial_residuals.feasibility_measure(trial_mu);
   const double trial_optimality = trial_residuals.optimality_measure(trial_mu);
   std::cout << "Current mu: " << current_mu << "\n";
   std::cout << "Trial mu: " << trial_mu << "\n";

   std::cout << "Filter strategy: feasibility " << trial_feasibility << " vs " << current_feasibility << "\n";
   std::cout << "Filter strategy: objective " << trial_optimality << " vs " << current_optimality << "\n";

   bool accept = false;
   /* check acceptance */
   bool filter_acceptable = this->filter.accept(trial_feasibility, trial_optimality);
   if (filter_acceptable) {
      std::cout << "Filter acceptable\n";
      // check acceptance wrt current x (h,f)
      filter_acceptable = this->filter.improves_current_iterate(current_feasibility, current_optimality, trial_feasibility, trial_optimality);
      if (filter_acceptable) {
         std::cout << "Current-iterate acceptable\n";
         double actual_reduction = this->filter.compute_actual_reduction(current_optimality, current_feasibility, trial_optimality);
         std::cout << "Filter predicted reduction: " << predicted_reduction << "\n";
         std::cout << "Filter actual reduction: " << actual_reduction << "\n";

         std::cout << "Switching condition: " << predicted_reduction << " >= " << this->parameters.Delta * std::pow(current_feasibility, 2) << " ?\n";
         std::cout << "Armijo condition: " << actual_reduction << " >= "
                   << this->parameters.Sigma * step_length * std::max(0., predicted_reduction - 1e-9) << " ?\n";

         /* switching condition: predicted reduction is not promising, accept */
         if (predicted_reduction < this->parameters.Delta * std::pow(current_feasibility, 2)) {
            this->filter.add(current_feasibility, current_optimality);
            accept = true;
         }
            /* Armijo sufficient decrease condition: predicted_reduction should be positive */
         else if (true || actual_reduction >= this->parameters.Sigma * step_length * std::max(0., predicted_reduction - 1e-9)) {
            accept = true;
         }
         else {
            std::cout << "Armijo condition not satisfied\n";
         }
      }
   }
   // TODO remove
   accept = true;
   return accept;
}
