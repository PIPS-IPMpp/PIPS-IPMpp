/*
 * StochPostsolver.h
 *
 *  Created on: 02.05.2019
 *      Author: Nils-Christian Kempke
 */

#ifndef PIPS_IPM_CORE_QPPREPROCESS_STOCHPOSTSOLVER_H_
#define PIPS_IPM_CORE_QPPREPROCESS_STOCHPOSTSOLVER_H_

#include <vector>
#include <memory>

#include "Postsolver.hpp"
#include "DistributedVector.h"
#include "DistributedProblem.hpp"
#include "DistributedVariables.h"
#include "SystemType.h"
#include "StochRowStorage.h"
#include "StochColumnStorage.h"

class StochPostsolver : public Postsolver {

public:
   StochPostsolver(const DistributedProblem& original_problem);
   ~StochPostsolver() override;

   void notifyRowModified(const INDEX& row);
   void notifyColModified(const INDEX& col);

   void notifySingletonEqualityRow(int node, int row, BlockType block_type, int col, double coeff, double rhs);
   void notifySingletonIneqalityRow(int node, int row, BlockType block_type, int col, double coeff, double lhs, double rhs);

   void notifySingletonRowBoundsTightened(const INDEX& row, const INDEX& col, double xlow_old, double xupp_old, double xlow_new, double xupp_new,
         double coeff);
   void notifyRedundantSide(const INDEX& row, bool is_upper_side, double lhs, double rhs);
   void notifyRedundantRow(const INDEX& row, int iclow, int icupp, double lhs, double rhs, const DistributedMatrix& matrix_row);
   void notifyFixedColumn(const INDEX& col, double value, double obj_coeff, const DistributedMatrix& eq_mat, const DistributedMatrix& ineq_mat);
   void notifyFixedEmptyColumn(const INDEX& col, double value, double obj_coeff, double xlow, double xupp);

   void putLinkingVarsSyncEvent();
   void putLinkingRowIneqSyncEvent();
   void putLinkingRowEqSyncEvent();
   void putBoundTighteningLinkingRowSyncEvent();

   void
   notifyFreeColumnSingletonEquality(const INDEX& row, const INDEX& col, double rhs, double obj_coeff, double col_coeff, double xlow, double xupp,
         const DistributedMatrix& matrix_row);
   void notifyFixedSingletonFromInequalityColumn(const INDEX& col, const INDEX& row, double value, double coeff, double xlow_old, double xupp_old);
   void notifyFreeColumnSingletonInequalityRow(const INDEX& row, const INDEX& col, double rhs, double coeff, double xlow, double xupp,
         const DistributedMatrix& matrix_row);

   void beginBoundTightening();
   void notifyRowPropagatedBound(const INDEX& row, const INDEX& col, double old_bound, double new_bound, bool is_upper_bound,
         const DistributedMatrix& matrix_row);
   void endBoundTightening(const std::vector<int>& store_linking_rows_A, const std::vector<int>& store_linking_rows_C, const DistributedMatrix& mat_A,
         const DistributedMatrix& mat_C);
   void notifyDeletedRow(SystemType system_type, int node, int row, bool linking_constraint);
   void notifyParallelColumns();

   void
   notifyNearlyParallelRowSubstitution(const INDEX& row1, const INDEX& row2, const INDEX& col1, const INDEX& col2, double scalar, double translation,
         double obj_col1, double obj_col2, double xlow_col2, double xupp_col2, double coeff_col1, double coeff_col2, double parallel_factor);
   void notifyNearlyParallelRowBoundsTightened(const INDEX& row1, const INDEX& row2, const INDEX& col1, const INDEX& col2, double xlow_col1,
         double xupp_col1, double xlow_col2, double xupp_col2, double coeff_col1, double coeff_col2, double scalar, double translation,
         double parallel_factor, double rhs, double clow, double cupp);

   void notifyParallelRowsBoundsTightened(const INDEX& row1, const INDEX& row2, double clow_old, double cupp_old, double clow_new, double cupp_new,
         double factor);

   void notifyTransformedInequalitiesIntoEqualties();

   [[nodiscard]] bool wasColumnRemoved(const INDEX& col) const;
   [[nodiscard]] bool wasRowRemoved(const INDEX& row) const;

private:
   void markColumnRemoved(const INDEX& col);
   void markColumnAdded(const INDEX& col);
   void markRowRemoved(const INDEX& row);
   void markRowAdded(const INDEX& row);

   /// stores row in specified node and returns it's new row index
   int storeRow(const INDEX& row, const DistributedMatrix& matrix_row);
   /// stores col in specified node and returns it's new col index
   int storeColumn(const INDEX& col, const DistributedMatrix& matrix_col_eq, const DistributedMatrix& matrix_col_ineq);

