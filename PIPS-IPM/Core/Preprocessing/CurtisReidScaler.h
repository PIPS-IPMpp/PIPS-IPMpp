//
// Created by nils-christian on 18.06.21.
//

#ifndef PIPSIPMPP_CURTISREIDSCALER_H
#define PIPSIPMPP_CURTISREIDSCALER_H

#include "Scaler.hpp"
#include "DistributedVector.h"

class Problem;

class CurtisReidScaler : Scaler {
private:

protected:
   void doObjScaling() const override;

   void applyGeoMean(Vector<double>& maxvec, const Vector<double>& minvec);
   void postEquiScale();
public:

   CurtisReidScaler(const ProblemFactory& problem_factory, const Problem& problem, bool bitshifting = false);
   virtual ~CurtisReidScaler() = default;

   /** scale */
   void scale() override;
};

#endif //PIPSIPMPP_CURTISREIDSCALER_H
