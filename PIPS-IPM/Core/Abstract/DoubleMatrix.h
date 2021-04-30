/* OOQP                                                               *
 * Authors: E. Michael Gertz, Stephen J. Wright                       *
 * (C) 2001 University of Chicago. See Copyright Notification in OOQP */

#ifndef DOUBLEMATRIX_H
#define DOUBLEMATRIX_H
/**
 * @file DoubleMatrix.h
 * @ingroup AbstractLinearAlgebra
 */

#include <cstring>
#include <iostream>
#include "Vector.hpp"
#include "SmartPointer.h"
#include "DoubleMatrixHandle.h"

class DoubleLinearSolver;

/**
 * Handle the manipulation of matrix elements
 * @ingroup AbstractLinearAlgebra
 */
class DoubleStorage : public IotrRefCount {
public:
   DoubleStorage() = default;

   virtual void atPutDense(int row, int col, double* A, int lda, int rowExtent, int colExtent) = 0;
   virtual void fromGetDense(int row, int col, double* A, int lda, int rowExtent, int colExtent) = 0;
   virtual void atPutSpRow(int row, const double A[], int lenA, int jcolA[], int& info) = 0;

   virtual void fromGetSpRow(int row, int col, double A[], int lenA, int jcolA[], int& nnz, int colExtent, int& info) = 0;

   virtual void getSize(int& m, int& n) const = 0;

   virtual void getDiagonal(Vector<double>& vec) = 0;
   virtual void setToDiagonal(const Vector<double>& vec) = 0;

   virtual void atPutDiagonal(int idiag, const Vector<double>& x) = 0;
   virtual void atAddDiagonal(int idiag, const Vector<double>& x) = 0;
   virtual void fromGetDiagonal(int idiag, Vector<double>& x) = 0;
   virtual void symmetricScale(const Vector<double>& vec) = 0;
   virtual void columnScale(const Vector<double>& vec) = 0;
   virtual void rowScale(const Vector<double>& vec) = 0;
   virtual void scalarMult(double num) = 0;
   [[nodiscard]] virtual double abmaxnorm() const = 0;
   [[nodiscard]] double abminnormNonZero() const { return abminnormNonZero(1e-30); };
   [[nodiscard]] virtual double abminnormNonZero(double tol) const = 0;
   ~DoubleStorage() override = default;
};

/** Parent of all matrix classes
 * @ingroup AbstractLinearAlgebra
 */
class DoubleMatrix : public IotrRefCount {
public:
   DoubleMatrix() = default;

   /** True if this matrix identifies itself to be of type matrixType. */
   [[nodiscard]] virtual int isKindOf(int matrixType) const = 0;

   /** Get the value of some of the elements of this matrix.
    *
    *  @param row start reading the elements of this matrix from row
    *             number "row".
    *  @param col start reading the elements of this matrix from column
    *             number "col".
    *  @param A   Any array to hold the values from this matrix.
    *  @param ldA the leading dimension of A.
    *  @param rowExtent get rowExtent rows from this matrix.
    *  @param colExtent get colExtent columns from this matrix.
    */
   virtual void fromGetDense(int row, int col, double* A, int lda, int rowExtent, int colExtent) = 0;

   /** Get one sparse row from this matrix.
    *
    * @param row  get row number "row"
    * @param col  ignore all elements of the row before column "col"
    * @param A    store the values of the sparse row in A
    * @param lenA  the length of A
    * @param jcolA an array of length lenA containing the column index of each
    *              element in the sparse row
    * @param colExtent ignore all elements of the row that have column
    *                  indices greater than or equal to col + colExtent
    * @param info info is 0 if and only if the sparse row can fit into A.
    */
   virtual void fromGetSpRow(int row, int col, double A[], int lenA, int jcolA[], int& nnz, int colExtent, int& info) = 0;

   /** Copy elements from sparse triple format into this matrix
    *  @param len the number of elements
    *  @param irow  an array containing the row number of the elements
    *  @param jcol  an array containing the column number of the elements
    *  @param A     an array containing the values for the elements.
    *  @param info  on return, info will be zero if and only if the insertion
    *               was successful.
    */
   virtual void putSparseTriple(int irow[], int len, int jcol[], double A[], int& info) = 0;

   /** y = beta * y + alpha * this * x */
   virtual void mult(double beta, Vector<double>& y, double alpha, const Vector<double>& x) const = 0;

   /** y = beta * y + alpha * this^T * x */
   virtual void transMult(double beta, Vector<double>& y, double alpha, const Vector<double>& x) const = 0;

