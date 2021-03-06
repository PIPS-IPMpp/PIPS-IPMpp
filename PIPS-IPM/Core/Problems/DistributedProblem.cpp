#include "DistributedProblem.hpp"
#include "DistributedTree.h"
#include "DistributedTreeCallbacks.h"
#include "DistributedSymmetricMatrix.h"
#include "DistributedMatrix.h"
#include "DistributedVector.h"
#include "mpi.h"
#include "PIPSIPMppOptions.h"
#include "BorderedSymmetricMatrix.h"
#include "BorderedMatrixLiftedA0wrapper.h"
#include <iostream>
#include <numeric>
#include <functional>

static bool blockIsInRange(int block, int blocksStart, int blocksEnd) {
   return ((block >= (blocksStart - 1) && block < blocksEnd) || block == -1);
}

static int nnzTriangular(int size) {
   assert(size >= 0);
   return ((1 + size) * size) / 2;
}

static void appendRowDense(int start, int end, int& nnz, int* jcolM) {
   assert(jcolM);
   assert(nnz >= 0);
   assert(start >= 0 && end >= start);

   for (int i = start; i < end; i++)
      jcolM[nnz++] = i;
}

static void
appendRowSparse(int startColIdx, int endColIdx, int colOffset, const int* jcolM_append, int& nnz, int* jcolM) {
   assert(jcolM);
   assert(nnz >= 0);
   assert(startColIdx >= 0 && startColIdx <= endColIdx);

   for (int c = startColIdx; c < endColIdx; c++)
      jcolM[nnz++] = colOffset + jcolM_append[c];
}


static int
appendDiagBlocks(const std::vector<int>& linkStartBlockId, const std::vector<int>& linkStartBlockLengths,
   int borderstart, int bordersize, int rowSC,
   int rowBlock, int& blockStartrow, int& nnz, int* jcolM) {
   assert(rowBlock >= blockStartrow && blockStartrow >= 0 && borderstart >= 0 && bordersize >= 0 && nnz >= 0);

   const int block = linkStartBlockId[rowBlock];
   const int currlength = (block >= 0) ? linkStartBlockLengths[block] : bordersize;

   assert(currlength >= 1);

   // add diagonal block (possibly up to the border)

   int rownnz = currlength - (rowBlock - blockStartrow);

   for (int i = 0; i < rownnz; ++i)
      jcolM[nnz++] = rowSC + i;

   // with offdiagonal blocks?
   if (block >= 0) {
      // add right off-diagonal block and border part

      const int nextlength = linkStartBlockLengths[block + 1];

      assert(nextlength >= 0);
      assert(block != int(linkStartBlockLengths.size()) - 2 || nextlength == 0);

      for (int i = rownnz; i < rownnz + nextlength; ++i)
         jcolM[nnz++] = rowSC + i;

      rownnz += nextlength + bordersize;

      for (int i = borderstart; i < borderstart + bordersize; ++i)
         jcolM[nnz++] = i;

      // last row of current block?
      if (rowBlock + 1 == blockStartrow + currlength)
         blockStartrow = rowBlock + 1;
   }

   return rownnz;
}

static void
appendDiagBlocksDist(const std::vector<int>& linkStartBlockId, const std::vector<int>& linkStartBlockLengths,
   int borderstart, int bordersize,
   int rowSC, int rowBlock, int blocksStart, int blocksEnd, int& blockStartrow, int& nnz, int* jcolM) {
   assert(rowBlock >= blockStartrow && blockStartrow >= 0 && borderstart >= 0 && bordersize >= 0 && nnz >= 0);

   const int block = linkStartBlockId[rowBlock];
   const int lastBlock = blocksEnd - 1;

   if (blockIsInRange(block, blocksStart, blocksEnd)) {
      const int currlength = (block >= 0) ? linkStartBlockLengths[block] : bordersize;
      const int rownnz = currlength - (rowBlock - blockStartrow);

      assert(currlength >= 1);

      // add diagonal block (possibly up to the border)
      for (int i = 0; i < rownnz; ++i)
         jcolM[nnz++] = rowSC + i;

      // with off-diagonal blocks? (at sparse part)
      if (block >= 0) {
         // add right off-diagonal block and border part

         const int nextlength = (block == lastBlock) ? 0 : linkStartBlockLengths[block + 1];

         assert(nextlength >= 0);
         assert(block != int(linkStartBlockLengths.size()) - 1 || nextlength == 0);

         for (int i = rownnz; i < rownnz + nextlength; ++i)
            jcolM[nnz++] = rowSC + i;

         for (int i = borderstart; i < borderstart + bordersize; ++i)
            jcolM[nnz++] = i;

         // last row of current block?
         if (rowBlock + 1 == blockStartrow + currlength)
            blockStartrow = rowBlock + 1;
      }
   }

   // at sparse part?
   if (block >= 0) {
      const int currlength = linkStartBlockLengths[block];
      assert(currlength >= 1);

      // last row of current block?
      if (rowBlock + 1 == blockStartrow + currlength)
         blockStartrow = rowBlock + 1;
   }
}


static int
appendMixedBlocks(const std::vector<int>& linkStartBlockId_Left, const std::vector<int>& linkStartBlockId_Right,
   const std::vector<int>& linkStartBlockLengths_Left, const std::vector<int>& linkStartBlockLengths_Right,
   int colStartIdxSC, int bordersize_cols,
   int rowIdx, int& colIdxOffset, int& rowBlockStartIdx, int& nnz, int* jcolM) {
   assert(rowIdx >= rowBlockStartIdx && rowBlockStartIdx >= 0 && colStartIdxSC >= 0 && nnz >= 0);
   assert(bordersize_cols >= 0 && colIdxOffset >= 0);
   assert(linkStartBlockLengths_Left.size() == linkStartBlockLengths_Right.size());

   const int nCols = int(linkStartBlockId_Right.size());
   const int block = linkStartBlockId_Left[rowIdx];

   assert(nCols >= bordersize_cols);

   int rownnz;

   // sparse row?
   if (block >= 0) {
      const int length_Right = linkStartBlockLengths_Right[block];
      const int colStartIdxBorderSC = colStartIdxSC + nCols - bordersize_cols;
      int colStartIdx = colStartIdxSC + colIdxOffset;
      rownnz = 0;

      assert(length_Right >= 0);

      // 1) left off-diagonal block (not for first block)
      if (block >= 1) {
         const int prevlength_Right = linkStartBlockLengths_Right[block - 1];
         assert(prevlength_Right >= 0);

         for (int i = 0; i < prevlength_Right; ++i)
            jcolM[nnz++] = (colStartIdx + i);

         rownnz += prevlength_Right;
         colStartIdx += prevlength_Right;
      }

      // 2) diagonal block
      for (int i = 0; i < length_Right; ++i)
         jcolM[nnz++] = (colStartIdx + i);

      rownnz += length_Right;
      colStartIdx += length_Right;

      // 3) right off-diagonal block (not for last block)
      if (int(linkStartBlockLengths_Left.size()) != block + 1) {
         const int nextlength_Right = linkStartBlockLengths_Right[block + 1];

         for (int i = 0; i < nextlength_Right; ++i)
            jcolM[nnz++] = (colStartIdx + i);

         rownnz += nextlength_Right;
         colStartIdx += nextlength_Right;
      }

      assert(colStartIdx <= colStartIdxBorderSC);

      // 4) right border
      for (int i = 0; i < bordersize_cols; ++i)
         jcolM[nnz++] = colStartIdxBorderSC + i;

      rownnz += bordersize_cols;

      // last row of current block?
      if (rowIdx + 1 == rowBlockStartIdx + linkStartBlockLengths_Left[block]) {
         rowBlockStartIdx = rowIdx + 1;

         if (block >= 1)
            colIdxOffset += linkStartBlockLengths_Right[block - 1];
      } else {
         assert(block == linkStartBlockId_Left[rowIdx + 1]);
      }
   } else {
      // append fully dense row
      for (int i = 0; i < nCols; ++i)
         jcolM[nnz++] = colStartIdxSC + i;

      rownnz = nCols;
   }

   return rownnz;
}


static void
appendMixedBlocksDist(const std::vector<int>& linkStartBlockId_Left, const std::vector<int>& linkStartBlockId_Right,
   const std::vector<int>& linkStartBlockLengths_Left, const std::vector<int>& linkStartBlockLengths_Right,
   int colStartIdxSC, int bordersize_cols,
   int rowIdx, int blocksStart, int blocksEnd, int& colIdxOffset, int& rowBlockStartIdx, int& nnz, int* jcolM) {
   assert(rowIdx >= rowBlockStartIdx && rowBlockStartIdx >= 0 && colStartIdxSC >= 0 && nnz >= 0);
   assert(bordersize_cols >= 0 && colIdxOffset >= 0);
   assert(linkStartBlockLengths_Left.size() == linkStartBlockLengths_Right.size());

   const int block = linkStartBlockId_Left[rowIdx];
   const bool blockInRange = blockIsInRange(block, blocksStart, blocksEnd);
   const int nCols = int(linkStartBlockId_Right.size());

   assert(nCols >= bordersize_cols);

   // sparse row?
   if (block >= 0) {
      if (blockInRange) {
         const int length_Right = linkStartBlockLengths_Right[block];
         const int colStartIdxBorderSC = colStartIdxSC + nCols - bordersize_cols;
         int colStartIdx = colStartIdxSC + colIdxOffset;

         assert(length_Right >= 0);

         // 1) left off-diagonal block (not for first block)
         if (block >= 1 && block != blocksStart - 1) {
            const int prevlength_Right = linkStartBlockLengths_Right[block - 1];
            assert(prevlength_Right >= 0);

            for (int i = 0; i < prevlength_Right; ++i)
               jcolM[nnz++] = (colStartIdx + i);
         }

         if (block >= 1) {
            const int prevlength_Right = linkStartBlockLengths_Right[block - 1];
            assert(prevlength_Right >= 0);

            colStartIdx += prevlength_Right;
         }

         // 2) diagonal block
         for (int i = 0; i < length_Right; ++i)
            jcolM[nnz++] = (colStartIdx + i);

         colStartIdx += length_Right;

         // 3) right off-diagonal block (not for last block)
         if (block != blocksEnd - 1) {
            const int nextlength_Right = linkStartBlockLengths_Right[block + 1];

            for (int i = 0; i < nextlength_Right; ++i)
               jcolM[nnz++] = (colStartIdx + i);

            colStartIdx += nextlength_Right;
         }

         assert(colStartIdx <= colStartIdxBorderSC);

         // 4) right border
         for (int i = 0; i < bordersize_cols; ++i)
            jcolM[nnz++] = colStartIdxBorderSC + i;
      }

      // last row of current block?
      if (rowIdx + 1 == rowBlockStartIdx + linkStartBlockLengths_Left[block]) {
         rowBlockStartIdx = rowIdx + 1;

         if (block >= 1)
            colIdxOffset += linkStartBlockLengths_Right[block - 1];
      } else {
         assert(block == linkStartBlockId_Left[rowIdx + 1]);
      }
   } else if (blockInRange) {
      assert(block == -1);

      // append dense row, but skip entries not in range
      for (int i = 0; i < nCols; ++i) {
         const int blockRight = linkStartBlockId_Right[i];
         const bool blockRightInRange = blockIsInRange(blockRight, blocksStart, blocksEnd);

         if (blockRightInRange)
            jcolM[nnz++] = colStartIdxSC + i;
      }
   }
}


void DistributedProblem::getSCrangeMarkers(int blocksStart, int blocksEnd, int& local2linksStartEq, int& local2linksEndEq,
   int& local2linksStartIneq,
   int& local2linksEndIneq) const {
   const int blocksStartReal = (blocksStart > 0) ? (blocksStart - 1) : blocksStart;
   const int nx0 = getLocalnx();
   const int my0 = getLocalmy();
   const int myl = getLocalmyl();
   local2linksStartEq = nx0 + my0;
   local2linksStartIneq = nx0 + my0 + myl;

   for (int block = 0; block < blocksStartReal; ++block) {
      const int lengthEq = linkStartBlockLengthsA[block];
      const int lengthIneq = linkStartBlockLengthsC[block];

      assert(lengthEq >= 0 && lengthIneq >= 0);

      local2linksStartEq += lengthEq;
      local2linksStartIneq += lengthIneq;
   }

   local2linksEndEq = local2linksStartEq + getSCdiagBlocksNRows(linkStartBlockLengthsA, blocksStart, blocksEnd);
   local2linksEndIneq = local2linksStartIneq + getSCdiagBlocksNRows(linkStartBlockLengthsC, blocksStart, blocksEnd);
}

void DistributedProblem::getSCrangeMarkersMy(int blocksStart, int blocksEnd, int& local2linksStartEq, int& local2linksEndEq,
   int& local2linksStartIneq,
   int& local2linksEndIneq) const {
   const int nx0 = getLocalnx();
   const int my0 = getLocalmy();
   const int myl = getLocalmyl();
   local2linksStartEq = nx0 + my0;
   local2linksStartIneq = nx0 + my0 + myl;

   for (int block = 0; block < blocksStart; ++block) {
      const int lengthEq = linkStartBlockLengthsA[block];
      const int lengthIneq = linkStartBlockLengthsC[block];

      assert(lengthEq >= 0 && lengthIneq >= 0);

      local2linksStartEq += lengthEq;
      local2linksStartIneq += lengthIneq;
   }

   local2linksEndEq = local2linksStartEq + getSCdiagBlocksNRowsMy(linkStartBlockLengthsA, blocksStart, blocksEnd);
   local2linksEndIneq = local2linksStartIneq + getSCdiagBlocksNRowsMy(linkStartBlockLengthsC, blocksStart, blocksEnd);
}


int DistributedProblem::getSCdiagBlocksNRows(const std::vector<int>& linkStartBlockLengths, int blocksStart, int blocksEnd) {
   assert(blocksStart >= 0 && blocksStart < blocksEnd);
   assert(blocksEnd <= int(linkStartBlockLengths.size()));

   int nRowsRange = 0;
   const int blocksStartReal = (blocksStart > 0) ? (blocksStart - 1) : blocksStart;

   // main loop, going over specified 2-link blocks
   for (int block = blocksStartReal; block < blocksEnd; ++block) {
      const int length = linkStartBlockLengths[block];
      assert(length >= 0);

      nRowsRange += length;
   }
   return nRowsRange;
}


int
DistributedProblem::getSCdiagBlocksNRowsMy(const std::vector<int>& linkStartBlockLengths, int blocksStart, int blocksEnd) {
   assert(blocksStart >= 0 && blocksStart < blocksEnd);
   assert(blocksEnd <= int(linkStartBlockLengths.size()));

   int nRowsRange = 0;

   // main loop, going over specified 2-link blocks
   for (int block = blocksStart; block < blocksEnd; ++block) {
      const int length = linkStartBlockLengths[block];
      assert(length >= 0);

      nRowsRange += length;
   }
   return nRowsRange;
}

int DistributedProblem::getSCdiagBlocksNRows(const std::vector<int>& linkStartBlockLengths) {
   return (getSCdiagBlocksNRows(linkStartBlockLengths, 0, int(linkStartBlockLengths.size())));
}

int DistributedProblem::getSCdiagBlocksMaxNnz(size_t nRows, const std::vector<int>& linkStartBlockLengths) {
   const size_t nBlocks = linkStartBlockLengths.size();
   size_t nRowsSparse = 0;

   int nnz = 0;

   // main loop, going over all 2-link blocks
   for (size_t block = 0; block < nBlocks; ++block) {
      if (linkStartBlockLengths[block] == 0)
         continue;

      const int length = linkStartBlockLengths[block];
      const int nextlength = linkStartBlockLengths[block + 1];

      assert(length > 0);
      assert(nextlength >= 0);
      assert(block != linkStartBlockLengths.size() - 2 || nextlength == 0);

      nRowsSparse += size_t(length);

      // diagonal block
      nnz += nnzTriangular(length);

      // (one) off-diagonal block
      nnz += length * nextlength;
   }

   // any rows left?
   if (nRowsSparse < nRows) {
      const size_t nRowsDense = nRows - nRowsSparse;
      nnz += nnzTriangular(nRowsDense) + nRowsDense * nRowsSparse;
   }

   return nnz;
}


