/* PIPS-IPM - PYOMO wrapper and input                                 *
 * Author:  Cosmin G. Petra                                           *
 * (C) 2013 Argonne National Laboratory. See Copyright Notification.  */
#include <stdio.h>
#include <stdlib.h>

#include "PyomoInput.hpp"
#include "PIPSIPMppInterface.hpp"

#include "sFactoryAugSchurLeaf.h"
#include "MehrotraStochSolver.h"

#include <string>
#include <sstream>

int main(int argc, char** argv) {
   MPI_Init(&argc, &argv);
   int mype;
   MPI_Comm_rank(MPI_COMM_WORLD, &mype);

   if (argc < 3) {
      if (mype == 0)
         printf("Usage: %s [pyomo root name] [num scenarios] [outer solve (opt)] [inner solve (opt)]\n", argv[0]);
      return 1;
   }

   string datarootname(argv[1]);
   int nscen = atoi(argv[2]);

   int outerSolve = 2;
   if (argc >= 4) {
      outerSolve = atoi(argv[3]);
      if (mype == 0)
         cout << "Using option [" << outerSolve << "] for outer solve" << endl;
   }

   int innerSolve = 0;
   if (argc >= 5) {
      innerSolve = atoi(argv[4]);
      if (mype == 0)
         cout << "Using option [" << innerSolve << "] for inner solve" << endl;
   }

   if (mype == 0)
      cout << argv[0] << " starting ..." << endl;
   PyomoInput* s = new PyomoInput(datarootname, nscen);
   if (mype == 0)
      cout << "Pyomo input created from " << datarootname << endl;
   PIPSIPMppInterface<sFactoryAugSchurLeaf, MehrotraStochSolver> pipsIpm(*s);

   pipsipmpp_options::setIntParameter("OUTER_SOLVE", outerSolve);
   pipsipmpp_options::setIntParameter("INNER_SC_SOLVE", innerSolve);

   if (mype == 0)
      cout << "PIPSIPMppInterface created" << endl;
   delete s;
   if (mype == 0)
      cout << "PyomoInput deleted ... starting to solve" << endl;

   pipsIpm.go();

   MPI_Finalize();
   return 0;
}

