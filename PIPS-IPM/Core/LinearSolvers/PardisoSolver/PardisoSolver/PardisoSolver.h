/* PIPS-IPM
 * Authors: Cosmin G. Petra, Miles Lubin, Murat Mut
 * (C) 2012 Argonne National Laboratory, see documentation for copyright
 */
#ifndef PARDISOLINSYS_H
#define PARDISOLINSYS_H

#include "../../DoubleLinearSolver.h"
#include "../../../LinearAlgebra/Dense/DenseSymmetricMatrix.h"
#include "../../../LinearAlgebra/Sparse/SparseSymmetricMatrix.h"

#include <map>
#include <vector>

/** implements the linear solver class using the Pardiso solver
 */

class PardisoSolver : public DoubleLinearSolver {

public:
   virtual void firstCall() = 0;

   /** sets mStorage to refer to the argument sgm */
   explicit PardisoSolver(const SparseSymmetricMatrix& sgm);
   explicit PardisoSolver(const DenseSymmetricMatrix& m);

   void diagonalChanged(int idiag, int extent) override;
   void matrixChanged() override;

   void solve(Vector<double>& rhs) override;
   void solve(GeneralMatrix& rhs) override;
   void solve(int nrhss, double* rhss, int* colSparsity) override;
   void solve(GeneralMatrix& rhs, int* colSparsity);

   [[nodiscard]] bool reports_inertia() const override { return true; };
   [[nodiscard]] std::tuple<unsigned int, unsigned int, unsigned int> get_inertia() const override;

protected:
   virtual void setIparm(int* iparm) const = 0;
   virtual void
   pardisoCall(void* pt, const int* maxfct, const int* mnum, const int* mtype, const int* phase, int* n, double* M, int* krowM, int* jcolM, int* perm,
         int* nrhs, int* iparm, const int* msglvl, double* rhs, double* sol, int* error) = 0;

   void initSystem();
   [[nodiscard]] bool iparmUnchanged() const;

   ~PardisoSolver() override;

   const SparseSymmetricMatrix* Msys{};
   const DenseSymmetricMatrix* Mdsys{};
   bool first{true};
   void* pt[64];
   int iparm[64];
   int n;

   // max number of factorizations having same sparsity pattern to keep at the same time
   const int maxfct{1};
   // actual matrix (as in index from 1 to maxfct)
   const int mnum{1};
   // messaging level (0 = no output, 1 = statistical info to screen)
   const int msglvl = 0;

   int phase{0};
   const int mtype{-2};

   int nrhs{-1};
   int error{0};

   /** storage for the upper triangular (in row-major format) */
   int* krowM{};
   int* jcolM{};
   double* M{};

   int nnz{0};

   /** mapping from from the diagonals of the PIPS linear systems to
       the diagonal elements of the (1,1) block  in the augmented system */
   std::map<int, int> diagMap;

   /** temporary storage for the factorization process */
   double* nvec{}; //temporary first

   /** buffer for solution of solve phase */
   std::vector<double> sol;
   /** buffer for non-zero right hand sides - PARDISO cannot porperly deal with 0 rhs when solving multiple rhss at once */
   std::vector<double> rhss_nonzero;
   /** maps a non-zero rhs to its original index */
   std::vector<int> map_rhs_nonzero_original;
};

#endif