int
DistributedProblem::getSCdiagBlocksMaxNnzDist(size_t nRows, const std::vector<int>& linkStartBlockLengths, int blocksStart,
   int blocksEnd) {
#ifndef NDEBUG
   const int nblocks = int(linkStartBlockLengths.size());
#endif
   assert(blocksStart >= 0);
   assert(blocksStart < blocksEnd);
   assert(blocksEnd <= nblocks);
   assert(nblocks >= 2);

   const int nRowsSparse = getSCdiagBlocksNRows(linkStartBlockLengths);
   const int nRowsSparseRange = getSCdiagBlocksNRows(linkStartBlockLengths, blocksStart, blocksEnd);

   int nnz = 0;

   // main loop, going over specified 2-link blocks
   for (int block = blocksStart; block < blocksEnd; ++block) {
      const int length = linkStartBlockLengths[block];

      if (length == 0)
         continue;

      const int prevlength = (block == 0) ? 0 : linkStartBlockLengths[block - 1];

      assert(length > 0);
      assert(prevlength >= 0);
      assert(block != nblocks - 1); // length should be 0 for last block

      // diagonal block
      nnz += nnzTriangular(length);

      // above off-diagonal block
      nnz += prevlength * length;
   }

   if (blocksStart > 0) {
      const int prevlength = linkStartBlockLengths[blocksStart - 1];

      nnz += nnzTriangular(prevlength);
   }

   // any rows left?
   if (nRowsSparse < int(nRows)) {
      const int nRowsDense = int(nRows) - nRowsSparse;
      nnz += nnzTriangular(nRowsDense);
      nnz += nRowsDense * nRowsSparseRange;
   }

   return nnz;
}

int DistributedProblem::getSCmixedBlocksMaxNnz(size_t nRows, size_t nCols, const std::vector<int>& linkStartBlockLength_Left,
   const std::vector<int>& linkStartBlockLength_Right) {
   assert(linkStartBlockLength_Left.size() == linkStartBlockLength_Right.size());

   const size_t nBlocks = linkStartBlockLength_Left.size();
   size_t nRowsSparse = 0;
   size_t nColsSparse = 0;

   int nnz = 0;

   // main loop, going over all 2-link blocks
   for (size_t block = 0; block < nBlocks; ++block) {
      const int length_Left = linkStartBlockLength_Left[block];
      const int length_Right = linkStartBlockLength_Right[block];
      assert(length_Left >= 0 && length_Right >= 0);

      nRowsSparse += size_t(length_Left);
      nColsSparse += size_t(length_Right);

      // diagonal block
      nnz += length_Left * length_Right;

      if (block == 0)
         continue;

      const int prevlength_Left = linkStartBlockLength_Left[block - 1];
      const int prevlength_Right = linkStartBlockLength_Right[block - 1];

      assert(prevlength_Left >= 0 && prevlength_Right >= 0);

      // left off-diagonal block
      nnz += length_Left * prevlength_Right;

      // upper off-diagonal block
      nnz += prevlength_Left * length_Right;
   }

   // dense border?
   if (nRowsSparse < nRows || nColsSparse < nCols) {
      assert(nRowsSparse <= nRows && nColsSparse <= nCols);

      const size_t nRowsDense = nRows - nRowsSparse;
      const size_t nColsDense = nCols - nColsSparse;

      nnz += nRowsDense * nColsSparse; // lower border part without right border
      nnz += nColsDense * nRows;       // complete right border
   }

   return nnz;
}


int
DistributedProblem::getSCmixedBlocksMaxNnzDist(size_t nRows, size_t nCols, const std::vector<int>& linkStartBlockLength_Left,
   const std::vector<int>& linkStartBlockLength_Right, int blocksStart, int blocksEnd) {
   assert(linkStartBlockLength_Left.size() == linkStartBlockLength_Right.size());
   assert(blocksStart >= 0);
   assert(blocksStart < blocksEnd);

#ifndef NDEBUG
   const int nBlocks = int(linkStartBlockLength_Left.size());
#endif
   const int blockLast = blocksEnd - 1;
   const int blocksStartReal = (blocksStart > 0) ? (blocksStart - 1) : 0;
   const int nRowsSparse = getSCdiagBlocksNRows(linkStartBlockLength_Left);
   const int nColsSparse = getSCdiagBlocksNRows(linkStartBlockLength_Right);
   const int nRowsSparseRange = getSCdiagBlocksNRows(linkStartBlockLength_Left, blocksStart, blocksEnd);
   const int nColsSparseRange = getSCdiagBlocksNRows(linkStartBlockLength_Right, blocksStart, blocksEnd);

   assert(nBlocks > 1 && blocksEnd <= nBlocks);
   assert(linkStartBlockLength_Left[nBlocks - 1] == 0 && linkStartBlockLength_Right[nBlocks - 1] == 0);

   int nnz = 0;

   // main loop, going over all 2-link blocks
   for (int block = blocksStartReal; block < blocksEnd; ++block) {
      const int length_Left = linkStartBlockLength_Left[block];
      const int length_Right = linkStartBlockLength_Right[block];

      // left off-diagonal block
      if (block != blocksStartReal) {
         assert(block >= 1);
         assert(linkStartBlockLength_Right[block - 1] >= 0);

         const int lastlenght_right = linkStartBlockLength_Right[block - 1];
         nnz += length_Left * lastlenght_right;
      }

      // diagonal block
      nnz += length_Left * length_Right;

      // right off-diagonal block
      if (block != blockLast) {
         assert(block < nBlocks - 1);

         const int nextlength_Right = linkStartBlockLength_Right[block + 1];

         assert(nextlength_Right >= 0);

         nnz += length_Left * nextlength_Right;
      }
   }

   // dense (right or lower) border?
   if (nRowsSparse < int(nRows) || nColsSparse < int(nCols)) {
      assert(nRowsSparse <= int(nRows) && nColsSparse <= int(nCols));

      const int nRowsDense = int(nRows) - nRowsSparse;
      const int nColsDense = int(nCols) - nColsSparse;

      nnz += nRowsDense * nColsSparseRange;  // lower left border part (without right border)
      nnz += nRowsSparseRange * nColsDense;  // upper right border
      nnz += nRowsDense * nColsDense;        // lower right border

   }

   return nnz;
}


int DistributedProblem::n2linksRows(const std::vector<int>& linkStartBlockLengths) {
   int n = 0;

   for (int linkStartBlockLength : linkStartBlockLengths)
      n += linkStartBlockLength;

   return n;
}

std::vector<int> DistributedProblem::get2LinkLengthsVec(const std::vector<int>& linkStartBlockId, const size_t nBlocks) {
   std::vector<int> linkStartBlockLengths(nBlocks, 0);

   const size_t nlinks = linkStartBlockId.size();

   for (size_t i = 0; i < nlinks; i++) {
      const int block = linkStartBlockId[i];

      if (block >= 0) {
         assert(size_t(block) < nBlocks);
         linkStartBlockLengths[block]++;
      }
   }
   assert(nBlocks == 0 || linkStartBlockLengths[nBlocks - 1] == 0);

   return linkStartBlockLengths;
}

std::unique_ptr<SparseSymmetricMatrix> DistributedProblem::createSchurCompSymbSparseUpper() const {
   assert(!children.empty());
   const int nx0 = getLocalnx();
   const int my0 = getLocalmy();
   const int myl = getLocalmyl();
   const int mzl = getLocalmzl();
   const int sizeSC = nx0 + my0 + myl + mzl;
   const int nnz = getSchurCompMaxNnz();

   assert(nnz > 0);

   int* krowM = new int[sizeSC + 1];
   int* jcolM = new int[nnz];
   auto* M = new double[nnz];
   std::uninitialized_fill(M, M + nnz, 0);

   krowM[0] = 0;

#ifndef NDEBUG
   if (!is_hierarchy_inner_leaf) {
      const auto[bm, bn] = getLocalB().getTranspose().n_rows_columns();
      assert(bm == nx0 && (bn == my0 || my0 == 0));
   } else {
      assert(nx0 == 0);
      assert(my0 == 0);
   }
#endif

   const int nx0NonZero = nx0 - n0LinkVars;
   int nnzcount = 0;

   assert(nx0NonZero >= 0);

   // dense square block, B_0^T, and dense border blocks
   for (int i = 0; i < nx0NonZero; ++i) {
      // get B_0^T (resp. A_0^T)
      const SparseMatrix& Btrans = getLocalB().getTranspose();
      const int* startRowBtrans = Btrans.krowM();
      const int* colidxBtrans = Btrans.jcolM();

      const int blength = my0 > 0 ? startRowBtrans[i + 1] - startRowBtrans[i] : 0;
      assert(blength >= 0);

      krowM[i + 1] = krowM[i] + (nx0 - i) + blength + myl + mzl;

      /* dense square block */
      appendRowDense(i, nx0, nnzcount, jcolM);

      /* B0^T */
      if (my0 > 0) {
         appendRowSparse(startRowBtrans[i], startRowBtrans[i + 1], nx0, colidxBtrans, nnzcount, jcolM);
      }

      /* dense sum X1^T Fi^T and X1^T Gi^T */
      appendRowDense(nx0 + my0, nx0 + my0 + myl + mzl, nnzcount, jcolM);

      assert(nnzcount == krowM[i + 1]);
   }

   // dense square block from 0LinkVars and rest of B_0^T, F_0^T, G_0^T
   for (int i = nx0NonZero; i < nx0; ++i) {
      appendRowDense(i, nx0, nnzcount, jcolM);

      if (my0 > 0) {
         // get B_0^T (resp. A_0^T)
         const SparseMatrix& Btrans = getLocalB().getTranspose();
         const int* startRowBtrans = Btrans.krowM();
         const int* colidxBtrans = Btrans.jcolM();

         appendRowSparse(startRowBtrans[i], startRowBtrans[i + 1], nx0, colidxBtrans, nnzcount, jcolM);
      }

      if (myl > 0) {
         const SparseMatrix& Ft = getLocalF().getTranspose();
         const int* startRowFtrans = Ft.krowM();
         const int* colidxFtrans = Ft.jcolM();

         appendRowSparse(startRowFtrans[i], startRowFtrans[i + 1], nx0 + my0, colidxFtrans, nnzcount, jcolM);
      }

      if (mzl > 0) {
         const SparseMatrix& Gt = getLocalG().getTranspose();
         const int* startRowGtrans = Gt.krowM();
         const int* colidxGtrans = Gt.jcolM();

         appendRowSparse(startRowGtrans[i], startRowGtrans[i + 1], nx0 + my0 + myl, colidxGtrans, nnzcount, jcolM);
      }

      krowM[i + 1] = nnzcount;
   }

   // empty rows; put diagonal for PARDISO
   for (int i = nx0; i < nx0 + my0; ++i) {
      const int rowStartIdx = krowM[i];

      jcolM[rowStartIdx] = i;
      krowM[i + 1] = rowStartIdx + 1;
   }

   nnzcount += my0;

   // equality linking: sparse diagonal blocks, and mixed rows
   int blockStartrow = 0;
   const int n2linksRowsEq = n2linkRowsEq();
   const int bordersizeEq = linkStartBlockIdA.size() - n2linksRowsEq;
   const int borderstartEq = nx0 + my0 + n2linksRowsEq;
   const int n2linksRowsIneq = n2linkRowsIneq();
   const int bordersizeIneq = linkStartBlockIdC.size() - n2linksRowsIneq;
   const int borderstartIneq = nx0 + my0 + myl + n2linksRowsIneq;

   assert(bordersizeEq >= 0 && n2linksRowsEq <= myl);
   assert(bordersizeIneq >= 0 && n2linksRowsIneq <= mzl);

   for (int i = nx0 + my0, j = 0, colIdxOffset = 0, blockStartrowMix = 0; i < nx0 + my0 + myl; ++i, ++j) {
      int blockrownnz = appendDiagBlocks(linkStartBlockIdA, linkStartBlockLengthsA, borderstartEq, bordersizeEq, i, j,
         blockStartrow, nnzcount,
         jcolM);

      blockrownnz += appendMixedBlocks(linkStartBlockIdA, linkStartBlockIdC, linkStartBlockLengthsA,
         linkStartBlockLengthsC, (nx0 + my0 + myl),
         bordersizeIneq, j, colIdxOffset, blockStartrowMix, nnzcount, jcolM);

      assert(blockStartrowMix == blockStartrow);

      krowM[i + 1] = krowM[i] + blockrownnz;
   }

   // inequality linking: dense border block and sparse diagonal blocks
   blockStartrow = 0;

   for (int i = nx0 + my0 + myl, j = 0; i < nx0 + my0 + myl + mzl; ++i, ++j) {
      const int blockrownnz = appendDiagBlocks(linkStartBlockIdC, linkStartBlockLengthsC, borderstartIneq,
         bordersizeIneq, i, j, blockStartrow,
         nnzcount, jcolM);

      krowM[i + 1] = krowM[i] + blockrownnz;
   }

   assert(nnzcount == nnz);

   return std::make_unique<SparseSymmetricMatrix>(sizeSC, nnz, krowM, jcolM, M, 1, false);
}


