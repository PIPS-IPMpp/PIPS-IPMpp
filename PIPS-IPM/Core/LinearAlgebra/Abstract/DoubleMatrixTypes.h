/* OOQP                                                               *
 * Authors: E. Michael Gertz, Stephen J. Wright                       *
 * (C) 2001 University of Chicago. See Copyright Notification in OOQP */
/* Modified by Cosmin Petra and Miles Lubin */

#ifndef DOUBLEMATRIXTYPES
#define DOUBLEMATRIXTYPES

// Types for dynamic cast (we wouldn't have to do this if more C++ compilers
// supported C++ RTTI). kSquareMatrix = 2 is reserved.
enum {
   kGenMatrix = 0x01,
   kSymMatrix = 0x03,
   // 0x10-0x1F - Dense matrices
   kDenseMatrix = 0x10,
   kDenseGenMatrix = kGenMatrix + kDenseMatrix,
   kDenseSymMatrix = kSymMatrix + kDenseMatrix,
   // 0x20-0x2F - SparseMatrices
   kSparseMatrix = 0x20,
   kSparseGenMatrix = kSparseMatrix + kGenMatrix,
   kSparseSymMatrix = kSparseMatrix + kSymMatrix,
   // 0x30-0x3F - PetscMatrices
   kPetscSpMatrix = 0x30,
   kPetscSpGenMatrix = kPetscSpMatrix + kGenMatrix,
   kPetscSpSymMatrix = kPetscSpMatrix + kSymMatrix,
   // 0x40-0x4f - stoch matrices
   kStochMatrix = 0x30,
   kDistributedMatrix = kStochMatrix + kGenMatrix,
   kStochSymMatrix = kStochMatrix + kSymMatrix,
   kStochGenDummyMatrix,
   kStochSymDummyMatrix,
   kScaDenMatrix = 0x40,
   kScaDenGenMatrix = kScaDenMatrix + kGenMatrix,
   kScaDenSymMatrix = kScaDenMatrix + kSymMatrix,
   kEmtlDenMatrix = 0x50,
   kEmtlDenGenMatrix = kEmtlDenMatrix + kGenMatrix,
   kEmtlDenSymMatrix = kEmtlDenMatrix + kSymMatrix,
   // 0x60-0x6f - string matrices
   kStringMatrix = 0x60,
   kStripMatrix = kStochMatrix + kGenMatrix,
   kStringGenDummyMatrix,
   // 0x70-0x7f - bordered matrices
   kBorderedMatrix = 0x70,
   kBorderedGenMatrix = kBorderedMatrix + kGenMatrix,
   kBorderedSymMatrix = kBorderedMatrix + kSymMatrix
};

#endif
