#ifndef MONITOR_H
#define MONITOR_H

#include "DistributedFactory.h"
#include "Scaler.h"

class Problem;

class Variables;

class Residuals;

class Statistics {
public:
   Statistics() = default;
   virtual ~Statistics() = default;
   Statistics(Scaler* scaler = nullptr);
   Statistics(const DistributedFactory& factory, const Scaler* scaler = nullptr);

   void
   print(const Problem* problem, const Variables* variables, const Residuals* residuals, double dnorm, double alpha, double sigma, int i, double mu,
         int status_code, int level);

   void print(const Problem* problem, const Variables* variables, const Residuals* residuals, double dnorm, double alpha_primal, double alpha_dual,
         double sigma, int i, double mu, int status_code, int level) const;

protected:
   const Scaler* scaler{};

   MPI_Comm mpi_comm;
   int rank{-1};
   int global_rank{-1};
};

#endif