std::unique_ptr<SparseSymmetricMatrix> DistributedProblem::createSchurCompSymbSparseUpperDist(int blocksStart, int blocksEnd) const {
   assert(!children.empty());

   const int nx0 = getLocalnx();
   const int my0 = getLocalmy();
   const int myl = getLocalmyl();
   const int mzl = getLocalmzl();
   const int mylLocal = myl - getSCdiagBlocksNRows(linkStartBlockLengthsA) +
      getSCdiagBlocksNRows(linkStartBlockLengthsA, blocksStart, blocksEnd);
   const int mzlLocal = mzl - getSCdiagBlocksNRows(linkStartBlockLengthsC) +
      getSCdiagBlocksNRows(linkStartBlockLengthsC, blocksStart, blocksEnd);
   const int sizeSC = nx0 + my0 + myl + mzl;
   const int nnz = getSchurCompMaxNnzDist(blocksStart, blocksEnd);

   assert(getSchurCompMaxNnzDist(0, linkStartBlockLengthsA.size()) == getSchurCompMaxNnz());
   assert(blocksStart >= 0 && blocksStart < blocksEnd);
   assert(nnz > 0);
   assert(myl >= 0 && mzl >= 0);

   int* const krowM = new int[sizeSC + 1];
   int* const jcolM = new int[nnz];
   auto* const M = new double[nnz];
   std::uninitialized_fill(M, M + nnz, 0);

   krowM[0] = 0;

   // get B_0^T (resp. A_0^T)
   const SparseMatrix& Btrans = getLocalB().getTranspose();
   const int* startRowBtrans = Btrans.krowM();
   const int* colidxBtrans = Btrans.jcolM();

#ifndef NDEBUG
   const auto[bm, bn] = Btrans.n_rows_columns();
   assert(bm == nx0 && bn == my0);
#endif

   const int nx0NonZero = nx0 - n0LinkVars;
   const int n2linksRowsEq = n2linkRowsEq();
   const int bordersizeEq = linkStartBlockIdA.size() - n2linksRowsEq;
   const int borderstartEq = nx0 + my0 + n2linksRowsEq;
   const int n2linksRowsIneq = n2linkRowsIneq();
   const int bordersizeIneq = linkStartBlockIdC.size() - n2linksRowsIneq;
   const int borderstartIneq = nx0 + my0 + myl + n2linksRowsIneq;
   int local2linksStartEq;
   int local2linksEndEq;
   int local2linksStartIneq;
   int local2linksEndIneq;

   this->getSCrangeMarkers(blocksStart, blocksEnd, local2linksStartEq, local2linksEndEq, local2linksStartIneq,
      local2linksEndIneq);

   assert(nx0NonZero >= 0);
   assert(bordersizeEq >= 0 && n2linksRowsEq <= myl);
   assert(bordersizeIneq >= 0 && n2linksRowsIneq <= mzl);
   assert(local2linksStartEq >= nx0 + my0 && local2linksEndEq <= borderstartEq);
   assert(local2linksStartIneq >= nx0 + my0 + myl && local2linksEndIneq <= borderstartIneq);

   int nnzcount = 0;

   // dense square block, B_0^T, and dense border blocks todo: add space for CDCt
   for (int i = 0; i < nx0NonZero; ++i) {
      const int blength = startRowBtrans[i + 1] - startRowBtrans[i];
      assert(blength >= 0);

      krowM[i + 1] = krowM[i] + (nx0 - i) + blength + mylLocal + mzlLocal;

      appendRowDense(i, nx0, nnzcount, jcolM);
      appendRowSparse(startRowBtrans[i], startRowBtrans[i + 1], nx0, colidxBtrans, nnzcount, jcolM);

      appendRowDense(local2linksStartEq, local2linksEndEq, nnzcount, jcolM);
      appendRowDense(borderstartEq, borderstartEq + bordersizeEq, nnzcount, jcolM);
      appendRowDense(local2linksStartIneq, local2linksEndIneq, nnzcount, jcolM);
      appendRowDense(borderstartIneq, borderstartIneq + bordersizeIneq, nnzcount, jcolM);

      assert(nnzcount == krowM[i + 1]);
   }

   // dense square block and rest of B_0, F_0^T, G_0^T
   for (int i = nx0NonZero; i < nx0; ++i) {
      appendRowDense(i, nx0, nnzcount, jcolM);

      appendRowSparse(startRowBtrans[i], startRowBtrans[i + 1], nx0, colidxBtrans, nnzcount, jcolM);

      if (myl > 0) {
         const SparseMatrix& Ft = getLocalF().getTranspose();
         const int* startRowFtrans = Ft.krowM();
         const int* colidxFtrans = Ft.jcolM();

         appendRowSparse(startRowFtrans[i], startRowFtrans[i + 1], nx0 + my0, colidxFtrans, nnzcount, jcolM);
      }

      if (mzl > 0) {
         const SparseMatrix& Gt = getLocalG().getTranspose();
         const int* startRowGtrans = Gt.krowM();
         const int* colidxGtrans = Gt.jcolM();

         appendRowSparse(startRowGtrans[i], startRowGtrans[i + 1], nx0 + my0 + myl, colidxGtrans, nnzcount, jcolM);
      }

      krowM[i + 1] = nnzcount;
   }

   // empty rows; put diagonal for PARDISO
   for (int i = nx0; i < nx0 + my0; ++i) {
      const int rowStartIdx = krowM[i];

      jcolM[rowStartIdx] = i;
      krowM[i + 1] = rowStartIdx + 1;
   }

   nnzcount += my0;

   // equality linking: sparse diagonal blocks, and mixed rows
   int blockStartrow = 0;

   for (int i = nx0 + my0, j = 0, colIdxOffset = 0, blockStartrowMix = 0; i < nx0 + my0 + myl; ++i, ++j) {
      appendDiagBlocksDist(linkStartBlockIdA, linkStartBlockLengthsA, borderstartEq, bordersizeEq, i, j, blocksStart,
         blocksEnd, blockStartrow,
         nnzcount, jcolM);

      appendMixedBlocksDist(linkStartBlockIdA, linkStartBlockIdC, linkStartBlockLengthsA, linkStartBlockLengthsC,
         (nx0 + my0 + myl), bordersizeIneq,
         j, blocksStart, blocksEnd, colIdxOffset, blockStartrowMix, nnzcount, jcolM);

      assert(blockStartrowMix == blockStartrow);

      krowM[i + 1] = nnzcount;
   }

   // inequality linking: dense border block and sparse diagonal blocks

   blockStartrow = 0;

   for (int i = nx0 + my0 + myl, j = 0; i < nx0 + my0 + myl + mzl; ++i, ++j) {
      appendDiagBlocksDist(linkStartBlockIdC, linkStartBlockLengthsC, borderstartIneq, bordersizeIneq, i, j,
         blocksStart, blocksEnd, blockStartrow,
         nnzcount, jcolM);

      krowM[i + 1] = nnzcount;
   }

   assert(nnzcount == nnz);

   initDistMarker(blocksStart, blocksEnd);

   return std::make_unique<SparseSymmetricMatrix>(sizeSC, nnzcount, krowM, jcolM, M, 1, false);
}

Permutation DistributedProblem::get0VarsLastGlobalsFirstPermutation(std::vector<int>& link_vars_n_blocks, int& n_globals) {
   const size_t n_link_vars = link_vars_n_blocks.size();
   n_globals = 0;

   if (n_link_vars == 0)
      return Permutation();

   Permutation permvec(n_link_vars, 0);

   int count = 0;
   int back_count = n_link_vars - 1;

   for (size_t i = 0; i < n_link_vars; ++i) {
      assert(count <= back_count);
      assert(link_vars_n_blocks[i] >= -1);

      if (link_vars_n_blocks[i] > threshold_global_vars) {
         ++n_globals;
         permvec[count++] = i;
      } else if (link_vars_n_blocks[i] == 0) {
         permvec[back_count--] = i;
      }
   }

   if (threshold_global_vars >= -1) {
      for (size_t i = 0; i < n_link_vars; ++i) {
         if (link_vars_n_blocks[i] == -1 ||
            (link_vars_n_blocks[i] > 0 && link_vars_n_blocks[i] <= threshold_global_vars)) {
            assert(count <= back_count);
            permvec[count++] = i;
         }
      }
   }
   assert(count == back_count + 1);

   permuteVector(permvec, link_vars_n_blocks);

#ifndef NDEBUG
   int n_globals_copy = 0;
   int phase = 0;
   for (size_t i = 0; i < n_link_vars; ++i) {
      if (phase == 0) {
         if (link_vars_n_blocks[i] <= threshold_global_vars) {
            ++phase;
            --i;
         } else
            ++n_globals_copy;
      } else if (phase == 1) {
         if (link_vars_n_blocks[i] == 0) {
            ++phase;
            --i;
         } else {
            assert(0 < link_vars_n_blocks[i] || link_vars_n_blocks[i] == -1);
            assert(link_vars_n_blocks[i] <= threshold_global_vars);
         }
      } else if (phase == 2)
         assert(link_vars_n_blocks[i] == 0);
   }
   assert(n_globals_copy == n_globals);
#endif

   return permvec;
}

Permutation
DistributedProblem::getAscending2LinkFirstGlobalsLastPermutation(std::vector<int>& linkStartBlockId,
   std::vector<int>& n_blocks_per_row, size_t nBlocks,
   int& n_globals) {
   assert(linkStartBlockId.size() == n_blocks_per_row.size());
   const size_t n_links = linkStartBlockId.size();
   n_globals = 0;

   if (n_links == 0)
      return Permutation();

   Permutation permvec(n_links, 0);
   std::vector<int> w(nBlocks + 1, 0);

   /* count the 2-links per block - the ones starting at block -1 are no 2-links and are counted in w[0] */
   for (size_t i = 0; i < n_links; ++i) {
      assert(-1 <= linkStartBlockId[i]);
      assert(linkStartBlockId[i] < int(nBlocks));

      w[linkStartBlockId[i] + 1]++;
   }

   /* set w[i] to the amount of preceding 2-links, so the start of the 2-links starting in block i */
   size_t n_two_links = 0;
   for (size_t i = 1; i <= nBlocks; ++i) {
      n_two_links += w[i];
      w[i] = n_two_links;
   }

   assert(n_two_links + w[0] == n_links);
   w[0] = 0; // former n non 2-links

   /* sort 2-links ascending to front */
   for (size_t i = 0; i < n_links; ++i) {
      /* index of 2_link_start of nBlocks if not a 2 link */
      const int two_link_start = (linkStartBlockId[i] >= 0) ? linkStartBlockId[i] : int(nBlocks);

      assert(w[two_link_start] <= int(n_links));
      assert(permvec[w[two_link_start]] == 0);

      permvec[w[two_link_start]] = i;

      /* move start of i-2-link block */
      w[two_link_start]++;
   }

   /* permvec now moves 2-links ascending and the rest to the end */
   /* now permute global (long) linking constraints further to the end */
#ifndef NDEBUG
   for (size_t i = 1; i < n_two_links; i++)
      assert(linkStartBlockId[permvec[i - 1]] <= linkStartBlockId[permvec[i]]);
   for (size_t i = n_two_links; i < n_links; ++i)
      assert(linkStartBlockId[permvec[i]] == -1);
#endif

   /* got through non-2-links from front and back and swap all globals to the back */
   int front_pointer = n_two_links;
   assert(n_links > 0);
   int end_pointer = n_links - 1;

   while (front_pointer <= end_pointer) {
      assert(linkStartBlockId[permvec[front_pointer]] == -1);
      assert(linkStartBlockId[permvec[end_pointer]] == -1);

      /* blocks == -1 indicates a non-local 2 link */
      auto is_local_link = [](int blocks) {
         return (blocks <= threshold_global_cons && (blocks != -1 || !shave_nonlocal_2links));
      };

      /* keep link at front if it is not global and either not a 2 link or we don't shave the 2links */
      if (is_local_link(n_blocks_per_row[permvec[front_pointer]]))
         ++front_pointer;
      else if (!is_local_link(n_blocks_per_row[permvec[end_pointer]]))
         --end_pointer;
      else {
         assert(front_pointer < end_pointer);
         std::swap(permvec[end_pointer], permvec[front_pointer]);
         assert(n_blocks_per_row[permvec[front_pointer]] <= threshold_global_cons);
         assert(n_blocks_per_row[permvec[end_pointer]] > threshold_global_cons);

         ++front_pointer;
         --end_pointer;
      }
   }
   n_globals = n_links - front_pointer;

   assert(permutation_is_valid(permvec));

   permuteVector(permvec, n_blocks_per_row);
   permuteVector(permvec, linkStartBlockId);

#ifndef NDEBUG
   int phase = 0;
   int n_globals_copy = 0;
   for (size_t i = 0; i < linkStartBlockId.size(); ++i) {
      /* first ones are ascending 2-links */
      if (phase == 0) {
         if (linkStartBlockId[i] == -1) {
            ++phase;
            --i;
         } else {
            assert(n_blocks_per_row[i] == 2);
            if (i > 1)
               assert(linkStartBlockId[i - 1] <= linkStartBlockId[i]);
         }
      }
         /* n-links up to the threshold */
      else if (phase == 1) {
         if (n_blocks_per_row[i] > threshold_global_cons) {
            ++phase;
            --i;
         } else
            assert(linkStartBlockId[i] == -1);
      }
         /* global linking constraints */
      else {
         ++n_globals_copy;
         assert(n_blocks_per_row[i] > threshold_global_cons);
      }
   }
   assert(n_globals == n_globals_copy);
#endif

   return permvec;
}

DistributedProblem::DistributedProblem(const DistributedTree* tree_, std::shared_ptr<Vector<double>> c_in,
   std::shared_ptr<SymmetricMatrix> Q_in, std::shared_ptr<Vector<double>> xlow_in,
   std::shared_ptr<Vector<double>> ixlow_in, std::shared_ptr<Vector<double>> xupp_in,
   std::shared_ptr<Vector<double>> ixupp_in, std::shared_ptr<GeneralMatrix> A_in, std::shared_ptr<Vector<double>> bA_in,
   std::shared_ptr<GeneralMatrix> C_in, std::shared_ptr<Vector<double>> clow_in,
   std::shared_ptr<Vector<double>> iclow_in,
   std::shared_ptr<Vector<double>> cupp_in,
   std::shared_ptr<Vector<double>> icupp_in, bool add_children, bool is_hierarchy_root,
   bool is_hierarchy_inner_root, bool is_hierarchy_inner_leaf) :
   Problem(std::move(c_in), std::move(Q_in),
   std::move(xlow_in), std::move(ixlow_in),
   std::move(xupp_in), std::move(ixupp_in), std::move(A_in), std::move(bA_in), std::move(C_in), std::move(clow_in),
   std::move(iclow_in), std::move(cupp_in), std::move(icupp_in)), stochNode{tree_},
   is_hierarchy_root{is_hierarchy_root},
   is_hierarchy_inner_root{is_hierarchy_inner_root}, is_hierarchy_inner_leaf{is_hierarchy_inner_leaf} {

   if (add_children)
      createChildren();
}

void DistributedProblem::write_to_streamDense(std::ostream& out) const {
   const int myRank = PIPS_MPIgetRank(MPI_COMM_WORLD);

   if (myRank == 0)
      out << "A:\n";
   (*equality_jacobian).write_to_streamDense(out);
   if (myRank == 0)
      out << "C:\n";
   (*inequality_jacobian).write_to_streamDense(out);
   if (myRank == 0)
      out << "obj:\n";
   (*objective_gradient).write_to_stream(out);
   if (myRank == 0)
      out << "bA:\n";
   (*equality_rhs).write_to_stream(out);
   if (myRank == 0)
      out << "xupp:\n";
   (*primal_upper_bounds).write_to_stream(out);
   if (myRank == 0)
      out << "ixupp:\n";
   (*primal_upper_bound_indicators).write_to_stream(out);
   if (myRank == 0)
      out << "xlow:\n";
   (*primal_lower_bounds).write_to_stream(out);
   if (myRank == 0)
      out << "ixlow:\n";
   (*primal_lower_bound_indicators).write_to_stream(out);
   if (myRank == 0)
      out << "cupp:\n";
   (*inequality_upper_bounds).write_to_stream(out);
   if (myRank == 0)
      out << "icupp:\n";
   (*inequality_upper_bound_indicators).write_to_stream(out);
   if (myRank == 0)
      out << "clow:\n";
   (*inequality_lower_bounds).write_to_stream(out);
   if (myRank == 0)
      out << "iclow:\n";
   (*inequality_lower_bound_indicators).write_to_stream(out);
}

std::unique_ptr<DistributedProblem> DistributedProblem::clone_full(bool switchToDynamicStorage) const {
   // todo Q is empty!
   std::shared_ptr<SymmetricMatrix> Q_clone(hessian->clone());
   std::shared_ptr<GeneralMatrix> A_clone(dynamic_cast<const DistributedMatrix&>(*equality_jacobian).clone_full(switchToDynamicStorage));
   std::shared_ptr<GeneralMatrix> C_clone(dynamic_cast<const DistributedMatrix&>(*inequality_jacobian).clone_full(switchToDynamicStorage));

   std::shared_ptr<DistributedVector<double>> c_clone(dynamic_cast<DistributedVector<double>*>(objective_gradient->clone_full()));
   std::shared_ptr<DistributedVector<double>> bA_clone(dynamic_cast<DistributedVector<double>*>(equality_rhs->clone_full()));
   std::shared_ptr<DistributedVector<double>> xupp_clone(dynamic_cast<DistributedVector<double>*>(primal_upper_bounds->clone_full()));
   std::shared_ptr<DistributedVector<double>> ixupp_clone(dynamic_cast<DistributedVector<double>*>(primal_upper_bound_indicators->clone_full()));
   std::shared_ptr<DistributedVector<double>> xlow_clone(dynamic_cast<DistributedVector<double>*>(primal_lower_bounds->clone_full()));
   std::shared_ptr<DistributedVector<double>> ixlow_clone(dynamic_cast<DistributedVector<double>*>(primal_lower_bound_indicators->clone_full()));
   std::shared_ptr<DistributedVector<double>> cupp_clone(dynamic_cast<DistributedVector<double>*>(inequality_upper_bounds->clone_full()));
   std::shared_ptr<DistributedVector<double>> icupp_clone(dynamic_cast<DistributedVector<double>*>(inequality_upper_bound_indicators->clone_full()));
   std::shared_ptr<DistributedVector<double>> clow_clone(dynamic_cast<DistributedVector<double>*>(inequality_lower_bounds->clone_full()));
   std::shared_ptr<DistributedVector<double>> iclow_clone(dynamic_cast<DistributedVector<double>*>(inequality_lower_bound_indicators->clone_full()));

   // TODO : tree is not actually cloned..
   const DistributedTree* tree_clone = stochNode;

   // TODO : proper copy ctor..
   return std::make_unique<DistributedProblem>(tree_clone, std::move(c_clone), std::move(Q_clone), std::move(xlow_clone),
      std::move(ixlow_clone), std::move(xupp_clone), std::move(ixupp_clone),
      std::move(A_clone), std::move(bA_clone),
      std::move(C_clone), std::move(clow_clone), std::move(iclow_clone), std::move(cupp_clone), std::move(icupp_clone));
}

