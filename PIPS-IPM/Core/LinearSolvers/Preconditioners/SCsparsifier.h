/*
 * SCsparsifier.h
 *
 *  Created on: 21.06.2019
 *      Author: bzfrehfe
 */

#ifndef PIPS_IPM_CORE_LINEARSOLVERS_PRECONDITIONERS_SCSPARSIFIER_H_
#define PIPS_IPM_CORE_LINEARSOLVERS_PRECONDITIONERS_SCSPARSIFIER_H_

#include "pipsport.h"
#include "sData.h"
#include <vector>

class sData;

const static double diagDomBounds[] = {   0.001,
										  0.0002,
										  0.000025,
										  0.000005,
										  0.000001   };

/** Schur complement sparsifier (based on comparison with diagonal entries) */
class SCsparsifier
{
   public:

      unsigned diagDomBoundsPosition;

      constexpr static double epsilonZero = 1e-15;

      SCsparsifier(MPI_Comm mpiComm = MPI_COMM_NULL);
      ~SCsparsifier();

      // returns sparsification bound
      double getDiagDomBound() const;

      // increases sparsification (more aggressive)
      void increaseDiagDomBound(bool& success);

      // decreases sparsification (less aggressive)
      void decreaseDiagDomBound(bool& success);

      // sets CSR column marker col of dominated local Schur complement (distributed) entries to -col
      void unmarkDominatedSCdistLocals(const sData& prob, SparseSymMatrix& sc) const;

      // resets unmarkDominatedSCdistEntries actions
      void resetSCdistEntries(SparseSymMatrix& sc) const;

      // deletes dominated Schur complement entries and converts matrix to Fortran format
      void getSparsifiedSC_fortran(const sData& prob, SparseSymMatrix& sc);

   private:

      MPI_Comm mpiComm;

      void updateDiagDomBound();
      std::vector<double> getDomDiagDist(const sData& prob, SparseSymMatrix& sc) const;

};


#endif /* PIPS_IPM_CORE_LINEARSOLVERS_PRECONDITIONERS_SCSPARSIFIER_H_ */
