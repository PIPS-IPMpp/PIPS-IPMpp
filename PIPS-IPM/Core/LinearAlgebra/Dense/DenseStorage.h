/* OOQP                                                               *
 * Authors: E. Michael Gertz, Stephen J. Wright                       *
 * (C) 2001 University of Chicago. See Copyright Notification in OOQP */

#ifndef DENSEDOUBLEMATRIX_H
#define DENSEDOUBLEMATRIX_H

#include "DoubleMatrix.h"
#include "Vector.hpp"

extern int DenseStorageInstances;

/** A class for manupulating the storage of dense matrices.
 *  @ingroup DenseLinearAlgebra
 */
class DenseStorage : public DoubleStorage {
private:
   DenseStorage() {};
protected:
   int neverDeleteElts;
public:
   int m;
   int n;
   double** M;

   DenseStorage(int m, int n);
   DenseStorage(double A[], int m, int n);

   virtual ~DenseStorage();

   void getSize(int& m, int& n) const override;

   void getDiagonal(Vector<double>& vec) override;
   void setToDiagonal(const Vector<double>& vec) override;

   void atPutDense(int row, int col, double* A, int lda, int rowExtent, int colExtent) override;

   void fromGetDense(int row, int col, double* A, int lda, int rowExtent, int colExtent) override;

   virtual void atPutZeros(int row, int col, int rowExtent, int colExtent);

   void putZeros();

   virtual void atAddOuterProductOf(int row, int col, double alpha, double* x, int incx, int nx);

   virtual void addToDiagonalAt(double alpha, double x[], int incx, int idiag, int extent);
   void fromGetSpRow(int row, int col, double A[], int lenA, int irowA[], int& nnz, int rowExtent, int& info) override;

   void columnScale(const Vector<double>& vec) override;
   void rowScale(const Vector<double>& vec) override;
   void symmetricScale(const Vector<double>& vec) override;
   void scalarMult(double num) override;
   double abmaxnorm() const override;
   double abminnormNonZero(double tol = 1e-30) const override;

   void atPutSpRow(int col, const double A[], int lenA, int irowA[], int& info) override;
   void putSparseTriple(int irow[], int len, int jcol[], double A[], int& info);

   void atPutDiagonal(int idiag, const Vector<double>& v) override;
   void atAddDiagonal(int idiag, const Vector<double>& v) override;
   void fromGetDiagonal(int idiag, Vector<double>& v) override;
   void atPutDiagonal(int idiag, const double x[], int incx, int extent);
   void atAddDiagonal(int idiag, const double x[], int incx, int extent);
   void diagonal_add_constant_from(int from, int length, double value);

};

#endif