void DistributedProblem::createChildren() {
   //follow the structure of one of the tree objects and create the same
   //structure for this class, and link this object with the corresponding
   //vectors and matrices
   auto& gSt = dynamic_cast<DistributedVector<double>&>(*objective_gradient);
   auto& QSt = dynamic_cast<DistributedSymmetricMatrix&>(*hessian);

   auto& xlowSt = dynamic_cast<DistributedVector<double>&>(*primal_lower_bounds);
   auto& ixlowSt = dynamic_cast<DistributedVector<double>&>(*primal_lower_bound_indicators);
   auto& xuppSt = dynamic_cast<DistributedVector<double>&>(*primal_upper_bounds);
   auto& ixuppSt = dynamic_cast<DistributedVector<double>&>(*primal_upper_bound_indicators);
   auto& ASt = dynamic_cast<DistributedMatrix&>(*equality_jacobian);
   auto& bASt = dynamic_cast<DistributedVector<double>&>(*equality_rhs);
   auto& CSt = dynamic_cast<DistributedMatrix&>(*inequality_jacobian);
   auto& clowSt = dynamic_cast<DistributedVector<double>&>(*inequality_lower_bounds);
   auto& iclowSt = dynamic_cast<DistributedVector<double>&>(*inequality_lower_bound_indicators);
   auto& cuppSt = dynamic_cast<DistributedVector<double>&>(*inequality_upper_bounds);
   auto& icuppSt = dynamic_cast<DistributedVector<double>&>(*inequality_upper_bound_indicators);

   for (size_t it = 0; it < gSt.children.size(); it++) {
      add_child(new DistributedProblem(stochNode->getChildren()[it].get(), gSt.children[it], QSt.children[it], xlowSt.children[it],
         ixlowSt.children[it],
         xuppSt.children[it], ixuppSt.children[it], ASt.children[it], bASt.children[it], CSt.children[it],
         clowSt.children[it],
         iclowSt.children[it], cuppSt.children[it], icuppSt.children[it]));
   }
}

void DistributedProblem::destroyChildren() {
   children.clear();
}

DistributedProblem* DistributedProblem::shaveBorderFromDataAndCreateNewTop(const DistributedTree& tree) {
   assert(tree.nChildren() == 1);
   std::unique_ptr<SymmetricMatrix> Q_hier(
      dynamic_cast<DistributedSymmetricMatrix&>(*hessian).raiseBorder(n_global_linking_vars));

   std::unique_ptr<BorderedMatrix> A_hier(
      dynamic_cast<DistributedMatrix&>(*equality_jacobian).raiseBorder(n_global_eq_linking_conss, n_global_linking_vars));
   if (pipsipmpp_options::get_bool_parameter("HIERARCHICAL_MOVE_A0_TO_DENSE_LAYER")) {
      A_hier = std::make_unique<BorderedMatrixLiftedA0wrapper>(std::move(A_hier));
   }
   std::shared_ptr<GeneralMatrix> C_hier(
      dynamic_cast<DistributedMatrix&>(*inequality_jacobian).raiseBorder(n_global_ineq_linking_conss, n_global_linking_vars));

   /* we ordered global linking vars first and global linking rows to the end */
   std::shared_ptr<DistributedVector<double>> g_hier(
      dynamic_cast<DistributedVector<double>&>(*objective_gradient).raiseBorder(n_global_linking_vars, -1));
   std::shared_ptr<DistributedVector<double>> bux_hier(
      dynamic_cast<DistributedVector<double>&>(*primal_upper_bounds).raiseBorder(n_global_linking_vars, -1));
   std::shared_ptr<DistributedVector<double>> ixupp_hier(
      dynamic_cast<DistributedVector<double>&>(*primal_upper_bound_indicators).raiseBorder(n_global_linking_vars, -1));
   std::shared_ptr<DistributedVector<double>> blx_hier(
      dynamic_cast<DistributedVector<double>&>(*primal_lower_bounds).raiseBorder(n_global_linking_vars, -1));
   std::shared_ptr<DistributedVector<double>> ixlow_hier(
      dynamic_cast<DistributedVector<double>&>(*primal_lower_bound_indicators).raiseBorder(n_global_linking_vars, -1));

   std::shared_ptr<DistributedVector<double>> bA_hier(
      dynamic_cast<DistributedVector<double>&>(*equality_rhs).raiseBorder(-1, n_global_eq_linking_conss));
   if (pipsipmpp_options::get_bool_parameter("HIERARCHICAL_MOVE_A0_TO_DENSE_LAYER")) {
      bA_hier->children[0]->move_first_to_parent();
   }

   std::shared_ptr<DistributedVector<double>> bu_hier(
      dynamic_cast<DistributedVector<double>&>(*inequality_upper_bounds).raiseBorder(-1, n_global_ineq_linking_conss));
   std::shared_ptr<DistributedVector<double>> icupp_hier(
      dynamic_cast<DistributedVector<double>&>(*inequality_upper_bound_indicators).raiseBorder(-1, n_global_ineq_linking_conss));
   std::shared_ptr<DistributedVector<double>> bl_hier(
      dynamic_cast<DistributedVector<double>&>(*inequality_lower_bounds).raiseBorder(-1, n_global_ineq_linking_conss));
   std::shared_ptr<DistributedVector<double>> iclow_hier(
      dynamic_cast<DistributedVector<double>&>(*inequality_lower_bound_indicators).raiseBorder(-1, n_global_ineq_linking_conss));

   // TODO what is this?
   //DistributedVector<double>* sc_hier = dynamic_cast<DistributedVector<double>&>(*sc).shaveBorder(-1);

   return new DistributedProblem(&tree, std::move(g_hier), std::move(Q_hier), std::move(blx_hier), std::move(ixlow_hier), std::move(bux_hier), std::move(ixupp_hier), std::move(A_hier), std::move(bA_hier), std::move(C_hier),
                  std::move(bl_hier), std::move(iclow_hier), std::move(bu_hier),
                  std::move(icupp_hier), false, true);
}

DistributedProblem* DistributedProblem::shaveDenseBorder(const DistributedTree& tree) {
   DistributedProblem* hierarchical_top = shaveBorderFromDataAndCreateNewTop(tree);

   const auto& ixlow = dynamic_cast<const DistributedVector<double>&>(*hierarchical_top->primal_lower_bound_indicators);
   const auto& ixupp = dynamic_cast<const DistributedVector<double>&>(*hierarchical_top->primal_upper_bound_indicators);
   assert(ixlow.first);
   assert(ixupp.first);
   number_primal_lower_bounds -= ixlow.first->number_nonzeros();
   number_primal_upper_bounds -= ixupp.first->number_nonzeros();

   const auto& iclow = dynamic_cast<const DistributedVector<double>&>(*hierarchical_top->inequality_lower_bound_indicators);
   const auto& icupp = dynamic_cast<const DistributedVector<double>&>(*hierarchical_top->inequality_upper_bound_indicators);
   assert(iclow.last);
   assert(icupp.last);
   number_inequality_lower_bounds -= iclow.last->number_nonzeros();
   number_inequality_upper_bounds -= icupp.last->number_nonzeros();

   nx = objective_gradient->length();
   my = equality_jacobian->n_rows();
   mz = inequality_jacobian->n_rows();

   /* adapt vectors and global link sizes - we pushed these up */

   /* global linking variables have been ordered to the front, global linking constraints to the end of the matrices */
   /* linking vars */
   hierarchical_top->n_blocks_per_link_var = this->n_blocks_per_link_var;
   this->n_blocks_per_link_var.erase(n_blocks_per_link_var.begin(),
      n_blocks_per_link_var.begin() + n_global_linking_vars);

   /* Amat linking cons */
   hierarchical_top->linkStartBlockIdA = this->linkStartBlockIdA;
   this->linkStartBlockIdA.erase(linkStartBlockIdA.end() - n_global_eq_linking_conss, linkStartBlockIdA.end());
   hierarchical_top->n_blocks_per_link_row_A = this->n_blocks_per_link_row_A;
   this->n_blocks_per_link_row_A.erase(n_blocks_per_link_row_A.end() - n_global_eq_linking_conss,
      n_blocks_per_link_row_A.end());

   /* Cmat linking cons */
   hierarchical_top->linkStartBlockIdC = this->linkStartBlockIdC;
   this->linkStartBlockIdC.erase(linkStartBlockIdC.end() - n_global_ineq_linking_conss, linkStartBlockIdC.end());
   hierarchical_top->n_blocks_per_link_row_C = this->n_blocks_per_link_row_C;
   this->n_blocks_per_link_row_C.erase(n_blocks_per_link_row_C.end() - n_global_ineq_linking_conss,
      n_blocks_per_link_row_C.end());

   assert(isSCrowLocal.empty());
   assert(isSCrowMyLocal.empty());

   hierarchical_top->n_global_eq_linking_conss = n_global_eq_linking_conss;
   this->n_global_eq_linking_conss = 0;

   hierarchical_top->n_global_ineq_linking_conss = n_global_ineq_linking_conss;
   this->n_global_ineq_linking_conss = 0;

   hierarchical_top->n_global_linking_vars = n_global_linking_vars;
   this->n_global_linking_vars = 0;

   hierarchical_top->useLinkStructure = false;

   hierarchical_top->children.push_back(this);
   stochNode = tree.getChildren()[0].get();

   return hierarchical_top;
}

Permutation
DistributedProblem::getChildLinkConsFirstOwnLinkConsLastPermutation(const std::vector<unsigned int>& map_block_subtree,
   const std::vector<int>& linkStartBlockId, int n_links_after_split) {
   /* assuming that global links have already been ordered last */
   Permutation perm(linkStartBlockId.size());

   assert(n_links_after_split >= 0);

#ifndef NDEBUG
   int last_map_block = -1;
#endif

   size_t pos_child_twolinks = 0;
   const auto end_child_twolinks = static_cast<size_t>(linkStartBlockId.size() - n_links_after_split);
   size_t pos_remaining_links = end_child_twolinks;

   for (size_t i = 0; i < linkStartBlockId.size(); ++i) {
      assert(last_map_block <= linkStartBlockId[i]);

      /* we arrived at the global links which will all stay at this node */
      if (linkStartBlockId[i] == -1) {
         assert(pos_child_twolinks == end_child_twolinks);
         perm[pos_remaining_links] = i;
         ++pos_remaining_links;
      } else {
         assert(0 <= linkStartBlockId[i]);
         const auto start_block_link_i = static_cast<size_t>(linkStartBlockId[i]);
         assert(start_block_link_i < map_block_subtree.size());

         if (start_block_link_i == map_block_subtree.size() - 1) {
            perm[pos_child_twolinks] = i;
            ++pos_child_twolinks;
         } else if (map_block_subtree[start_block_link_i] != map_block_subtree[start_block_link_i + 1]) {
            perm[pos_remaining_links] = i;
            ++pos_remaining_links;
         } else {
            assert(map_block_subtree[start_block_link_i] == map_block_subtree[start_block_link_i + 1]);
            perm[pos_child_twolinks] = i;
            ++pos_child_twolinks;
         }
      }
   }
   assert(pos_child_twolinks == end_child_twolinks);
   assert(pos_remaining_links == linkStartBlockId.size());
   assert(permutation_is_valid(perm));
   return perm;
}

void DistributedProblem::reorderLinkingConstraintsAccordingToSplit() {
   /* assert that distributed Schur complement has not yet been initialized */
   assert(isSCrowLocal.empty());
   assert(isSCrowMyLocal.empty());

   const std::vector<unsigned int>& map_block_subtree = dynamic_cast<const DistributedTreeCallbacks*>(stochNode)->getMapBlockSubTrees();

   Permutation perm_A = getChildLinkConsFirstOwnLinkConsLastPermutation(map_block_subtree, linkStartBlockIdA,
      stochNode->myl());
   Permutation perm_C = getChildLinkConsFirstOwnLinkConsLastPermutation(map_block_subtree, linkStartBlockIdC,
      stochNode->mzl());

   /* which blocks do the individual two-links start in */
   permuteLinkingCons(perm_A, perm_C);
   permuteLinkStructureDetection(perm_A, perm_C);
}