   /** the magnitude of the element in this matrix with largest absolute value.
    */
   [[nodiscard]] virtual double abmaxnorm() const = 0;

   /** the magnitude of the element in this matrix with smallest absolute value != 0. */
   [[nodiscard]] double abminnormNonZero() const { return abminnormNonZero(1e-30); };
   [[nodiscard]] virtual double abminnormNonZero(double tol) const = 0;

   /** Write this element to a C++ stream */
   virtual void writeToStream(std::ostream& out) const = 0;

   virtual void writeToStreamDense(std::ostream& out) const = 0;

   virtual void writeToStreamDenseRow(std::ostream&, int) const { assert(false && "not implemented"); };

   virtual void writeDashedLineToStream(std::ostream&) const { assert(false && "not implemented"); };

   /** Place the diagonal elements of this matrix in the vector first */
   virtual void getDiagonal(Vector<double>& vec) = 0;
   /** Set the matrix to the diagoanl matrix whose diagonal is first */
   virtual void setToDiagonal(const Vector<double>& vec) = 0;

   /** Set some of the diagonal elements of this matrix.
    * @param idiag the index of the first diagonal element to be modified.
    * @param x the new values for the diagonal elements.
    *
    * The length of x is the number of diagonal elements to be modified.
    * Typically x will have length less than the length of the diagonal.
    */
   virtual void atPutDiagonal(int idiag, const Vector<double>& x) = 0;
   /** Add to some of the diagonal elements of this matrix.
    * @param idiag the index of the first diagonal element to be modified.
    * @param x the values to add to the diagonal elements.
    *
    * The length of x is the number of diagonal elements to be modified.
    * Typically x will have length less than the length of the diagonal.
    */
   virtual void atAddDiagonal(int idiag, const Vector<double>& x) = 0;
   virtual void diagonal_add_constant_from(int /*from*/, int /*length*/, double /*value*/) { assert(false && "not implemented"); };

   /** Get some of the diagonal elements of this matrix.
    * @param idiag the index of the first diagonal element to be read.
    * @param x a vector to hold the diagonal elements
    *
    * The length of x is the number of diagonal elements to be gotten.
    * Typically x will have length less than the length of the diagonal.
    */
   virtual void fromGetDiagonal(int idiag, Vector<double>& x) = 0;

   /** Get the number of rows and columns in the matrix
    * @param m the number of rows
    * @param n the number of columns
    */

   virtual void symmetricScale(const Vector<double>& vec) = 0;
   virtual void columnScale(const Vector<double>& vec) = 0;
   virtual void rowScale(const Vector<double>& vec) = 0;
   virtual void scalarMult(double num) = 0;

   virtual void getSize(long long& m, long long& n) const = 0;
   virtual void getSize(int& m, int& n) const = 0;

   void printSize() const {
      long long m, n;
      getSize(m, n);
      std::cout << m << " x " << n << " (rows x cols)\n";
   }

   ~DoubleMatrix() override = default;
};

/** Parent of all Symmetric matrices. 
 * @ingroup AbstractLinearAlgebra
 */
class SymMatrix : public DoubleMatrix {
public:

   /** Put a submatrix of M into this matrix.
    *
    * The submatrix is placed into this matrix in a manner that preserves
    * the symmetry of this matrix. The element of M whose destination is in
    * the lower triangle of this matrix are placed there without change, and
    * the upper triangle is taken to be the reflection across the diagonal.
    *
    * @param destRow The top row of the submatrix of M is placed at destRow.
    * @param destCol The leftmost column of the submatrix of M is placed at
    *                destCol.
    * @param srcRow The first row of the submatrix of M is srcRow.
    * @param srcCol The first column of the submatrix of M is srcCol.
    * @param rowExtent rowExtent rows are copied from M.
    * @param colExtent colExtent columns are copied from M.
    */
   virtual void symAtPutSubmatrix(int destRow, int destCol, DoubleMatrix& M, int srcRow, int srcCol, int rowExtent, int colExtent) = 0;

   /** Randomize the elements of this matrix in a manner that makes the matrix
    *  positive semi-definite.
    */
   virtual void randomizePSD(double* seed) = 0;

   /** Put a sparse row into this matrix symmetrically.
    *
    *  Elements of the row that would lie above the diagonal are ignored.
    *  The matrix is symmetrized by reflection across the diagonal.
    *
    *  The meaning of the parmameters is the same as in fromGetSpRow.
    *  @see DoubleMatrix::fromGetSpRow
    */
   virtual void symAtPutSpRow(int col, double A[], int lenA, int irowA[], int& info) = 0;
   /** the size of this square matrix */
   [[nodiscard]] virtual long long size() const = 0;