   [[nodiscard]] bool isRowModified(const INDEX& row) const;
   void markRowClean(const INDEX& row);
   void markColClean(const INDEX& col);
   [[nodiscard]] bool isColModified(const INDEX& col) const;

public:
   /// synchronization events

   PostsolveStatus postsolve(const Variables& reduced_solution, Variables& original_solution, TerminationStatus result_code) override;
private:

   const int my_rank{PIPS_MPIgetRank()};
   const bool distributed{PIPS_MPIgetDistributed()};

   double postsolve_tol;
   const double INF_NEG;
   const double INF_POS;

   bool transformed_inequalities_to_equalities{false};

   enum ReductionType {
      FIXED_COLUMN = 0,
      SUBSTITUTED_COLUMN = 1,
      STORE_BOUND_TIGHTENING_LINKING_ROWS = 2,
      REDUNDANT_SIDE = 3,
      REDUNDANT_ROW = 4,
      BOUNDS_TIGHTENED = 5,
      SINGLETON_EQUALITY_ROW = 6,
      SINGLETON_INEQUALITY_ROW = 7,
      FIXED_EMPTY_COLUMN = 8,
      FREE_COLUMN_SINGLETON_EQUALITY = 9,
      NEARLY_PARALLEL_ROW_SUBSTITUTION = 10,
      DELETED = 11,
      FIXED_COLUMN_SINGLETON_FROM_INEQUALITY = 12,
      FREE_COLUMN_SINGLETON_INEQUALITY_ROW = 13,
      PARALLEL_ROWS_BOUNDS_TIGHTENED = 14,
      NEARLY_PARALLEL_ROW_BOUNDS_TIGHTENED = 15,
      LINKING_INEQ_ROW_SYNC_EVENT = 16,
      LINKIN_EQ_ROW_SYNC_EVENT = 17,
      LINKING_VARS_SYNC_EVENT = 18,
      BOUND_TIGHTENING_LINKING_ROW_SYNC_EVENT = 19,
   };

   const unsigned int n_rows_original;
   const unsigned int n_cols_original;

   /// for now mapping will contain a dummy value for columns that have not been fixed and the value the columns has been fixed to otherwise
   /// 1 indicates that the row / col has not been removed from the problem - -1 indicates the row / col has been removed */
   std::unique_ptr<DistributedVector<int>> padding_origcol{};
   std::unique_ptr<DistributedVector<int>> padding_origrow_equality{};
   std::unique_ptr<DistributedVector<int>> padding_origrow_inequality{};

   /// has a row been modified since last storing it
   /// 1 if yes, -1 if not
   std::unique_ptr<DistributedVector<int>> eq_row_marked_modified{};
   std::unique_ptr<DistributedVector<int>> ineq_row_marked_modified{};
   /// has a column been modified
   std::unique_ptr<DistributedVector<int>> column_marked_modified{};

   /// vectors for storing ints and doubles containting information needed by postsolve
   std::vector<ReductionType> reductions;
   std::vector<INDEX> indices;
   std::vector<unsigned int> start_idx_indices;

   std::vector<double> float_values;
   std::vector<int> int_values;

   std::vector<unsigned int> start_idx_float_values;
   std::vector<unsigned int> start_idx_int_values;

   StochRowStorage row_storage;

   StochColumnStorage col_storage;

   /// stores the index for a row/col indicating where in stored_rows/cols that row/col was stored last
   std::unique_ptr<DistributedVector<int>> eq_row_stored_last_at{};
   std::unique_ptr<DistributedVector<int>> ineq_row_stored_last_at{};
   std::unique_ptr<DistributedVector<int>> col_stored_last_at{};

   /// stores which reduction is last bound-tightening on variable
   std::unique_ptr<DistributedVector<int>> last_upper_bound_tightened{};
   std::unique_ptr<DistributedVector<int>> last_lower_bound_tightened{};

   /// stuff for synchronization in-between processes
   const int length_array_outdated_indicators;
   bool* array_outdated_indicators;

   /* local changes in linking variables */
   bool& outdated_linking_vars;
   std::vector<double> array_linking_var_changes;
   std::unique_ptr<DenseVector<double>> x_changes{};
   std::unique_ptr<DenseVector<double>> v_changes{};
   std::unique_ptr<DenseVector<double>> w_changes{};
   std::unique_ptr<DenseVector<double>> gamma_changes{};
   std::unique_ptr<DenseVector<double>> phi_changes{};