void DistributedProblem::addChildrenForSplit() {
   if (stochNode->isHierarchicalInnerLeaf())
      is_hierarchy_inner_leaf = true;
   else
      is_hierarchy_inner_root = true;

   assert(isSCrowLocal.empty());
   assert(isSCrowMyLocal.empty());

   const std::vector<unsigned int>& map_blocks_children = dynamic_cast<const DistributedTreeCallbacks*>(stochNode)->getMapBlockSubTrees();
   const unsigned int n_new_children = getNDistinctValues(map_blocks_children);

   const auto& tree = dynamic_cast<const DistributedTreeCallbacks&>(*stochNode);
   std::vector<DistributedProblem*> new_children(n_new_children);

   unsigned int childchild_pos{0};
   for (unsigned int i = 0; i < n_new_children; ++i) {
      std::shared_ptr<DistributedSymmetricMatrix> Q_child = is_hierarchy_inner_root
         ? dynamic_cast<DistributedSymmetricMatrix&>(*hessian).children[i]
         : dynamic_cast<DistributedSymmetricMatrix&>(*dynamic_cast<DistributedSymmetricMatrix&>(*hessian).diag).children[i];

      std::shared_ptr<DistributedMatrix> A_child = is_hierarchy_inner_root ? dynamic_cast<DistributedMatrix&>(*equality_jacobian).children[i]
         : dynamic_cast<DistributedMatrix&>(*dynamic_cast<DistributedMatrix&>(*equality_jacobian).Bmat).children[i];
      std::shared_ptr<DistributedMatrix> C_child = is_hierarchy_inner_root ? dynamic_cast<DistributedMatrix&>(*inequality_jacobian).children[i]
         : dynamic_cast<DistributedMatrix&>(*dynamic_cast<DistributedMatrix&>(*inequality_jacobian).Bmat).children[i];

      std::shared_ptr<DistributedVector<double>> g_child = is_hierarchy_inner_root
         ? dynamic_cast<DistributedVector<double>&>(*objective_gradient).children[i]
         : dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*objective_gradient).first).children[i];
      std::shared_ptr<DistributedVector<double>> blx_child = is_hierarchy_inner_root
         ? dynamic_cast<DistributedVector<double>&>(*primal_lower_bounds).children[i]
         : dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*primal_lower_bounds).first).children[i];
      std::shared_ptr<DistributedVector<double>> ixlow_child = is_hierarchy_inner_root
         ? dynamic_cast<DistributedVector<double>&>(*primal_lower_bound_indicators).children[i]
         : dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*primal_lower_bound_indicators).first).children[i];
      std::shared_ptr<DistributedVector<double>> bux_child = is_hierarchy_inner_root
         ? dynamic_cast<DistributedVector<double>&>(*primal_upper_bounds).children[i]
         : dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*primal_upper_bounds).first).children[i];
      std::shared_ptr<DistributedVector<double>> ixupp_child = is_hierarchy_inner_root
         ? dynamic_cast<DistributedVector<double>&>(*primal_upper_bound_indicators).children[i]
         : dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*primal_upper_bound_indicators).first).children[i];

      std::shared_ptr<DistributedVector<double>> bA_child = is_hierarchy_inner_root
         ? dynamic_cast<DistributedVector<double>&>(*equality_rhs).children[i]
         : dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*equality_rhs).first).children[i];

      std::shared_ptr<DistributedVector<double>> bl_child = is_hierarchy_inner_root
         ? dynamic_cast<DistributedVector<double>&>(*inequality_lower_bounds).children[i]
         : dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*inequality_lower_bounds).first).children[i];
      std::shared_ptr<DistributedVector<double>> iclow_child = is_hierarchy_inner_root
         ? dynamic_cast<DistributedVector<double>&>(*inequality_lower_bound_indicators).children[i]
         : dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*inequality_lower_bound_indicators).first).children[i];
      std::shared_ptr<DistributedVector<double>> bu_child = is_hierarchy_inner_root
         ? dynamic_cast<DistributedVector<double>&>(*inequality_upper_bounds).children[i]
         : dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*inequality_upper_bounds).first).children[i];
      std::shared_ptr<DistributedVector<double>> icupp_child = is_hierarchy_inner_root
         ? dynamic_cast<DistributedVector<double>&>(*inequality_upper_bound_indicators).children[i]
         : dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*inequality_upper_bound_indicators).first).children[i];

      assert(dynamic_cast<const DistributedTreeCallbacks&>(*tree.getChildren()[i]).isHierarchicalInnerLeaf());
      const DistributedTree* tree_child = dynamic_cast<const DistributedTreeCallbacks&>(*tree.getChildren()[i]).getSubRoot();

      auto* child = new DistributedProblem(tree_child, g_child, Q_child, blx_child, ixlow_child, bux_child, ixupp_child,
         A_child, bA_child,
         C_child, bl_child, iclow_child, bu_child, icupp_child, false, false, false, true);
      new_children[i] = child;

      const int myl = tree_child->myl();
      const int mzl = tree_child->mzl();

      child->linkConsPermutationA.resize(myl);
      std::iota(child->linkConsPermutationA.begin(), child->linkConsPermutationA.end(), 0);

      child->linkConsPermutationC.resize(mzl);
      std::iota(child->linkConsPermutationC.begin(), child->linkConsPermutationC.end(), 0);

      /// A
      child->linkStartBlockIdA.insert(child->linkStartBlockIdA.begin(), linkStartBlockIdA.begin(),
         linkStartBlockIdA.begin() + myl);
      std::transform(child->linkStartBlockIdA.begin(), child->linkStartBlockIdA.end(), child->linkStartBlockIdA.begin(),
         [&childchild_pos](const int& a) { return a - childchild_pos; });

      child->n_blocks_per_link_row_A.insert(child->n_blocks_per_link_row_A.begin(), n_blocks_per_link_row_A.begin(),
         n_blocks_per_link_row_A.begin() + myl);
      n_blocks_per_link_row_A.erase(n_blocks_per_link_row_A.begin(), n_blocks_per_link_row_A.begin() + myl);

      /// C
      child->linkStartBlockIdC.insert(child->linkStartBlockIdC.begin(), linkStartBlockIdC.begin(),
         linkStartBlockIdC.begin() + mzl);
      std::transform(child->linkStartBlockIdC.begin(), child->linkStartBlockIdC.end(), child->linkStartBlockIdC.begin(),
         [&childchild_pos](const int& a) { return a - childchild_pos; });

      child->n_blocks_per_link_row_C.insert(child->n_blocks_per_link_row_C.begin(), n_blocks_per_link_row_C.begin(),
         n_blocks_per_link_row_C.begin() + mzl);
      n_blocks_per_link_row_C.erase(n_blocks_per_link_row_C.begin(), n_blocks_per_link_row_C.begin() + mzl);

      const int first_child = childchild_pos;
      while (childchild_pos < map_blocks_children.size() && map_blocks_children[childchild_pos] == i) {
         children[childchild_pos]->has_RAC = false;
         child->add_child(children[childchild_pos]);

         if (childchild_pos + 1 == map_blocks_children.size() || map_blocks_children[childchild_pos + 1] != i) {
            child->linkStartBlockLengthsA.push_back(0);
            child->linkStartBlockLengthsC.push_back(0);
         } else {
            child->linkStartBlockLengthsA.push_back(linkStartBlockLengthsA[childchild_pos]);
            linkStartBlockLengthsA[childchild_pos] = -20;

            child->linkStartBlockLengthsC.push_back(linkStartBlockLengthsC[childchild_pos]);
            linkStartBlockLengthsC[childchild_pos] = -20;
         }
         ++childchild_pos;
      }
      const int last_child = childchild_pos;

      int eq_to_erase{0};
      if (!linkStartBlockIdA.empty())
         while (first_child <= *(linkStartBlockIdA.begin() + eq_to_erase) &&
            *(linkStartBlockIdA.begin() + eq_to_erase) < last_child)
            ++eq_to_erase;

      int ineq_to_erase{0};
      if (!linkStartBlockIdC.empty())
         while (first_child <= *(linkStartBlockIdC.begin() + ineq_to_erase) &&
            *(linkStartBlockIdC.begin() + ineq_to_erase) < last_child)
            ++ineq_to_erase;
      assert(myl == 0 || myl == eq_to_erase);
      assert(mzl == 0 || mzl == ineq_to_erase);

      linkStartBlockIdA.erase(linkStartBlockIdA.begin(), linkStartBlockIdA.begin() + eq_to_erase);
      linkStartBlockIdC.erase(linkStartBlockIdC.begin(), linkStartBlockIdC.begin() + ineq_to_erase);

      assert(child->linkStartBlockLengthsA.size() == child->children.size());
      assert(child->linkStartBlockLengthsA.back() == 0);

      // Leaving child->linkVarsPermutation, child->n_blocks_per_link_var empty for now - not sure if ever needed

      child->useLinkStructure = true;
   }

   linkStartBlockLengthsA.erase(
      std::remove_if(linkStartBlockLengthsA.begin(), linkStartBlockLengthsA.end(), [](int a) { return a == -20; }),
      linkStartBlockLengthsA.end());

   linkStartBlockLengthsC.erase(
      std::remove_if(linkStartBlockLengthsC.begin(), linkStartBlockLengthsC.end(), [](int a) { return a == -20; }),
      linkStartBlockLengthsC.end());

   for (int& i : linkStartBlockIdA)
      if (i >= 0)
         i = map_blocks_children[i];

   for (int& i : linkStartBlockIdC)
      if (i >= 0)
         i = map_blocks_children[i];

   children.clear();
   children.insert(children.begin(), new_children.begin(), new_children.end());

   assert(linkStartBlockLengthsA.size() == linkStartBlockLengthsC.size());
   assert(linkStartBlockLengthsA.size() == new_children.size());
}

void DistributedProblem::splitData() {
   const std::vector<unsigned int>& map_block_subtree = dynamic_cast<const DistributedTreeCallbacks*>(stochNode)->getMapBlockSubTrees();
   const std::vector<MPI_Comm> child_comms = dynamic_cast<const DistributedTreeCallbacks*>(stochNode)->getChildComms();
   assert(child_comms.size() == getNDistinctValues(map_block_subtree));

   if (stochNode->isHierarchicalInnerLeaf()) {
      dynamic_cast<DistributedSymmetricMatrix&>(*dynamic_cast<DistributedSymmetricMatrix&>(*hessian).diag).splitMatrix(
         map_block_subtree, child_comms);
      dynamic_cast<DistributedMatrix&>(*dynamic_cast<DistributedMatrix&>(*equality_jacobian).Bmat).splitMatrix(linkStartBlockLengthsA,
         map_block_subtree, stochNode->myl(),
         child_comms);
      dynamic_cast<DistributedMatrix&>(*dynamic_cast<DistributedMatrix&>(*inequality_jacobian).Bmat).splitMatrix(linkStartBlockLengthsC,
         map_block_subtree, stochNode->mzl(),
         child_comms);

      dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*objective_gradient).first).split(
         map_block_subtree, child_comms);

      dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*primal_upper_bounds).first).split(
         map_block_subtree, child_comms);
      dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*primal_upper_bound_indicators).first).split(
         map_block_subtree, child_comms);
      dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*primal_lower_bounds).first).split(
         map_block_subtree, child_comms);
      dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*primal_lower_bound_indicators).first).split(
         map_block_subtree, child_comms);

      dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*equality_rhs).first).split(
         map_block_subtree, child_comms,
         linkStartBlockLengthsA, stochNode->myl());

      dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*inequality_upper_bounds).first).split(
         map_block_subtree, child_comms,
         linkStartBlockLengthsC, stochNode->mzl());
      dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*inequality_upper_bound_indicators).first).split(
         map_block_subtree, child_comms,
         linkStartBlockLengthsC, stochNode->mzl());
      dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*inequality_lower_bounds).first).split(
         map_block_subtree, child_comms,
         linkStartBlockLengthsC, stochNode->mzl());
      dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*inequality_lower_bound_indicators).first).split(
         map_block_subtree, child_comms,
         linkStartBlockLengthsC, stochNode->mzl());
   } else {
      dynamic_cast<DistributedSymmetricMatrix&>(*hessian).splitMatrix(map_block_subtree, child_comms);
      dynamic_cast<DistributedMatrix&>(*equality_jacobian).splitMatrix(linkStartBlockLengthsA, map_block_subtree, stochNode->myl(),
         child_comms);
      dynamic_cast<DistributedMatrix&>(*inequality_jacobian).splitMatrix(linkStartBlockLengthsC, map_block_subtree, stochNode->mzl(),
         child_comms);

      dynamic_cast<DistributedVector<double>&>(*objective_gradient).split(map_block_subtree, child_comms);

      dynamic_cast<DistributedVector<double>&>(*primal_upper_bounds).split(map_block_subtree, child_comms);
      dynamic_cast<DistributedVector<double>&>(*primal_upper_bound_indicators).split(map_block_subtree, child_comms);
      dynamic_cast<DistributedVector<double>&>(*primal_lower_bounds).split(map_block_subtree, child_comms);
      dynamic_cast<DistributedVector<double>&>(*primal_lower_bound_indicators).split(map_block_subtree, child_comms);

      dynamic_cast<DistributedVector<double>&>(*equality_rhs).split(map_block_subtree, child_comms, linkStartBlockLengthsA,
         stochNode->myl());

      dynamic_cast<DistributedVector<double>&>(*inequality_upper_bounds).split(map_block_subtree, child_comms, linkStartBlockLengthsC,
         stochNode->mzl());
      dynamic_cast<DistributedVector<double>&>(*inequality_upper_bound_indicators).split(map_block_subtree, child_comms, linkStartBlockLengthsC,
         stochNode->mzl());
      dynamic_cast<DistributedVector<double>&>(*inequality_lower_bounds).split(map_block_subtree, child_comms, linkStartBlockLengthsC,
         stochNode->mzl());
      dynamic_cast<DistributedVector<double>&>(*inequality_lower_bound_indicators).split(map_block_subtree, child_comms, linkStartBlockLengthsC,
         stochNode->mzl());
   }
}

void DistributedProblem::recomputeSize() {
   dynamic_cast<DistributedSymmetricMatrix&>(*hessian).recomputeSize();
   dynamic_cast<DistributedMatrix&>(*equality_jacobian).recomputeSize();
   dynamic_cast<DistributedMatrix&>(*inequality_jacobian).recomputeSize();

   dynamic_cast<DistributedVector<double>&>(*objective_gradient).recomputeSize();

   dynamic_cast<DistributedVector<double>&>(*primal_upper_bounds).recomputeSize();
   dynamic_cast<DistributedVector<double>&>(*primal_upper_bound_indicators).recomputeSize();
   dynamic_cast<DistributedVector<double>&>(*primal_lower_bounds).recomputeSize();
   dynamic_cast<DistributedVector<double>&>(*primal_lower_bound_indicators).recomputeSize();

   dynamic_cast<DistributedVector<double>&>(*equality_rhs).recomputeSize();

   dynamic_cast<DistributedVector<double>&>(*inequality_upper_bounds).recomputeSize();
   dynamic_cast<DistributedVector<double>&>(*inequality_upper_bound_indicators).recomputeSize();
   dynamic_cast<DistributedVector<double>&>(*inequality_lower_bounds).recomputeSize();
   dynamic_cast<DistributedVector<double>&>(*inequality_lower_bound_indicators).recomputeSize();
}

void DistributedProblem::splitStringMatricesAccordingToSubtreeStructure() {
   assert(dynamic_cast<DistributedMatrix&>(*equality_jacobian).Blmat->is_a(kStripMatrix));
   assert(dynamic_cast<DistributedMatrix&>(*inequality_jacobian).Blmat->is_a(kStripMatrix));
   auto& Blmat = dynamic_cast<StripMatrix&>(*dynamic_cast<DistributedMatrix&>(*equality_jacobian).Blmat);
   auto& Dlmat = dynamic_cast<StripMatrix&>(*dynamic_cast<DistributedMatrix&>(*inequality_jacobian).Blmat);

   if (stochNode->getCommWorkers() == MPI_COMM_NULL) {
      assert(Blmat.is_a(kStringGenDummyMatrix));
      assert(Dlmat.is_a(kStringGenDummyMatrix));
      return;
   }

   Blmat.splitAlongTree(dynamic_cast<const DistributedTreeCallbacks&>(*stochNode));
   Dlmat.splitAlongTree(dynamic_cast<const DistributedTreeCallbacks&>(*stochNode));

   assert(children.size() == Blmat.children.size());
   assert(children.size() == Dlmat.children.size());
}


void DistributedProblem::splitDataAndAddAsChildLayer() {
   splitData();
   addChildrenForSplit();
}

void DistributedProblem::splitDataAccordingToTree() {
   /* we came to a leaf and stop here */
   if (!stochNode->isHierarchicalInnerRoot() && !stochNode->isHierarchicalInnerLeaf())
      return;

   reorderLinkingConstraintsAccordingToSplit();
   splitDataAndAddAsChildLayer();
}

void DistributedProblem::permuteLinkStructureDetection(const Permutation& perm_A, const Permutation& perm_C) {
   assert(isSCrowLocal.empty());
   assert(isSCrowMyLocal.empty());

   permuteVector(perm_A, linkStartBlockIdA);
   permuteVector(perm_A, n_blocks_per_link_row_A);

   permuteVector(perm_C, linkStartBlockIdC);
   permuteVector(perm_C, n_blocks_per_link_row_C);

   permuteVector(perm_A, linkConsPermutationA);
   permuteVector(perm_C, linkConsPermutationC);
}

void DistributedProblem::permuteLinkingCons(const Permutation& permA, const Permutation& permC) {
   assert(permutation_is_valid(permA));
   assert(permutation_is_valid(permC));
   assert(!is_hierarchy_root);

   if (stochNode->isHierarchicalInnerLeaf()) {
      dynamic_cast<DistributedMatrix&>(*dynamic_cast<DistributedMatrix&>(*equality_jacobian).Bmat).permuteLinkingCons(permA);
      dynamic_cast<DistributedMatrix&>(*dynamic_cast<DistributedMatrix&>(*inequality_jacobian).Bmat).permuteLinkingCons(permC);
      dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*equality_rhs).first).permuteLinkingEntries(
         permA);
      dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*inequality_lower_bounds).first).permuteLinkingEntries(
         permC);
      dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*inequality_upper_bounds).first).permuteLinkingEntries(
         permC);
      dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*inequality_lower_bound_indicators).first).permuteLinkingEntries(
         permC);
      dynamic_cast<DistributedVector<double>&>(*dynamic_cast<DistributedVector<double>&>(*inequality_upper_bound_indicators).first).permuteLinkingEntries(
         permC);
   } else {
      dynamic_cast<DistributedMatrix&>(*equality_jacobian).permuteLinkingCons(permA);
      dynamic_cast<DistributedMatrix&>(*inequality_jacobian).permuteLinkingCons(permC);
      dynamic_cast<DistributedVector<double>&>(*equality_rhs).permuteLinkingEntries(permA);
      dynamic_cast<DistributedVector<double>&>(*inequality_lower_bounds).permuteLinkingEntries(permC);
      dynamic_cast<DistributedVector<double>&>(*inequality_upper_bounds).permuteLinkingEntries(permC);
      dynamic_cast<DistributedVector<double>&>(*inequality_lower_bound_indicators).permuteLinkingEntries(permC);
      dynamic_cast<DistributedVector<double>&>(*inequality_upper_bound_indicators).permuteLinkingEntries(permC);
   }
}