   /** deep clone matrix */
   [[nodiscard]] virtual SymMatrix* clone() const {
      assert(false && "not implemented");
      return nullptr;
   };

};

/** Parent of all non-symmetric, possibly non-square, matrices.
 * @ingroup AbstractLinearAlgebra
 */
class GenMatrix : public DoubleMatrix {
public:
   /** Put a submatrix of M into this matrix.
    *
    * @param destRow The top row of the submatrix of M is placed at destRow.
    * @param destCol The leftmost column of the submatrix of M is placed at
    *                destCol.
    * @param srcRow The first row of the submatrix of M is srcRow.
    * @param srcCol The first column of the submatrix of M is srcCol.
    * @param rowExtent rowExtent rows are copied from M.
    * @param colExtent colExtent columns are copied from M.
    */
   virtual void atPutSubmatrix(int destRow, int destCol, DoubleMatrix& M, int srcRow, int srcCol, int rowExtent, int colExtent) = 0;

   /** Set the value of some of the elements of this matrix.
    *
    *  @see DoubleMatrix::fromGetDense
    */
   virtual void atPutDense(int row, int col, double* A, int lda, int rowExtent, int colExtent) = 0;

   /** Put a sparse row into this matrix.
    *
    *  The meaning of the parmameters is the same as in fromGetSpRow.
    *  @see DoubleMatrix::fromGetSpRow
    */
   virtual void atPutSpRow(int col, double A[], int lenA, int jcolA[], int& info) = 0;

   /** Fill this matrix with random elements.
    * @param alpha the elements will be no smaller than alpha.
    * @param beta the elements will be no larger than beta.
    * @param seed a arbitrary number used to seed the random number
    *             generator.
    */
   virtual void randomize(double alpha, double beta, double* seed) = 0;

   /** C = this^T * D * this where D=diag(d) is a diagonal matrix. */
   virtual void matTransDMultMat(Vector<double>& d, SymMatrix** res) = 0;

   /** C = this^T * inv(D) * this where D=diag(d) is a diagonal matrix. */
   virtual void matTransDinvMultMat(Vector<double>& d, SymMatrix** res) = 0;

   virtual void writeMPSformatRows(std::ostream& /*out*/, int /*rowType*/, Vector<double>* /*irhs*/) const {}

   /** get number of elements per row to given vector */
   virtual void getNnzPerRow(Vector<int>& /*nnzVec*/) { assert(0 && "not implemented"); };

   /** get number of elements per column to given vector */
   virtual void getNnzPerCol(Vector<int>& /*nnzVec*/) { assert(0 && "not implemented"); };

   /** fill vector with absolute minimum/maximum value of each row */
   virtual void getRowMinMaxVec(bool /*getMin*/, bool /*initializeVec*/, const Vector<double>* /*colScaleVec*/, Vector<double>& /*minmaxVec*/ ) {
      assert(0 && "not implemented");
   };

   /** fill vector with absolute minimum/maximum value of each column */
   virtual void getColMinMaxVec(bool /*getMin*/, bool /*initializeVec*/, const Vector<double>* /*rowScaleVec*/, Vector<double>& /*minmaxVec*/ ) {
      assert(0 && "not implemented");
   };

   /** add absolute value sum of each row to vector */
   virtual void addRowSums(Vector<double>& /*first*/ ) const { assert(0 && "not implemented"); };

   /** add absolute value sum of each column to vector */
   virtual void addColSums(Vector<double>& /*first*/ ) const { assert(0 && "not implemented"); };

   /** return nonzeros in matrix */
   [[nodiscard]] virtual int numberOfNonZeros() const {
      assert(false && "not implemented");
      return -1;
   };

   /** clone of matrix with n = 0 - possibly with underlying dynamic sparse storage */
   [[nodiscard]] virtual GenMatrix* cloneEmptyRows(bool /*s witchToDynamicStorage = false*/) const {
      assert(false && "not implemented");
      return nullptr;
   };

   /** full clone of matrix - possibly with underlying dynamic sparse storage */
   [[nodiscard]] virtual GenMatrix* cloneFull(bool /* switchToDynamicStorage = false */) const {
      assert(false && "not implmented");
      return nullptr;
   };

   /** shave of bottom n constraints and return them in a new matrix */
   [[nodiscard]] virtual GenMatrix* shaveBottom(int /* nrows */) {
      assert(false && "not implemented");
      return nullptr;
   };
};

#endif
