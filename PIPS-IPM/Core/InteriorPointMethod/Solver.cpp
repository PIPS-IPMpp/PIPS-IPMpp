#include "Solver.hpp"
#include "Problem.h"
#include "Variables.h"
#include "Residuals.h"
#include "AbstractLinearSystem.h"
#include "AbstractOptions.h"
#include "DistributedFactory.h"
#include <cmath>

double g_iterNumber = 0.;
int print_level = 1000;

int gOuterBiCGFails = 0;
int gOuterBiCGIter = 0;
double gOuterBiCGIterAvg = 0.;
int gInnerBiCGFails = 0;
int gInnerBiCGIter = 0;

// gmu is needed by MA57!
double gmu;

Solver::Solver(DistributedFactory& factory, Problem& problem) : factory(factory), step(factory.make_variables(problem)) {
}

void Solver::solve_linear_system(Variables& iterate, Problem& problem, Residuals& residuals, Variables& step, AbstractLinearSystem& linear_system) {
   double problem_norm = std::sqrt(problem.datanorm());
   iterate.push_to_interior(problem_norm, problem_norm);

   residuals.evaluate(problem, iterate);
   residuals.set_complementarity_residual(iterate, 0.);

   linear_system.factorize(&problem, &iterate);
   linear_system.solve(&problem, &iterate, &residuals, &step);
   step.negate();

   // take the full affine scaling step
   iterate.saxpy(&step, 1.);
   double shift = 1e3 + 2 * iterate.violation();
   iterate.shift_bound_variables(shift, shift);
   return;
}

Solver::~Solver() {
   delete step;
}