   /* local changes in equality linking rows */
   bool& outdated_equality_linking_rows;
   std::vector<double> array_eq_linking_row_changes;
   std::unique_ptr<DenseVector<double>> y_changes{};

   /* local changes in inequality rows */
   bool& outdated_inequality_linking_rows;
   std::vector<double> array_ineq_linking_row_changes;
   std::unique_ptr<DenseVector<double>> z_changes{};
   std::unique_ptr<DenseVector<double>> s_changes{};
   std::unique_ptr<DenseVector<double>> t_changes{};
   std::unique_ptr<DenseVector<double>> u_changes{};

   void finishNotify();

/// postsolve operations
   bool postsolveRedundantSide(DistributedVariables& original_vars, int reduction_idx) const;
   bool postsolveRedundantRow(DistributedVariables& original_vars, int reduction_idx);
   bool postsolveBoundsTightened(DistributedVariables& original_vars, int reduction_idx);
   bool postsolveFixedColumn(DistributedVariables& original_vars, int reduction_idx);
   bool postsolveFixedEmptyColumn(DistributedVariables& original_vars, int reduction_idx);
   bool postsolveFixedColumnSingletonFromInequality(DistributedVariables& original_vars, int reduction_idx);
   bool postsolveSingletonEqualityRow(DistributedVariables& original_vars, int reduction_idx) const;
   bool postsolveSingletonInequalityRow(DistributedVariables& original_vars, int reduction_idx) const;
   bool postsolveNearlyParallelRowSubstitution(DistributedVariables& original_vars, int reduction_idx);
   bool postsolveNearlyParallelRowBoundsTightened(DistributedVariables& original_vars, int reduction_idx);
   bool postsolveParallelRowsBoundsTightened(DistributedVariables& original_vars, int reduction_idx) const;
   bool postsolveFreeColumnSingletonEquality(DistributedVariables& original_vars, int reduction_idx);
   bool postsolveFreeColumnSingletonInequalityRow(DistributedVariables& original_vars, int reduction_idx);

   bool syncLinkingVarChanges(DistributedVariables& original_vars);
   bool syncEqLinkingRowChanges(DistributedVariables& original_vars);
   bool syncIneqLinkingRowChanges(DistributedVariables& original_vars);
   bool syncLinkingRowsAfterBoundTightening(DistributedVariables& original_vars, int i);
   int findNextRowInStored(int pos_reduction, unsigned int& start, const INDEX& row) const;

   void addIneqRowDual(double& z, double& lambda, double& pi, double value) const;

   void setOriginalVarsFromReduced(const DistributedVariables& reduced_vars, DistributedVariables& original_vars) const;
   [[nodiscard]] bool allVariablesSet(const DistributedVariables& vars) const;
   [[nodiscard]] bool complementarySlackVariablesMet(const DistributedVariables& vars, const INDEX& col, double tol) const;
   [[nodiscard]] bool complementarySlackRowMet(const DistributedVariables& vars, const INDEX& row, double tol) const;

   [[nodiscard]] bool sameNonZeroPatternDistributed(const DistributedVector<double>& svec) const; // TODO: move
   static bool sameNonZeroPatternDistributed(const DenseVector<double>& vec) ; // TODO: move

   template<typename T>
   void setOriginalValuesFromReduced(DistributedVector<T>& original_vector, const DistributedVector<T>& reduced_vector,
         const DistributedVector<int>& padding_original) const;

   template<typename T>
   void setOriginalValuesFromReduced(DenseVector<T>& original_vector, const DenseVector<T>& reduced_vector,
         const DenseVector<int>& padding_original) const;

   template<typename T>
   void set_original_ineq_eq_tranformed_values_from_reduced(DistributedVector<T>& original_first, DistributedVector<T>& original_last,
      const DistributedVector<T>& reduced, const DistributedVector<int>& padding_original_first, const DistributedVector<int>& padding_original_last, bool mixed_row_column) const;

   template<typename T>
   void set_original_ineq_eq_tranformed_values_from_reduced(DenseVector<T>& original_first, DenseVector<T>& original_last,
      const DenseVector<T>& reduced, const DenseVector<int>& padding_original_first, const DenseVector<int>& padding_original_last) const;

   template<typename T>
   void set_original_ineq_eq_tranformed_values_from_reduced(DenseVector<T>& original_first, DenseVector<T>& original_second, DenseVector<T>& original_third,
      const DenseVector<T>& reduced, const DenseVector<int>& padding_original_first, const DenseVector<int>& padding_original_second, const DenseVector<int>& padding_original_third) const;

};


#endif /* PIPS_IPM_CORE_QPPREPROCESS_STOCHPOSTSOLVER_H_ */
