/* OOQP                                                               *
 * Authors: E. Michael Gertz, Stephen J. Wright                       *
 * (C) 2001 University of Chicago. See Copyright Notification in OOQP */

#ifndef QPGENDATA
#define QPGENDATA

#include "Problem.h"
#include "Vector.hpp"

class MpsReader;

class Variables;

#ifdef TESTING
class QpGenDataTester;
#endif

/**
 * Data for the general QP formulation.
 *
 * @ingroup QpGen
 */

class QP : public Problem {
#ifdef TESTING
   friend QpGenDataTester;
#endif

protected:

   QP() = default;

public:
   SmartPointer<SymmetricMatrix> Q;

   /** constructor that sets up pointers to the data objects that are
       passed as arguments */
   QP(Vector<double>* c, SymmetricMatrix* Q, Vector<double>* xlow, Vector<double>* ixlow, Vector<double>* xupp,
         Vector<double>* ixupp, GeneralMatrix* A, Vector<double>* bA, GeneralMatrix* C, Vector<double>* clow, Vector<double>* iclow, Vector<double>* cupp,
         Vector<double>* ciupp);

   /** insert the Hessian Q into the matrix M for the fundamental linear system, where M is stored as a SymMatrix */
   virtual void putQIntoAt(SymmetricMatrix& M, int row, int col);

   /** insert the Hessian Q into the matrix M for the fundamental linear system, where M is stored as a GenMatrix */
   virtual void putQIntoAt(GeneralMatrix& M, int row, int col);

   /** y = beta * y + alpha * Q * x */
   virtual void hessian_multiplication(double beta, Vector<double>& y, double alpha, const Vector<double>& x) const override;

   /** extract the diagonal of the Hessian and put it in the Vector<double> hessian_diagonal */
   void hessian_diagonal(Vector<double>& hessian_diagonal) override;

   void createScaleFromQ();

   void scaleQ();

   void flipQ();

   double datanorm() const override;

   virtual void datainput() {};

   virtual void datainput(MpsReader* reader, int& iErr);

   void print() override;

   virtual void objective_gradient(const Variables& variables, Vector<double>& gradient) const override;

   virtual double objective_value(const Variables& variables) const override;

   ~QP() override = default;
};

#endif