void DistributedProblem::permuteLinkingVars(const Permutation& perm) {
   assert(permutation_is_valid(linkVarsPermutation));
   assert(!is_hierarchy_root);

   dynamic_cast<DistributedMatrix&>(*equality_jacobian).permuteLinkingVars(perm);
   dynamic_cast<DistributedMatrix&>(*inequality_jacobian).permuteLinkingVars(perm);
   dynamic_cast<DistributedVector<double>&>(*objective_gradient).permuteVec0Entries(perm);
   dynamic_cast<DistributedVector<double>&>(*primal_upper_bounds).permuteVec0Entries(perm);
   dynamic_cast<DistributedVector<double>&>(*primal_lower_bounds).permuteVec0Entries(perm);
   dynamic_cast<DistributedVector<double>&>(*primal_upper_bound_indicators).permuteVec0Entries(perm);
   dynamic_cast<DistributedVector<double>&>(*primal_lower_bound_indicators).permuteVec0Entries(perm);
}

DistributedVariables*
DistributedProblem::getVarsUnperm(const Variables& vars, const Problem& unpermData_in) const {
   auto* unperm_vars = new DistributedVariables(dynamic_cast<const DistributedVariables&>(vars));
   const auto& unpermData = dynamic_cast<const DistributedProblem&>(unpermData_in);

   if (is_hierarchy_root)
      unperm_vars->collapseHierarchicalStructure(*this, unpermData.stochNode);

   unperm_vars->update_indicators(unpermData.primal_lower_bound_indicators, unpermData.primal_upper_bound_indicators,
      unpermData.inequality_lower_bound_indicators, unpermData.inequality_upper_bound_indicators);

   const Permutation perm_inv_link_vars = getLinkVarsPermInv();
   const Permutation perm_inv_link_cons_eq = getLinkConsEqPermInv();
   const Permutation perm_inv_link_cons_ineq = getLinkConsIneqPermInv();

   if (!perm_inv_link_vars.empty())
      unperm_vars->permuteVec0Entries(perm_inv_link_vars, true);

   if (!perm_inv_link_cons_eq.empty())
      unperm_vars->permuteEqLinkingEntries(perm_inv_link_cons_eq);

   if (!perm_inv_link_cons_ineq.empty())
      unperm_vars->permuteIneqLinkingEntries(perm_inv_link_cons_ineq, true);

   return unperm_vars;
}

DistributedResiduals*
DistributedProblem::getResidsUnperm(const Residuals& resids, const Problem& unpermData_in) const {
   auto* unperm_resids = new DistributedResiduals(dynamic_cast<const DistributedResiduals&>(resids));
   const auto& unpermData = dynamic_cast<const DistributedProblem&>(unpermData_in);

   if (is_hierarchy_root)
      unperm_resids->collapse_hierarchical_structure(*this, stochNode);

   unperm_resids->update_indicators(unpermData.primal_lower_bound_indicators, unpermData.primal_upper_bound_indicators,
      unpermData.inequality_lower_bound_indicators, unpermData.inequality_upper_bound_indicators);

   const Permutation perm_inv_link_vars = this->getLinkVarsPermInv();
   const Permutation perm_inv_link_cons_eq = this->getLinkConsEqPermInv();
   const Permutation perm_inv_link_cons_ineq = this->getLinkConsIneqPermInv();

   /* when using the hierarchical approach the unpermute is done in collapsHierarchicalStructure already */
   const bool do_not_permut_bounds = true;

   if (!perm_inv_link_vars.empty())
      unperm_resids->permute_vec_0_entries(perm_inv_link_vars, do_not_permut_bounds);

   if (!perm_inv_link_cons_eq.empty())
      unperm_resids->permute_eq_linking_entries(perm_inv_link_cons_eq);

   if (!perm_inv_link_cons_ineq.empty())
      unperm_resids->permute_ineq_linking_entries(perm_inv_link_cons_ineq, do_not_permut_bounds);

   return unperm_resids;
}

void DistributedProblem::removeN0LinkVarsIn2Links(std::vector<int>& n_blocks_per_link_var, const DistributedMatrix& Astoch,
   const DistributedMatrix& Cstoch,
   const std::vector<int>& linkStartBlockIdA, const std::vector<int>& linkStartBlockIdC) {
   for (size_t i = 0; i < n_blocks_per_link_var.size(); ++i) {
      /* variable is n0LinkVar */
      if (n_blocks_per_link_var[i] == 0) {
         /// Blmat
         {
            const SparseMatrix& Blmat = dynamic_cast<SparseMatrix&>(*Astoch.Blmat).getTranspose();
            const int col_start_A = Blmat.krowM()[i];
            const int col_end_A = Blmat.krowM()[i + 1];

            for (int k = col_start_A; k < col_end_A; ++k) {
               const int row_for_col = Blmat.jcolM()[k];

               /* variable appears in a 2link */
               if (linkStartBlockIdA[row_for_col] >= 0) {
                  n_blocks_per_link_var[i] = -1;
                  continue;
               }
            }
         }

         /// Dlmat
         {
            const SparseMatrix& Dlmat = dynamic_cast<SparseMatrix&>(*Cstoch.Blmat).getTranspose();
            const int col_start_C = Dlmat.krowM()[i];
            const int col_end_C = Dlmat.krowM()[i + 1];

            for (int k = col_start_C; k < col_end_C; ++k) {
               const int row_for_col = Dlmat.jcolM()[k];

               /* variable appears in a 2link */
               if (linkStartBlockIdC[row_for_col] >= 0) {
                  n_blocks_per_link_var[i] = -1;
                  continue;
               }
            }
         }
      }
   }
}


void DistributedProblem::activateLinkStructureExploitation() {
   assert(!stochNode->isHierarchicalRoot());

   if (useLinkStructure)
      return;
   useLinkStructure = true;

   const int myrank = PIPS_MPIgetRank(MPI_COMM_WORLD);

   /* don't attempt to use linking structure when there actually is no linking constraints */
   if (stochNode->myl() == 0 && stochNode->mzl() == 0) {
      if (pipsipmpp_options::get_bool_parameter("HIERARCHICAL")) {
         if (myrank == 0)
            std::cout << "No linking constraints found - hierarchical approach cannot be used\n";
         MPI_Abort(MPI_COMM_WORLD, -1);
      }

      useLinkStructure = false;
      if (myrank == 0)
         std::cout << "no linking constraints so no linking structure found\n";
      return;
   }

   const int nx0 = getLocalnx();

   const auto& Astoch = dynamic_cast<const DistributedMatrix&>(*equality_jacobian);
   const auto& Cstoch = dynamic_cast<const DistributedMatrix&>(*inequality_jacobian);

   n_blocks_per_link_var = std::vector<int>(nx0, 0);
   Astoch.updateKLinkVarsCount(n_blocks_per_link_var);

   std::vector<int> tmp = std::vector<int>(nx0,
      0); // to avoid doubling through second Allreduce inside the count functions
   Cstoch.updateKLinkVarsCount(tmp);

   std::transform(n_blocks_per_link_var.begin(), n_blocks_per_link_var.end(), tmp.begin(),
      n_blocks_per_link_var.begin(), std::plus<int>());

   Astoch.get2LinkStartBlocksAndCountsNew(linkStartBlockIdA, n_blocks_per_link_row_A);
   Cstoch.get2LinkStartBlocksAndCountsNew(linkStartBlockIdC, n_blocks_per_link_row_C);

   /* since 2 links can get permuted out of the linking part we cannot rely on n0Linkvars the appear in 2links */
   if (pipsipmpp_options::get_bool_parameter("HIERARCHICAL"))
      removeN0LinkVarsIn2Links(n_blocks_per_link_var, Astoch, Cstoch, linkStartBlockIdA, linkStartBlockIdC);

#ifndef NDEBUG
   std::vector<int> linkStart_A2 = Astoch.get2LinkStartBlocks();
   assert(linkStartBlockIdA.size() == linkStart_A2.size());
   for (size_t i = 0; i < linkStart_A2.size(); ++i) {
      assert(linkStart_A2[i] == linkStartBlockIdA[i]);
      if (linkStart_A2[i] != linkStartBlockIdA[i] && myrank == 0)
         std::cout << "New : " << linkStart_A2[i] << " != " << linkStartBlockIdA[i] << " old\n";
   }

   std::vector<int> linkStart_C2 = Cstoch.get2LinkStartBlocks();
   assert(linkStartBlockIdC.size() == linkStart_C2.size());
   for (size_t i = 0; i < linkStart_C2.size(); ++i) {
      assert(linkStart_C2[i] == linkStartBlockIdC[i]);
      if (linkStart_C2[i] != linkStartBlockIdC[i] && myrank == 0)
         std::cout << "New : " << linkStart_C2[i] << " != " << linkStartBlockIdC[i] << " old\n";
   }
#endif

   linkStartBlockLengthsA = get2LinkLengthsVec(linkStartBlockIdA, stochNode->nChildren());
   linkStartBlockLengthsC = get2LinkLengthsVec(linkStartBlockIdC, stochNode->nChildren());

   printLinkConsStats();
   printLinkVarsStats();

   int n2LinksEq = 0;
   int n2LinksIneq = 0;

   n0LinkVars = std::count_if(n_blocks_per_link_var.begin(), n_blocks_per_link_var.end(), [](int blocks) {
      return (blocks == 0);
   });

   if (threshold_global_vars <= 0) {
      n0LinkVars = 0;
   }

   n2LinksEq = std::count_if(linkStartBlockIdA.begin(), linkStartBlockIdA.end(), [](int blocks) {
      return (blocks >= 0);
   });

   n2LinksIneq = std::count_if(linkStartBlockIdC.begin(), linkStartBlockIdC.end(), [](int blocks) {
      return (blocks >= 0);
   });

#ifndef NDEBUG
   int n0LinkVars_cpy = 0;
   for (int i : n_blocks_per_link_var)
      if (i == 0)
         n0LinkVars_cpy++;
   if (threshold_global_vars <= 0) {
      n0LinkVars_cpy = 0;
   }
   int n2LinksEq_cpy = 0;
   for (int i : linkStartBlockIdA)
      if (i >= 0)
         n2LinksEq_cpy++;

   int n2LinksIneq_cpy = 0;
   for (int i : linkStartBlockIdC)
      if (i >= 0)
         n2LinksIneq_cpy++;

   assert(n0LinkVars_cpy == n0LinkVars);
   assert(n2LinksEq_cpy == n2LinksEq);
   assert(n2LinksIneq_cpy == n2LinksIneq);
   assert(n2LinksEq == n2linkRowsEq());
   assert(n2LinksIneq == n2linkRowsIneq());
#endif

   const double ratio =
      (n2LinksEq + n2LinksIneq + n0LinkVars) /
         double(linkStartBlockIdA.size() + linkStartBlockIdC.size() + n_blocks_per_link_var.size());
   if (myrank == 0) {
      std::cout << "number of 0-link variables: " << n0LinkVars << " (out of " << nx0 << " link variables)\n";
      std::cout << "number of equality 2-links: " << n2LinksEq << " (out of " << linkStartBlockIdA.size()
                << " equalities)\n";
      std::cout << "number of inequality 2-links: " << n2LinksIneq << " (out of " << linkStartBlockIdC.size()
                << " inequalities)\n";

      std::cout << "ratio: " << ratio << "\n";
   }


   if (!pipsipmpp_options::get_bool_parameter("SCHUR_COMPLEMENT_FORCE_SPARSE_COMPUTATIONS") && !pipsipmpp_options::get_bool_parameter("HIERARCHICAL")) {
      if (ratio < minStructuredLinksRatio) {
         if (myrank == 0)
            std::cout << "not enough linking structure found ( required ratio : " << minStructuredLinksRatio << ")\n";
         useLinkStructure = false;
      }
   }

   if (useLinkStructure) {
      assert(linkStartBlockIdA.size() == unsigned(stochNode->myl()));
      assert(linkStartBlockIdC.size() == unsigned(stochNode->mzl()));

#ifndef NDEBUG
      const int myl = stochNode->myl();
      const int mzl = stochNode->mzl();
      assert(myl >= 0 && mzl >= 0 && (mzl + myl > 0));
#endif

      assert(linkConsPermutationA.empty());
      assert(linkConsPermutationC.empty());

      const size_t nBlocks = dynamic_cast<DistributedVector<double>&>(*objective_gradient).children.size();

      // compute permutation vectors
      linkConsPermutationA = getAscending2LinkFirstGlobalsLastPermutation(linkStartBlockIdA, n_blocks_per_link_row_A,
         nBlocks,
         n_global_eq_linking_conss);
      linkConsPermutationC = getAscending2LinkFirstGlobalsLastPermutation(linkStartBlockIdC, n_blocks_per_link_row_C,
         nBlocks,
         n_global_ineq_linking_conss);
      permuteLinkingCons(linkConsPermutationA, linkConsPermutationC);

      assert(linkVarsPermutation.empty());
      //n0LinkVars = 0;
      linkVarsPermutation = get0VarsLastGlobalsFirstPermutation(n_blocks_per_link_var, n_global_linking_vars);
      permuteLinkingVars(linkVarsPermutation);
   }
}

void DistributedProblem::add_child(DistributedProblem* child) {
   children.push_back(child);
}

double DistributedProblem::evaluate_objective(const Variables& variables) const {
   const auto& x = dynamic_cast<const DistributedVector<double>&>(*variables.primals);
   std::unique_ptr<Vector<double> > temp(x.clone());

   this->get_objective_gradient(*temp);
   this->hessian_multiplication(1.0, *temp, 0.5, *variables.primals);
   return temp->dotProductWith(*variables.primals);
}

void DistributedProblem::printLinkVarsStats() {
   assert(!is_hierarchy_inner_leaf && !is_hierarchy_inner_root && !is_hierarchy_root);
   int n = getLocalnx();

   std::vector<int> linkCountA(n, 0);
   std::vector<int> linkCountC(n, 0);
   std::vector<int> linkCount0(n, 0);
   std::vector<int> linkCountLC(n, 0);

   auto& Astoch = dynamic_cast<DistributedMatrix&>(*equality_jacobian);
   auto& Cstoch = dynamic_cast<DistributedMatrix&>(*inequality_jacobian);

   Astoch.updateKLinkVarsCount(linkCountA);
   Cstoch.updateKLinkVarsCount(linkCountC);

   dynamic_cast<SparseMatrix&>(*Astoch.Bmat).getTranspose().updateNonEmptyRowsCount(linkCount0);
   dynamic_cast<SparseMatrix&>(*Astoch.Bmat).deleteTransposed();
   dynamic_cast<SparseMatrix&>(*Cstoch.Bmat).getTranspose().updateNonEmptyRowsCount(linkCount0);
   dynamic_cast<SparseMatrix&>(*Cstoch.Bmat).deleteTransposed();

   if (Astoch.Blmat) {
      dynamic_cast<SparseMatrix&>(*Astoch.Blmat).getTranspose().updateNonEmptyRowsCount(linkCountLC);
      dynamic_cast<SparseMatrix&>(*Astoch.Blmat).deleteTransposed();
   }

   if (Cstoch.Blmat) {
      dynamic_cast<SparseMatrix&>(*Cstoch.Blmat).getTranspose().updateNonEmptyRowsCount(linkCountLC);
      dynamic_cast<SparseMatrix&>(*Cstoch.Blmat).deleteTransposed();
   }

   const int rank = PIPS_MPIgetRank();

   if (rank == 0) {
      std::vector<int> linkSizes(nLinkStats, 0);

      int count0 = 0;
      int countLC = 0;
      int count0LC = 0;

      for (int i = 0; i < n; i++) {
         const int linkCountAB = linkCountA[i] + linkCountC[i];
         assert(linkCountAB >= 0 && linkCount0[i] >= 0 && linkCountLC[i] >= 0);
         assert(linkCount0[i] <= 2 && linkCountLC[i] <= 2);

         if (linkCountAB < nLinkStats)
            linkSizes[size_t(linkCountAB)]++;

         if (linkCountAB == 0 && linkCountLC[i] == 0 && linkCount0[i] != 0)
            count0++;

         if (linkCountAB == 0 && linkCount0[i] == 0 && linkCountLC[i] != 0)
            countLC++;

         if (linkCountAB == 0 && (linkCount0[i] != 0 || linkCountLC[i] != 0))
            count0LC++;
      }


      int nlocal = 0;
      for (int i = 0; i < nLinkStats; i++)
         if (linkSizes[i] != 0) {
            nlocal += linkSizes[i];
            std::cout << "---" << i << "-link vars: " << linkSizes[i] << "\n";
         }

      assert(n - nlocal >= 0);
      std::cout << "---total linking variables: " << n << " (global: " << n - nlocal << ")\n";

      std::cout << "   Block0 exclusive vars " << count0 << "\n";
      std::cout << "   LC exclusive vars " << countLC << "\n";
      std::cout << "   Block0 or LC vars " << count0LC << "\n";
   }
}

