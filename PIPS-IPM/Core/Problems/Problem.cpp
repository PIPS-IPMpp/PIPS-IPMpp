#include <SimpleVector.hpp>
#include "Problem.hpp"

Problem::Problem(std::shared_ptr<Vector<double>> g_in, std::shared_ptr<Vector<double>> xlow_in,
   std::shared_ptr<Vector<double>> ixlow_in, std::shared_ptr<Vector<double>> xupp_in,
   std::shared_ptr<Vector<double>> ixupp_in,
   std::shared_ptr<GeneralMatrix> A_in, std::shared_ptr<Vector<double>> bA_in, std::shared_ptr<GeneralMatrix> C_in,
   std::shared_ptr<Vector<double>> clow_in, std::shared_ptr<Vector<double>> iclow_in,
   std::shared_ptr<Vector<double>> cupp_in, std::shared_ptr<Vector<double>> icupp_in) : A{std::move(A_in)},
   C{std::move(C_in)},
   g{std::move(g_in)}, bA{std::move(bA_in)}, primal_upper_bounds{std::move(xupp_in)}, primal_upper_bound_indicators{std::move(ixupp_in)},
   primal_lower_bounds{std::move(xlow_in)},
   primal_lower_bound_indicators{std::move(ixlow_in)}, inequality_upper_bounds{std::move(cupp_in)}, inequality_upper_bound_indicators{std::move(icupp_in)}, inequality_lower_bounds{std::move(clow_in)},
   inequality_lower_bound_indicators{std::move(iclow_in)},
   nx{g->length()}, my{A->n_rows()}, mz{C->n_rows()}, number_primal_lower_bounds{primal_lower_bound_indicators->number_nonzeros()},
   number_primal_upper_bounds{primal_upper_bound_indicators->number_nonzeros()},
   number_inequality_lower_bounds{inequality_lower_bound_indicators->number_nonzeros()}, number_inequality_upper_bounds{inequality_upper_bound_indicators->number_nonzeros()} {
   assert(primal_lower_bound_indicators && primal_upper_bound_indicators && inequality_lower_bound_indicators && inequality_upper_bound_indicators);
}

void Problem::Amult(double beta, Vector<double>& y, double alpha, const Vector<double>& x) const {
   A->mult(beta, y, alpha, x);
}

void Problem::Cmult(double beta, Vector<double>& y, double alpha, const Vector<double>& x) const {
   C->mult(beta, y, alpha, x);
}

void Problem::ATransmult(double beta, Vector<double>& y, double alpha, const Vector<double>& x) const {
   A->transMult(beta, y, alpha, x);
}

void Problem::CTransmult(double beta, Vector<double>& y, double alpha, const Vector<double>& x) const {
   C->transMult(beta, y, alpha, x);
}

void Problem::getg(Vector<double>& myG) const {
   myG.copyFrom(*g);
}

void Problem::getbA(Vector<double>& bout) const {
   bout.copyFrom(*bA);
}

void Problem::putAIntoAt(GeneralMatrix& M, int row, int col) {
   M.atPutSubmatrix(row, col, *A, 0, 0, my, nx);
}

void Problem::putAIntoAt(SymmetricMatrix& M, int row, int col) {
   M.symAtPutSubmatrix(row, col, *A, 0, 0, my, nx);
}

void Problem::putCIntoAt(GeneralMatrix& M, int row, int col) {
   M.atPutSubmatrix(row, col, *C, 0, 0, mz, nx);
}

void Problem::putCIntoAt(SymmetricMatrix& M, int row, int col) {
   M.symAtPutSubmatrix(row, col, *C, 0, 0, mz, nx);
}

void Problem::scaleA() {
   A->columnScale(*sc);
}

void Problem::scaleC() {
   C->columnScale(*sc);
}

void Problem::scaleg() {
   auto& scVector = dynamic_cast<SimpleVector<double>&>(*sc);
   assert (scVector.length() == g->length());

   // D * g
   g->componentMult(scVector);
}

void Problem::scalexupp() {
   auto& scVector = dynamic_cast<SimpleVector<double>&>(*sc);

   assert (scVector.length() == primal_upper_bounds->length());

   // inverse(D) * bux
   primal_upper_bounds->componentDiv(scVector);

}


void Problem::scalexlow() {
   auto& scVector = dynamic_cast<SimpleVector<double>&>(*sc);

   assert (scVector.length() == primal_lower_bounds->length());

   // inverse(D) * blx
   primal_lower_bounds->componentDiv(scVector);

}

void Problem::flipg() {
   // Multiply C matrix by -1
   g->scalarMult(-1.0);
}

double Problem::datanorm() const {
   double norm = 0.0;
   double componentNorm;

   componentNorm = g->inf_norm();
   if (componentNorm > norm)
      norm = componentNorm;

   componentNorm = bA->inf_norm();
   if (componentNorm > norm)
      norm = componentNorm;

   componentNorm = A->inf_norm();
   if (componentNorm > norm)
      norm = componentNorm;

   componentNorm = C->inf_norm();
   if (componentNorm > norm)
      norm = componentNorm;

   assert(primal_lower_bounds->matchesNonZeroPattern(*primal_lower_bound_indicators));
   componentNorm = primal_lower_bounds->inf_norm();
   if (componentNorm > norm)
      norm = componentNorm;

   assert(primal_upper_bounds->matchesNonZeroPattern(*primal_upper_bound_indicators));
   componentNorm = primal_upper_bounds->inf_norm();
   if (componentNorm > norm)
      norm = componentNorm;

   assert(inequality_lower_bounds->matchesNonZeroPattern(*inequality_lower_bound_indicators));
   componentNorm = inequality_lower_bounds->inf_norm();
   if (componentNorm > norm)
      norm = componentNorm;

   assert(inequality_upper_bounds->matchesNonZeroPattern(*inequality_upper_bound_indicators));
   componentNorm = inequality_upper_bounds->inf_norm();
   if (componentNorm > norm)
      norm = componentNorm;

   return norm;
}

void Problem::print() {
   std::cout << "begin c\n";
   g->write_to_stream(std::cout);
   std::cout << "end c\n";

   std::cout << "begin xlow\n";
   primal_lower_bounds->write_to_stream(std::cout);
   std::cout << "end xlow\n";
   std::cout << "begin ixlow\n";
   primal_lower_bound_indicators->write_to_stream(std::cout);
   std::cout << "end ixlow\n";

   std::cout << "begin xupp\n";
   primal_upper_bounds->write_to_stream(std::cout);
   std::cout << "end xupp\n";
   std::cout << "begin ixupp\n";
   primal_upper_bound_indicators->write_to_stream(std::cout);
   std::cout << "end ixupp\n";
   std::cout << "begin A\n";

   A->write_to_stream(std::cout);
   std::cout << "end A\n";
   std::cout << "begin b\n";
   bA->write_to_stream(std::cout);
   std::cout << "end b\n";
   std::cout << "begin C\n";
   C->write_to_stream(std::cout);
   std::cout << "end C\n";

   std::cout << "begin clow\n";
   inequality_lower_bounds->write_to_stream(std::cout);
   std::cout << "end clow\n";
   std::cout << "begin iclow\n";
   inequality_lower_bound_indicators->write_to_stream(std::cout);
   std::cout << "end iclow\n";

   std::cout << "begin cupp\n";
   inequality_upper_bounds->write_to_stream(std::cout);
   std::cout << "end cupp\n";
   std::cout << "begin icupp\n";
   inequality_upper_bound_indicators->write_to_stream(std::cout);
   std::cout << "end icupp\n";
}