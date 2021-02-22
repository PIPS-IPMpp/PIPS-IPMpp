/* PIPS
   Authors: Cosmin Petra
   See license and copyright information in the documentation */

#ifndef STOCHLEAFLINSYS_SCHURSLV
#define STOCHLEAFLINSYS_SCHURSLV

#include "sLinsysLeaf.h"

class StochTree;
class sFactory;
class sData;

/** This class solves the linear system corresponding to a leaf node.
 *  It just redirects the call to QpGenSparseLinsys.
 */
class sLinsysLeafSchurSlv : public sLinsysLeaf
{
 public:
  template<class LINSOLVER>
  sLinsysLeafSchurSlv(sFactory* factory,
		      sData* prob_,				    
		      OoqpVector* dd_, OoqpVector* dq_, 
		      OoqpVector* nomegaInv_,
		      OoqpVector* primal_reg_,
		      OoqpVector* dual_y_reg_,
		      OoqpVector* dual_z_reg_,
		      OoqpVector* rhs_,
            LINSOLVER* solver
         );

  void factor2(sData *prob, Variables *vars) override;
  void addTermToDenseSchurCompl(sData *prob, 
				DenseSymMatrix& SC) override;
  void addTermToSparseSchurCompl(sData *prob,
            SparseSymMatrix& SC) override;

 private:
  bool switchedToSafeSlv;

}; 
template<class LINSOLVER>
sLinsysLeafSchurSlv::sLinsysLeafSchurSlv(sFactory* factory,
					 sData* prob,
					 OoqpVector* dd_, 
					 OoqpVector* dq_, 
					 OoqpVector* nomegaInv_,
					 OoqpVector* primal_reg_,
					 OoqpVector* dual_y_reg_,
					 OoqpVector* dual_z_reg_,
					 OoqpVector* rhs_,
					 LINSOLVER* s)
: sLinsysLeaf(factory, prob, dd_, dq_, nomegaInv_, primal_reg_, dual_y_reg_, dual_z_reg_, rhs_, s),
  switchedToSafeSlv(false)
{}


#endif
