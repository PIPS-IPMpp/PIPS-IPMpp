//
// Created by nils-christian on 25.05.21.
//

#include "sort.h"
#include <climits>

void doubleLexSort(int first[], int n, int second[], double data[]) {
   int fi, se, j, k, kinc, inc;
   double dtemp;
   const int incs[] = {1, 5, 19, 41, 109, 209, 505,
      929, 2161, 3905, 8929, 16001, INT_MAX};

   for (k = 0; incs[k] <= n / 2; k++);

   kinc = k - 1;
   // incs[kinc] is the greatest value in the sequence that is also less
   // than or equal to n/2.
   // If n == {0,1}, kinc == -1 and so no sort will take place.

   for (; kinc >= 0; kinc--) {
      // Loop over all increments
      inc = incs[kinc];

      for (k = inc; k < n; k++) {
         dtemp = data[k];
         fi = first[k];
         se = second[k];
         for (j = k; j >= inc; j -= inc) {
            if (fi < first[j - inc] ||
               (fi == first[j - inc] &&
                  se < second[j - inc])) {
               data[j] = data[j - inc];
               first[j] = first[j - inc];
               second[j] = second[j - inc];
            } else {
               break;
            }
         }
         data[j] = dtemp;
         first[j] = fi;
         second[j] = se;
      }
   } // End loop over all increments
}

