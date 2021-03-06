/*
 * StochVectorUtilities.h
 *
 *  Created on: 13.11.2019
 *      Author: bzfkempk
 */
#ifndef DISTRIBUTEDVECTORUTILITIES_H
#define DISTRIBUTEDVECTORUTILITIES_H

#include "DistributedVector.h"
#include "DenseVector.hpp"

#include <string>
#include <iostream>
#include <limits>
// TODO : Hierarchical approach cannot use any of these...

/* Utility functions to return for a given StochVectorBase<T> a certain DenseVectorBase<T>
 *	either from one of the children (node = 0, ..., nChildren - 1 )
 * or from the parent node = -1
 *
 * for the col first function DenseVectorBase<T> first is returned
 *
 * for row first function either the associated DenseVectorBase<T> first (linking = false) or last (linking = true) is returned
 *
 * asserts existence of these vectors as well as the specified child
 */
template<typename T>
inline DenseVector<T>& getSimpleVecFromStochVec(const DistributedVector<T>& stochvec, int node, bool linking) {
   assert(-1 <= node && node < static_cast<int>(stochvec.children.size()));

   if (node == -1) {
      if (linking) {
         assert(stochvec.last);
         return dynamic_cast<DenseVector<T>&>(*(stochvec.last));
      }
      else {
         assert(stochvec.first);
         return dynamic_cast<DenseVector<T>&>(*(stochvec.first));
      }
   }
   else {
      if (linking) {
         assert(stochvec.last);
         return dynamic_cast<DenseVector<T>&>(*(stochvec.last));
      }
      else {
         assert(stochvec.children[node]->first);
         return dynamic_cast<DenseVector<T>&>(*(stochvec.children[node]->first));
      }
   }
}

template<typename T>
inline DenseVector<T>& getSimpleVecFromRowStochVec(const Vector<T>& ooqpvec, int node, bool linking) {
   return getSimpleVecFromStochVec(dynamic_cast<const DistributedVector<T>&>(ooqpvec), node, linking);
}

template<typename T>
inline DenseVector<T>& getSimpleVecFromColStochVec(const Vector<T>& ooqpvec, int node) {
   return getSimpleVecFromStochVec(dynamic_cast<const DistributedVector<T>&>(ooqpvec), node, false);
}

template<typename T>
inline T& getSimpleVecFromRowStochVec(const Vector<T>& ooqpvec, const INDEX& row) {
   assert(row.isRow());
   DenseVector<T>& vec = getSimpleVecFromStochVec(dynamic_cast<const DistributedVector<T>&>(ooqpvec), row.getNode(), row.getLinking());
   const int index = row.getIndex();
   assert(0 <= index && index < vec.length());

   return vec[index];
}

template<typename T>
inline T& getSimpleVecFromColStochVec(const Vector<T>& ooqpvec, const INDEX& col) {
   assert(col.isCol());
   DenseVector<T>& vec = getSimpleVecFromStochVec(dynamic_cast<const DistributedVector<T>&>(ooqpvec), col.getNode(), false);
   const int index = col.getIndex();
   assert(0 <= index && index < vec.length());

   return vec[index];
}

/// clone the structure of a StochVectorBase<T> into one of type U
template<typename T, typename U>
inline DistributedVector<U>* cloneStochVector(const DistributedVector<T>& svec) {
   DistributedVector<U>* clone;

   if (svec.isKindOf(kStochDummy))
      return new DistributedDummyVector<U>();

   if (svec.last)
      clone = new DistributedVector<U>(svec.first->length(), svec.last->length(), svec.mpiComm);
   else
      clone = new DistributedVector<U>(svec.first->length(), svec.mpiComm);

   for (size_t it = 0; it < svec.children.size(); it++) {
      clone->AddChild(std::shared_ptr<DistributedVector<U>>(cloneStochVector<T, U>(*svec.children[it])));
   }
   return clone;
}

template<typename T, typename U>
inline DistributedDummyVector<U>* cloneStochVector(const DistributedDummyVector<T>&) {
   return new DistributedDummyVector<U>();
}

template<typename T, typename U>
inline DistributedVector<U>* cloneStochVector(const Vector<T>& ooqpvec) {
   if (ooqpvec.isKindOf(kStochDummy))
      return cloneStochVector<T, U>(dynamic_cast<const DistributedDummyVector<T>&>(ooqpvec));
   else
      return cloneStochVector<T, U>(dynamic_cast<const DistributedVector<T>&>(ooqpvec));
}

inline void writeLBltXltUBToStreamAllStringStream(std::stringstream& sout, const DenseVector<double>& lb, const DenseVector<double>& ixlow,
      const DenseVector<double>& ub, const DenseVector<double>& ixupp) {
   assert(lb.length() == ixlow.length());
   assert(lb.length() == ub.length());
   assert(lb.length() == ixupp.length());

   for (int i = 0; i < lb.length(); i++) {
      const double lbx = PIPSisZero(ixlow[i]) ? -std::numeric_limits<double>::infinity() : lb[i];
      const double ubx = PIPSisZero(ixupp[i]) ? std::numeric_limits<double>::infinity() : ub[i];

      sout << lbx << "\t<= x <=\t" << ubx << "\n";
   }
}