void DistributedProblem::printLinkConsStats() {
   int myl = getLocalmyl();
   int mzl = getLocalmzl();

   int rank;
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   if (myl > 0) {
      std::vector<int> linkCount(myl, 0);

      dynamic_cast<DistributedMatrix&>(*equality_jacobian).updateKLinkConsCount(linkCount);

      if (rank == 0) {
         std::vector<int> linkSizes(nLinkStats, 0);

         for (int i = 0; i < myl; i++)
            if (linkCount[i] < nLinkStats) {
               assert(linkCount[i] >= 0);
               linkSizes[size_t(linkCount[i])]++;
            }

         int nlocal = 0;
         for (int i = 0; i < nLinkStats; i++)
            if (linkSizes[i] != 0) {
               nlocal += linkSizes[i];
               std::cout << "---equality " << i << "-link cons: " << linkSizes[i] << "\n";
            }
         std::cout << "---total equality linking constraints: " << myl << " (global: " << myl - nlocal << ")\n";

      }
   } else if (rank == 0)
      std::cout << "---total equality linking constraints: 0\n";


   if (mzl > 0) {
      std::vector<int> linkCount(mzl, 0);

      dynamic_cast<DistributedMatrix&>(*inequality_jacobian).updateKLinkConsCount(linkCount);

      if (rank == 0) {
         std::vector<int> linkSizes(nLinkStats, 0);

         for (int i = 0; i < mzl; i++)
            if (linkCount[i] < nLinkStats) {
               assert(linkCount[i] >= 0);
               linkSizes[size_t(linkCount[i])]++;
            }

         int nlocal = 0;
         for (int i = 0; i < nLinkStats; i++)
            if (linkSizes[i] != 0) {
               nlocal += linkSizes[i];
               std::cout << "inequality " << i << "-link cons: " << linkSizes[i] << "\n";
            }
         std::cout << "---total inequality linking constraints: " << mzl << " (global: " << mzl - nlocal << ")\n";
      }
   } else if (rank == 0)
      std::cout << "---total inequality linking constraints: 0\n";
}

DistributedProblem::~DistributedProblem() {
   for (auto& it : children)
      delete it;
}

Permutation DistributedProblem::getLinkVarsPermInv() const {
   if (is_hierarchy_root)
      return this->children[0]->getLinkVarsPermInv();
   else
      return getInversePermutation(linkVarsPermutation);
}

Permutation DistributedProblem::getLinkConsEqPermInv() const {
   if (is_hierarchy_root)
      return this->children[0]->getLinkConsEqPermInv();
   else
      return getInversePermutation(linkConsPermutationA);
}

Permutation DistributedProblem::getLinkConsIneqPermInv() const {
   if (is_hierarchy_root)
      return this->children[0]->getLinkConsIneqPermInv();
   else
      return getInversePermutation(linkConsPermutationC);
}

int DistributedProblem::getLocalnx() const {
   return std::max(0, stochNode->nx());
}

int DistributedProblem::getLocalmy() const {
   return std::max(0, stochNode->my());
}

int DistributedProblem::getLocalmyl() const {
   return std::max(0, stochNode->myl());
}

int DistributedProblem::getLocalmz() const {
   return std::max(0, stochNode->mz());
}

int DistributedProblem::getLocalmzl() const {
   return std::max(0, stochNode->mzl());
}

void DistributedProblem::getLocalSizes(int& nx, int& my, int& mz, int& myl, int& mzl) const {
   nx = std::max(stochNode->nx(), 0);
   my = std::max(stochNode->my(), 0);
   mz = std::max(stochNode->mz(), 0);
   mzl = std::max(stochNode->mzl(), 0);
   myl = std::max(stochNode->myl(), 0);
}

void DistributedProblem::getLocalNnz(int& nnzQ, int& nnzB, int& nnzD) const {
   if (is_hierarchy_root || is_hierarchy_inner_root || is_hierarchy_inner_leaf)
      assert(0 && "TODO : implement");
   const auto& Qst = dynamic_cast<const DistributedSymmetricMatrix&>(*hessian);
   const auto& Ast = dynamic_cast<const DistributedMatrix&>(*equality_jacobian);
   const auto& Cst = dynamic_cast<const DistributedMatrix&>(*inequality_jacobian);

   nnzQ = dynamic_cast<const SparseSymmetricMatrix&>(*Qst.diag).getStorage().len + dynamic_cast<const SparseMatrix&>(*Qst.border).getStorage().len;
   nnzB = dynamic_cast<const SparseMatrix&>(*Ast.Bmat).getStorage().len;
   nnzD = dynamic_cast<const SparseMatrix&>(*Cst.Bmat).getStorage().len;
}

/*
 * At this stage we expect the Schur Complement to be of the form
 *                                                                                             nx  my0  mz0  myl  mzl
 *         [  Xsymi   0     0    X1iT FiT      X1iT GiT  ]     [ Q0  A0T  C0T  F0T  G0T  ]   [  x   x    x    x    x ]
 *         [   0      0     0       0             0      ]     [ A0   0    0    0    0   ]   [  x   0    0    0    0 ]
 * - SUM_i [   0      0     0       0             0      ]  +  [ C0   0   Om0   0    0   ] = [  x   0    x    0    0 ] =: global Schur complement (symmetric)
 *         [ Fi X1i   0     0   Fi K11i FiT  Fi K11i GiT ]     [ F0   0    0    0    0   ]   [  x   0    0    x    x ]
 *         [ Gi X1i   0     0   Gi K11i FiT  Gi K11i GiT ]     [ G0   0    0    0  OmN+1 ]   [  x   0    0    x    x ]
 *
 * since we know, that x_3 = Om0^-1 ( b_3 - C0 x1 ) we can substitute x_3 in this system and arrive at the smaller
 *
 *         [  Xsymi   0      X1iT FiT      X1iT GiT  ]     [ Q0  A0T  F0T  G0T  ]   [  x   x    x    x ]
 *         [   0      0         0             0      ]  +  [ A0   0    0    0   ] = [  x   0    0    0 ] =: global Schur complement (symmetric)
 * - SUM_i [ Fi X1i   0     Fi K11i FiT  Fi K11i GiT ]     [ F0   0    0    0   ]   [  x   0    x    x ]
 *         [ Gi X1i   0     Gi K11i FiT  Gi K11i GiT ]     [ G0   0    0  OmN+1 ]   [  x   0    x    x ]
 *
 * additionally we detect n0linkVars - Varaibles only appearing in one linking constraint block (dual to the A0/B0 block).
 * These will not get dense since no one adds to them - we add them separately with their correct number of non-zeros:
 *
 *                                                                                                   nx  my0  myl-myl00 mzl-mzl00 myl00 mzl00
 *         [  Xsymi   0   X1iT FiT      X1iT GiT  0  0 ]     [ Q0  A0T  F0T  G0T F00T   G00T  ]   [   x   x      x         x        x     x  ]
 *         [   0      0      0             0      0  0 ]     [ A0   0    0    0    0     0    ]   [   x   0      0         0        0     0  ]
 * - SUM_i [ Fi X1i   0  Fi K11i FiT  Fi K11i GiT 0  0 ]  +  [ F0   0    0    0    0     0    ] = [   x   0      x         x        0     0  ]
 *         [ Gi X1i   0  Gi K11i FiT  Gi K11i GiT 0  0 ]     [ G0   0    0  OmN+1  0     0    ]   [   x   0      x         x        0     0  ]
 *         [    0     0      0             0      0  0 ]     [ F00  0    0    0    0     0    ]   [   x   0      0         0        0     0  ]
 *         [    0     0      0             0      0  0 ]     [ G00  0    0    0    0  OmN+100 ]   [   x   0      0         0        0     x  ]
 *
 *
 *                       [ Qi BiT DiT ]^-1     [ K11 K12 K13 ]
 * Where Ki = (Ki)_lk =  [ Bi  0   0  ]      = [ K21 K22 K23 ]
 *                       [ Di  0   0  ]        [ K31 K32 K33 ] symmetric, and Om0 OmN+1 are diagonal and Xsym symmetric too.
 *
 *
 * Structure:
 * SUM_i Fi K11i FiT = SUM_i Fi K11i GiT = SUM_i Gi K11i FiT = SUM_i Gi K11i GiT
 *
 *      [ x  x  x  x  .  .  x  x  x ]
 *      [ x  x  x                   ]
 *      [ x  x  x  x                ]
 *    = [ .     x  .  .             ]
 *      [ .        .  .  .          ]
 *      [ .           .  .  .       ]
 *      [ .              .  .  x    ]
 *      [ .                 x  x  x ]
 *      [ x                    x  x ]
 *
 *      sizes depending on the blocks involved.
 *
 * For the (distributed) computation of the Schur Complement we only need to communicate
 *
 *       [  Xsymi   X1iT FiT      X1iT GiT  ]     [ Q0  F0T  G0T  ]   [  x   x   x ]
 * SUM_i [ Fi X1i  Fi K11i FiT  Fi K11i GiT ]  +  [ F0   0    0   ] = [  x   x   x ]
 *       [ Gi X1i  Gi K11i FiT  Gi K11i GiT ]     [ G0   0  OmN+1 ]   [  x   x   x ]
 *
 *       and even more structure can be exploited : define n0LinkVars variables that do only appear in the A_0(B_0) / C_0(D_0) block and in non of
 *       the other A_i, C_i
 *
 *       X1i = K11iRi + K12i Ai + K13i Ci

 *       Since Fi X1i := Fi (K11i Ri + K12i Ai + K12i Ci) = Fi (K11i [0, Ri] + K12i [0, Ai] + K13i [0, Ci] )
 *           =  [     0        Fi (K11i Ri + K12i Ai + K13i Ci] myl
 *                 n0LinkVars             n - n0LinkVars
 *
 *       So we need not to store the n0LinkVars (same for sym?) part either (neither for F, not for G), thus:
 *
 *                                                                      nx  myl mzl
 *       [  Xsymi   X1iT FiT      X1iT GiT  ]     [ Q0  F0T  G0T  ]   [  x   x   x ]
 * SUM_i [ Fi X1i  Fi K11i FiT  Fi K11i GiT ]  +  [ F0   0    0   ] = [  x   x   x ]
 *       [ Gi X1i  Gi K11i FiT  Gi K11i GiT ]     [ G0   0  OmN+1 ]   [  x   x   x ]
 *
 *
 */
int DistributedProblem::getSchurCompMaxNnz() const {
   if (is_hierarchy_root)
      assert(0 && "not available in hierarchy root");
   assert(!children.empty());

   const int n0 = getLocalnx();
   const int my = getLocalmy();
   const int myl = getLocalmyl();
   const int mzl = getLocalmzl();

#ifndef NDEBUG
   if (!is_hierarchy_inner_leaf) {
      const auto[mB, nB] = getLocalB().n_rows_columns();
      assert( (mB == my || my == 0) && nB == n0);
   } else {
      assert(my == 0);
      assert(n0 == 0);
   }
#endif

   int nnz = 0;

   assert(n0 >= n0LinkVars);

   /* Xsym */
   nnz += nnzTriangular(n0);

   // add B_0 (or A_0, depending on notation)
   if (my > 0)
      nnz += getLocalB().numberOfNonZeros();

   // add borders
   /* X1iT FiT */
   nnz += myl * (n0 - n0LinkVars);
   /* X1iT GiT */
   nnz += mzl * (n0 - n0LinkVars);

   // (empty) diagonal
   nnz += my;

   // add linking equality parts
   nnz += getSCdiagBlocksMaxNnz(linkStartBlockIdA.size(), linkStartBlockLengthsA);

   // add linking inequality parts
   nnz += getSCdiagBlocksMaxNnz(linkStartBlockIdC.size(), linkStartBlockLengthsC);

   // add linking mixed parts
   nnz += getSCmixedBlocksMaxNnz(linkStartBlockIdA.size(), linkStartBlockIdC.size(), linkStartBlockLengthsA,
      linkStartBlockLengthsC);

   if (myl > 0) {
      const SparseMatrix& Ft = getLocalF().getTranspose();
      const int* startRowFtrans = Ft.krowM();
      nnz += startRowFtrans[n0] - startRowFtrans[n0 - n0LinkVars];
   }

   if (mzl > 0) {
      const SparseMatrix& Gt = getLocalG().getTranspose();
      const int* startRowGtrans = Gt.krowM();
      nnz += startRowGtrans[n0] - startRowGtrans[n0 - n0LinkVars];
   }
   return nnz;
}


int DistributedProblem::getSchurCompMaxNnzDist(int blocksStart, int blocksEnd) const {
   assert(!children.empty());

   const int n0 = getLocalnx();
   const int my = getLocalmy();
   const int myl = getLocalmyl();
   const int mzl = getLocalmzl();
   const int mylLocal = myl - getSCdiagBlocksNRows(linkStartBlockLengthsA) +
      getSCdiagBlocksNRows(linkStartBlockLengthsA, blocksStart, blocksEnd);
   const int mzlLocal = mzl - getSCdiagBlocksNRows(linkStartBlockLengthsC) +
      getSCdiagBlocksNRows(linkStartBlockLengthsC, blocksStart, blocksEnd);

#ifndef NDEBUG
   {
      const auto[mB, nB] = getLocalB().n_rows_columns();
      assert(mB == my && nB == n0);
   }
#endif

   int nnz = 0;

   assert(n0 >= n0LinkVars);

   // sum up half of dense square
   nnz += nnzTriangular(n0);

   // add B_0 (or A_0, depending on notation)
   nnz += getLocalB().numberOfNonZeros();

   // add borders
   nnz += mylLocal * (n0 - n0LinkVars);
   nnz += mzlLocal * (n0 - n0LinkVars);

   // (empty) diagonal
   nnz += my;

   // add linking equality parts
   nnz += getSCdiagBlocksMaxNnzDist(linkStartBlockIdA.size(), linkStartBlockLengthsA, blocksStart, blocksEnd);

   // add linking inequality parts
   nnz += getSCdiagBlocksMaxNnzDist(linkStartBlockIdC.size(), linkStartBlockLengthsC, blocksStart, blocksEnd);

   // add linking mixed parts
   nnz += getSCmixedBlocksMaxNnzDist(linkStartBlockIdA.size(), linkStartBlockIdC.size(), linkStartBlockLengthsA,
      linkStartBlockLengthsC, blocksStart,
      blocksEnd);

   if (myl > 0) {
      const SparseMatrix& Ft = getLocalF().getTranspose();
      const int* startRowFtrans = Ft.krowM();
      nnz += startRowFtrans[n0] - startRowFtrans[n0 - n0LinkVars];
   }

   if (mzl > 0) {
      const SparseMatrix& Gt = getLocalG().getTranspose();
      const int* startRowGtrans = Gt.krowM();
      nnz += startRowGtrans[n0] - startRowGtrans[n0 - n0LinkVars];
   }

   return nnz;
}

