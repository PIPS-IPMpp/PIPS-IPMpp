/* OOQP                                                               *
 * Authors: E. Michael Gertz, Stephen J. Wright                       *
 * (C) 2001 University of Chicago. See Copyright Notification in OOQP */

#ifndef QPGENDELINSYSSCA
#define QPGENDELINSYSSCA

#include "QpGenLinsys.h"

class QuadraticProblem;
#include "ScaGenIndefSolver.h"
#include "ScaDenSymMatrixHandle.h"
#include "OoqpVectorHandle.h"



class QpGenScaLinsys : public QpGenLinsys {
protected:
  ScaDenSymMatrixHandle kkt;
  ScaGenIndefSolver * solver;
public:
  QpGenScaLinsys( QpGen * factory,
		 QuadraticProblem * data,
		 LinearAlgebraPackage * la, ScaDenSymMatrix * Mat,
		 ScaGenIndefSolver * solver );


  virtual void solveCompressed( OoqpVector& rhs );

  virtual void putXDiagonal( OoqpVector& xdiag );
  virtual void putZDiagonal( OoqpVector& zdiag );


  virtual void factor(Problem *prob, Variables *vars);

  virtual ~QpGenScaLinsys();
};

#endif