inline void
writeLBltXltUBtoStringStreamDenseChild(std::stringstream& sout, const DistributedVector<double>& lb, const DistributedVector<double>& ixlow,
      const DistributedVector<double>& ub, const DistributedVector<double>& ixupp) {
   assert(lb.children.size() == ixlow.children.size());
   assert(lb.children.size() == ub.children.size());
   assert(lb.children.size() == ixupp.children.size());

   if (lb.isKindOf(kStochDummy)) {
      assert(ixlow.isKindOf(kStochDummy));
      assert(ub.isKindOf(kStochDummy));
      assert(ixupp.isKindOf(kStochDummy));
      return;
   }

   sout << "--" << std::endl;
   assert(lb.first);
   assert(ixlow.first);
   assert(ub.first);
   assert(ixupp.first);
   writeLBltXltUBToStreamAllStringStream(sout, dynamic_cast<const DenseVector<double>&>(*lb.first),
         dynamic_cast<const DenseVector<double>&>(*ixlow.first), dynamic_cast<const DenseVector<double>&>(*ub.first),
         dynamic_cast<const DenseVector<double>&>(*ixupp.first));


   for (size_t it = 0; it < lb.children.size(); it++) {
      sout << "-- " << std::endl;
      writeLBltXltUBtoStringStreamDenseChild(sout, *lb.children[it], *ixlow.children[it], *ub.children[it], *ixupp.children[it]);
   }

   if (lb.last) {
      assert(ixlow.last);
      assert(ub.last);
      assert(ixupp.last);
      sout << "---" << std::endl;
      writeLBltXltUBToStreamAllStringStream(sout, dynamic_cast<const DenseVector<double>&>(*lb.last),
            dynamic_cast<const DenseVector<double>&>(*ixlow.last), dynamic_cast<const DenseVector<double>&>(*ub.last),
            dynamic_cast<const DenseVector<double>&>(*ixupp.last));
   }
}

inline void writeLBltXltUBtoStreamDense(std::ostream& out, const DistributedVector<double>& lb, const DistributedVector<double>& ixlow,
      const DistributedVector<double>& ub, const DistributedVector<double>& ixupp, MPI_Comm mpiComm) {
   assert(lb.children.size() == ixlow.children.size());
   assert(lb.children.size() == ub.children.size());
   assert(lb.children.size() == ixupp.children.size());

   if (lb.isKindOf(kStochDummy)) {
      assert(ixlow.isKindOf(kStochDummy));
      assert(ub.isKindOf(kStochDummy));
      assert(ixupp.isKindOf(kStochDummy));
      return;
   }

   const int my_rank = PIPS_MPIgetRank(mpiComm);
   const int world_size = PIPS_MPIgetSize(mpiComm);
   bool is_distributed = PIPS_MPIgetDistributed(mpiComm);

   MPI_Status status;
   int l;
   std::stringstream sout;

   if (my_rank == 0) {
      sout << "----" << std::endl;
      assert(lb.first);
      assert(ixlow.first);
      assert(ub.first);
      assert(ixupp.first);
      writeLBltXltUBToStreamAllStringStream(sout, dynamic_cast<const DenseVector<double>&>(*lb.first),
            dynamic_cast<const DenseVector<double>&>(*ixlow.first), dynamic_cast<const DenseVector<double>&>(*ub.first),
            dynamic_cast<const DenseVector<double>&>(*ixupp.first));

      for (size_t it = 0; it < lb.children.size(); it++)
         writeLBltXltUBtoStringStreamDenseChild(sout, *lb.children[it], *ixlow.children[it], *ub.children[it], *ixupp.children[it]);

      out << sout.str();
      sout.str(std::string());

      for (int p = 1; p < world_size; p++) {
         MPI_Probe(p, p, mpiComm, &status);
         MPI_Get_count(&status, MPI_CHAR, &l);
         char* buf = new char[l];
         MPI_Recv(buf, l, MPI_CHAR, p, p, mpiComm, &status);
         std::string rowPartFromP(buf, l);
         out << rowPartFromP;
         delete[] buf;
      }
      if (lb.last) {
         assert(ixlow.last);
         assert(ub.last);
         assert(ixupp.last);
         sout << "---" << std::endl;
         writeLBltXltUBToStreamAllStringStream(sout, dynamic_cast<const DenseVector<double>&>(*lb.last),
               dynamic_cast<const DenseVector<double>&>(*ixlow.last), dynamic_cast<const DenseVector<double>&>(*ub.last),
               dynamic_cast<const DenseVector<double>&>(*ixupp.last));
      }
      sout << "----" << std::endl;
      out << sout.str();
   }
   else if (is_distributed) { // rank != 0
      for (size_t it = 0; it < lb.children.size(); it++)
         writeLBltXltUBtoStringStreamDenseChild(sout, *lb.children[it], *ixlow.children[it], *ub.children[it], *ixupp.children[it]);

      std::string str = sout.str();
      MPI_Ssend(str.c_str(), str.length(), MPI_CHAR, 0, my_rank, mpiComm);
   }

   if (is_distributed == 1)
      MPI_Barrier(mpiComm);
}

#endif