const SparseSymmetricMatrix& DistributedProblem::getLocalQ() const {
   if(is_hierarchy_root) {
      const auto& Q_bordered = dynamic_cast<const BorderedSymmetricMatrix&>(*hessian);
      return dynamic_cast<const SparseSymmetricMatrix&>(*Q_bordered.top_left_block);
   } else if (is_hierarchy_inner_leaf && stochNode->getCommWorkers() != MPI_COMM_NULL) {
      const auto& Q_distributed = dynamic_cast<const DistributedSymmetricMatrix&>(*hessian);
      assert(Q_distributed.diag->is_a(kStochSymMatrix));
      return dynamic_cast<SparseSymmetricMatrix&>(*dynamic_cast<DistributedSymmetricMatrix&>(*Q_distributed.diag).diag);
   } else {
      const auto& Q_distributed = dynamic_cast<const DistributedSymmetricMatrix&>(*hessian);
      assert(Q_distributed.diag->is_a(kSparseSymMatrix));
      return dynamic_cast<SparseSymmetricMatrix&>(*Q_distributed.diag);
   }
}

const SparseMatrix& DistributedProblem::getLocalCrossHessian() const {
   auto& Qst = dynamic_cast<const DistributedSymmetricMatrix&>(*hessian);
   assert(!is_hierarchy_inner_root && !is_hierarchy_root && !is_hierarchy_inner_leaf);

   return dynamic_cast<const SparseMatrix&>(*Qst.border);
}

// T_i x_0 + W_i x_i = b_i

// This is T_i
const SparseMatrix& DistributedProblem::getLocalA() const {
   assert(!is_hierarchy_root);

   if (is_hierarchy_inner_leaf && stochNode->getCommWorkers() != MPI_COMM_NULL) {
      const auto& A_distributed = dynamic_cast<const DistributedMatrix&>(*equality_jacobian);
      assert(A_distributed.Amat->is_a(kDistributedMatrix));
      return dynamic_cast<const SparseMatrix&>(*dynamic_cast<const DistributedMatrix&>(*A_distributed.Amat).Amat);
   } else {
      const auto& A_distributed = dynamic_cast<const DistributedMatrix&>(*equality_jacobian);
      assert(A_distributed.Amat->is_a(kSparseGenMatrix));
      return dynamic_cast<const SparseMatrix&>(*A_distributed.Amat);
   }
}

// This is W_i:
const SparseMatrix& DistributedProblem::getLocalB() const {
   if (is_hierarchy_root) {
      const auto& A_bordered = dynamic_cast<const BorderedMatrix&>(*equality_jacobian);
      return dynamic_cast<const SparseMatrix&>(*A_bordered.border_left->first);
   } else if (is_hierarchy_inner_leaf && stochNode->getCommWorkers() != MPI_COMM_NULL) {
      auto& A_distributed = dynamic_cast<const DistributedMatrix&>(*equality_jacobian);
      assert(A_distributed.Bmat->is_a(kDistributedMatrix));
      return dynamic_cast<const SparseMatrix&>(*dynamic_cast<const DistributedMatrix&>(*A_distributed.Bmat).Bmat);
   } else {
      auto& A_distributed = dynamic_cast<const DistributedMatrix&>(*equality_jacobian);
      assert(A_distributed.Bmat->is_a(kSparseGenMatrix));
      return dynamic_cast<const SparseMatrix&>(*A_distributed.Bmat);
   }
}

// This is F_i (linking equality matrix):
const SparseMatrix& DistributedProblem::getLocalF() const {
   if (is_hierarchy_root) {
      const auto& A_bordered = dynamic_cast<const BorderedMatrix&>(*equality_jacobian);
      return dynamic_cast<const SparseMatrix&>(*A_bordered.bottom_left_block);
   } else if (is_hierarchy_inner_leaf && stochNode->getCommWorkers() != MPI_COMM_NULL) {
      const auto& A_distributed = dynamic_cast<const DistributedMatrix&>(*equality_jacobian);
      assert(A_distributed.Bmat->is_a(kDistributedMatrix));
      return dynamic_cast<const SparseMatrix&>(*dynamic_cast<const DistributedMatrix&>(*A_distributed.Bmat).Blmat);
   } else {
      const auto& A_distributed = dynamic_cast<const DistributedMatrix&>(*equality_jacobian);
      assert(A_distributed.Blmat->is_a(kSparseGenMatrix));
      return dynamic_cast<const SparseMatrix&>(*A_distributed.Blmat);
   }
}

const StripMatrix& DistributedProblem::getLocalFBorder() const {
   assert(is_hierarchy_inner_leaf);
   auto& Ast = dynamic_cast<const DistributedMatrix&>(*equality_jacobian);

   assert(Ast.Blmat->is_a(kStripMatrix));
   return dynamic_cast<const StripMatrix&>(*Ast.Blmat);
}

const StripMatrix& DistributedProblem::getLocalGBorder() const {
   assert(is_hierarchy_inner_leaf);
   auto& Cst = dynamic_cast<const DistributedMatrix&>(*inequality_jacobian);

   assert(Cst.Blmat->is_a(kStripMatrix));
   return dynamic_cast<const StripMatrix&>(*Cst.Blmat);
}

// low_i <= C_i x_0 + D_i x_i <= upp_i

// This is C_i
const SparseMatrix& DistributedProblem::getLocalC() const {
   assert(!is_hierarchy_root);
   auto& Cst = dynamic_cast<const DistributedMatrix&>(*inequality_jacobian);

   if (is_hierarchy_inner_leaf && stochNode->getCommWorkers() != MPI_COMM_NULL) {
      assert(Cst.Amat->is_a(kDistributedMatrix));
      return dynamic_cast<const SparseMatrix&>(*dynamic_cast<const DistributedMatrix&>(*Cst.Amat).Amat);
   } else {
      assert(Cst.Amat->is_a(kSparseGenMatrix));
      return dynamic_cast<const SparseMatrix&>(*Cst.Amat);
   }
}

// This is D_i
const SparseMatrix& DistributedProblem::getLocalD() const {
   assert(!is_hierarchy_root);
   auto& Cst = dynamic_cast<const DistributedMatrix&>(*inequality_jacobian);

   if (is_hierarchy_inner_leaf && stochNode->getCommWorkers() != MPI_COMM_NULL) {
      assert(Cst.Bmat->is_a(kDistributedMatrix));
      return dynamic_cast<const SparseMatrix&>(*dynamic_cast<const DistributedMatrix&>(*Cst.Bmat).Bmat);
   } else {
      assert(Cst.Bmat->is_a(kSparseGenMatrix));
      return dynamic_cast<const SparseMatrix&>(*Cst.Bmat);
   }

}

// This is G_i (linking inequality matrix):
const SparseMatrix& DistributedProblem::getLocalG() const {
   if (is_hierarchy_root) {
      const auto& C_bordered = dynamic_cast<const BorderedMatrix&>(*inequality_jacobian);
      return dynamic_cast<const SparseMatrix&>(*C_bordered.bottom_left_block);
   } else if (is_hierarchy_inner_leaf && stochNode->getCommWorkers() != MPI_COMM_NULL) {
      const auto& C_distributed = dynamic_cast<const DistributedMatrix&>(*inequality_jacobian);
      assert(C_distributed.Bmat->is_a(kDistributedMatrix));
      return dynamic_cast<const SparseMatrix&>(*dynamic_cast<const DistributedMatrix&>(*C_distributed.Bmat).Blmat);
   } else {
      const auto& C_distributed = dynamic_cast<const DistributedMatrix&>(*inequality_jacobian);
      assert(C_distributed.Blmat->is_a(kSparseGenMatrix));
      return dynamic_cast<const SparseMatrix&>(*C_distributed.Blmat);
   }
}

void
DistributedProblem::cleanUpPresolvedData(const DistributedVector<int>& rowNnzVecA, const DistributedVector<int>& rowNnzVecC,
   const DistributedVector<int>& colNnzVec) {
   auto& Q_stoch = dynamic_cast<DistributedSymmetricMatrix&>(*hessian);
   // todo only works if Q is empty - not existent
   Q_stoch.deleteEmptyRowsCols(colNnzVec);
   Q_stoch.recomputeSize();

   // clean up equality system
   auto& A_stoch = dynamic_cast<DistributedMatrix&>(*equality_jacobian);
   auto& b_Astoch = dynamic_cast<DistributedVector<double>&>(*equality_rhs);

   A_stoch.initStaticStorageFromDynamic(rowNnzVecA, colNnzVec);
   A_stoch.freeDynamicStorage();
   A_stoch.recomputeSize();

   b_Astoch.removeEntries(rowNnzVecA);

   // clean up inequality system and x
   auto& C_stoch = dynamic_cast<DistributedMatrix&>(*inequality_jacobian);
   auto& g_stoch = dynamic_cast<DistributedVector<double>&>(*objective_gradient);

   auto& blx_stoch = dynamic_cast<DistributedVector<double>&>(*primal_lower_bounds);
   auto& ixlow_stoch = dynamic_cast<DistributedVector<double>&>(*primal_lower_bound_indicators);
   auto& bux_stoch = dynamic_cast<DistributedVector<double>&>(*primal_upper_bounds);
   auto& ixupp_stoch = dynamic_cast<DistributedVector<double>&>(*primal_upper_bound_indicators);

   auto& bl_stoch = dynamic_cast<DistributedVector<double>&>(*inequality_lower_bounds);
   auto& iclow_stoch = dynamic_cast<DistributedVector<double>&>(*inequality_lower_bound_indicators);
   auto& bu_stoch = dynamic_cast<DistributedVector<double>&>(*inequality_upper_bounds);
   auto& icupp_stoch = dynamic_cast<DistributedVector<double>&>(*inequality_upper_bound_indicators);

   C_stoch.initStaticStorageFromDynamic(rowNnzVecC, colNnzVec);
   C_stoch.freeDynamicStorage();
   C_stoch.recomputeSize();

   g_stoch.removeEntries(colNnzVec);

   blx_stoch.removeEntries(colNnzVec);
   ixlow_stoch.removeEntries(colNnzVec);
   bux_stoch.removeEntries(colNnzVec);
   ixupp_stoch.removeEntries(colNnzVec);

   bl_stoch.removeEntries(rowNnzVecC);
   iclow_stoch.removeEntries(rowNnzVecC);
   bu_stoch.removeEntries(rowNnzVecC);
   icupp_stoch.removeEntries(rowNnzVecC);

   nx = g_stoch.length();
   my = A_stoch.n_rows();
   mz = C_stoch.n_rows();

   number_primal_lower_bounds = ixlow_stoch.number_nonzeros();
   number_primal_upper_bounds = ixupp_stoch.number_nonzeros();
   number_inequality_lower_bounds = iclow_stoch.number_nonzeros();
   number_inequality_upper_bounds = icupp_stoch.number_nonzeros();
}

void DistributedProblem::initDistMarker(int blocksStart, int blocksEnd) const {
   assert(isSCrowLocal.empty());
   assert(isSCrowMyLocal.empty());

   assert(!linkStartBlockIdA.empty() || !linkStartBlockIdC.empty());
   assert(blocksStart >= 0 && blocksStart < blocksEnd && blocksEnd <= int(linkStartBlockLengthsA.size()));

   const int nx0 = getLocalnx();
   const int my0 = getLocalmy();
   const int myl = getLocalmyl();
   const int mzl = getLocalmzl();
   const int sizeSC = nx0 + my0 + myl + mzl;

   assert(sizeSC > 0);

   isSCrowLocal.resize(sizeSC);
   isSCrowMyLocal.resize(sizeSC);

   for (int i = 0; i < nx0; i++) {
      isSCrowLocal[i] = false;
      isSCrowMyLocal[i] = false;
   }

   for (int i = nx0; i < nx0 + my0; i++) {
      isSCrowLocal[i] = false;
      isSCrowMyLocal[i] = false;
   }

   // equality linking
   for (int i = nx0 + my0, j = 0; i < nx0 + my0 + myl; i++, j++) {
      assert(unsigned(j) < linkStartBlockIdA.size());
      const int block = linkStartBlockIdA[j];
      isSCrowLocal[i] = (block != -1);
      isSCrowMyLocal[i] = (block >= blocksStart && block < blocksEnd);
   }

   // inequality linking
   for (int i = nx0 + my0 + myl, j = 0; i < nx0 + my0 + myl + mzl; i++, j++) {
      assert(unsigned(j) < linkStartBlockIdC.size());
      const int block = linkStartBlockIdC[j];
      isSCrowLocal[i] = (block != -1);
      isSCrowMyLocal[i] = (block >= blocksStart && block < blocksEnd);
   }
}

const std::vector<bool>& DistributedProblem::getSCrowMarkerLocal() const {
   assert(!isSCrowLocal.empty());

   return isSCrowLocal;
}

const std::vector<bool>& DistributedProblem::getSCrowMarkerMyLocal() const {
   assert(!isSCrowMyLocal.empty());

   return isSCrowMyLocal;
}

int DistributedProblem::n2linkRowsEq() const {
   return n2linksRows(linkStartBlockLengthsA);
}

int DistributedProblem::n2linkRowsIneq() const {
   return n2linksRows(linkStartBlockLengthsC);
}

// is root node data of DistributedQP object same on all procs?
bool DistributedProblem::isRootNodeInSync() const {
   bool in_sync = true;

   /* matrix Q */
   // todo

   /* matrix A */
   if (!dynamic_cast<const DistributedMatrix&>(*equality_jacobian).isRootNodeInSync()) {
      std::cout << "ERROR: matrix A corrupted!\n";
      in_sync = false;
   }

   /* matrix C */
   if (!dynamic_cast<const DistributedMatrix&>(*inequality_jacobian).isRootNodeInSync()) {
      std::cout << "ERROR: matrix C corrupted!\n";
      in_sync = false;
   }

   /* objective g */
   if (!dynamic_cast<const DistributedVector<double>&>(*objective_gradient).isRootNodeInSync()) {
      std::cout << "ERROR: objective vector corrupted!\n";
      in_sync = false;
   }

   /* rhs equality bA */
   if (!dynamic_cast<const DistributedVector<double>&>(*equality_rhs).isRootNodeInSync()) {
      std::cout << "ERROR: rhs of A corrupted!\n";
      in_sync = false;
   }

   /* upper bounds x bux */
   if (!dynamic_cast<const DistributedVector<double>&>(*primal_upper_bounds).isRootNodeInSync()) {
      std::cout << "ERROR: upper bounds x corrupted!\n";
      in_sync = false;
   }

   /* index for upper bounds x ixupp */
   if (!dynamic_cast<const DistributedVector<double>&>(*primal_upper_bound_indicators).isRootNodeInSync()) {
      std::cout << "ERROR: index upper bounds x corrupted!\n";
      in_sync = false;
   }

   /* lower bounds x blx */
   if (!dynamic_cast<const DistributedVector<double>&>(*primal_lower_bounds).isRootNodeInSync()) {
      std::cout << "ERROR: lower bounds x corrupted!\n";
      in_sync = false;
   }

   /* index for lower bounds x ixlow */
   if (!dynamic_cast<const DistributedVector<double>&>(*primal_lower_bound_indicators).isRootNodeInSync()) {
      std::cout << "ERROR: index lower bounds x corrupted!\n";
      in_sync = false;
   }

   /* upper bounds C bu */
   if (!dynamic_cast<const DistributedVector<double>&>(*inequality_upper_bounds).isRootNodeInSync()) {
      std::cout << "ERROR: rhs C corrupted!\n";
      in_sync = false;
   }

   /* index upper bounds C icupp */
   if (!dynamic_cast<const DistributedVector<double>&>(*inequality_upper_bound_indicators).isRootNodeInSync()) {
      std::cout << "ERROR: index rhs C corrupted!\n";
      in_sync = false;
   }

   /* lower bounds C bl */
   if (!dynamic_cast<const DistributedVector<double>&>(*inequality_lower_bounds).isRootNodeInSync()) {
      std::cout << "ERROR: lower bounds C corrupted!\n";
      in_sync = false;
   }

   /* index for lower bounds C iclow */
   if (!dynamic_cast<const DistributedVector<double>&>(*inequality_lower_bound_indicators).isRootNodeInSync()) {
      std::cout << "ERROR: index lower bounds C corrupted!\n";
      in_sync = false;
   }

   /* scale sc */
   // todo

   return in_sync;
}