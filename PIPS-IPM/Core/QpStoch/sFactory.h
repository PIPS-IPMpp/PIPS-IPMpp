/* PIPS
   Authors: Cosmin Petra
   See license and copyright information in the documentation */

#ifndef SPSTOCHFACTORY
#define SPSTOCHFACTORY

#include "QpGen.h"

// save diagnostic state
#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wsuggest-override"
#include "mpi.h"
// turn the warnings back 
#pragma GCC diagnostic pop

class QpGenData;
class sData;

class QpGenVars;
class StochInputTree;
class stochasticInput;
class sTree;
class StochSymMatrix;
class sResiduals;
class sVars;
class sLinsys;
class sLinsysRoot;
class sLinsysLeaf;
#include "StochResourcesMonitor.h"

class sFactory : public QpGen {
 protected:
  int m_blocks;

  long long nnzQ, nnzA, nnzC;
 public:
  sFactory( stochasticInput&, MPI_Comm comm=MPI_COMM_WORLD );

  /** This is a obsolete constructor since it uses sTreeCallbacks to create
   *   data objects
   */
  sFactory( StochInputTree*, MPI_Comm comm=MPI_COMM_WORLD );

 protected:
  sFactory( int nx_, int my_, int mz_, int nnzQ_, int nnzA_, int nnzC_ );
  sFactory();

 public:

  virtual ~sFactory();

  virtual Data  * makeData();

   Residuals     * makeResiduals( Data * prob_in ) override;
   Variables     * makeVariables( Data * prob_in ) override;

   LinearSystem* makeLinsys( Data * prob_in ) override;


   void joinRHS( OoqpVector& rhs_in,  OoqpVector& rhs1_in,
			OoqpVector& rhs2_in, OoqpVector& rhs3_in ) override;

   void separateVars( OoqpVector& x_in, OoqpVector& y_in,
			     OoqpVector& z_in, OoqpVector& vars_in ) override;

  virtual sLinsysRoot* newLinsysRoot() = 0;
  virtual sLinsysRoot* newLinsysRoot(sData* prob,
				     OoqpVector* dd,OoqpVector* dq,
				     OoqpVector* nomegaInv, OoqpVector* rhs) = 0;

  virtual sLinsysLeaf* newLinsysLeaf(sData* prob,
				     OoqpVector* dd,OoqpVector* dq,
				     OoqpVector* nomegaInv, OoqpVector* rhs);


  sTree * tree;
  sData * data;
  //  Variables

  virtual void iterateStarted();
  virtual void iterateEnded();
  void writeProblemToStream(ostream& out, bool printRhs) const;

  sResiduals *resid;
  vector<sVars*> registeredVars;

  sLinsysRoot* linsys;

  StochIterateResourcesMonitor iterTmMonitor;
  double m_tmTotal;
};

#endif
