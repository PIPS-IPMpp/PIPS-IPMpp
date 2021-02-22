/* PIPS
   Authors: Cosmin Petra
   See license and copyright information in the documentation */

#ifndef SAUGLINSYS
#define SAUGLINSYS

#include "sLinsysRoot.h"
#include "pipsport.h"


class sData;
/** 
 * ROOT (= NON-leaf) linear system in reduced augmented form
 */
class sLinsysRootAug : public sLinsysRoot {

 public:

  sLinsysRootAug(sFactory * factory_, sData * prob_);
  sLinsysRootAug(sFactory* factory,
		 sData* prob_,
		 OoqpVector* dd_, OoqpVector* dq_,
		 OoqpVector* nomegaInv_,
		 OoqpVector* regP, OoqpVector* regDy, OoqpVector* regDz,
		 OoqpVector* rhs_
       );
  virtual ~sLinsysRootAug();

  void finalizeKKT( sData* prob, Variables* vars) override;
  void finalizeKKTdist(sData* prob) override;

  void Lsolve(sData *prob, OoqpVector& x) override;
  void Dsolve(sData *prob, OoqpVector& x) override;
  void Ltsolve(sData *prob, OoqpVector& x) override;

 protected:
  SymMatrix* createKKT (sData* prob) const;
  void createSolversAndKKts(sData* prob);
  void createReducedRhss();

  void assembleLocalKKT( sData* prob ) override;
  void solveReducedLinkCons( sData *prob, SimpleVector& b);
  void solveReducedLinkConsBlocked( sData* data, DenseGenMatrix& rhs_mat_transp, int rhs_start, int n_rhs );
  void addInnerToHierarchicalSchurComplement( DenseSymMatrix& schur_comp, sData* data_border ) override;

 private:
  void finalizeKKTdense( sData* prob, Variables* vars);
  void finalizeKKTsparse( sData* prob, Variables* vars);
  void solveWithIterRef( sData *prob, SimpleVector& b);
  void solveWithBiCGStab( sData *prob, SimpleVector& b);

  void DsolveHierarchyBorder( DenseGenMatrix& b ) override;

  // add specified columns of given matrix Ht (either Ft or Gt) to Schur complement
  void addLinkConsBlock0Matrix( sData *prob, SparseGenMatrix& Ht, int nHtOffsetCols, int nKktOffsetCols, int startCol, int endCol);

  /** y = beta*y - alpha* SC * x */
  void SCmult ( double beta, SimpleVector& y, double alpha, SimpleVector& x, sData* prob);

  SymMatrix* CtDC{};

  std::vector<std::unique_ptr<SimpleVector>> reduced_rhss_blocked;
  SimpleVector* redRhs;

  SimpleVector* zDiagReg{};
};

#endif

