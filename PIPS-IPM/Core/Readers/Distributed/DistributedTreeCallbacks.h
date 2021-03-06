/* PIPS-IPM                                                           *
 * Author:  Cosmin G. Petra                                           *
 * (C) 2012 Argonne National Laboratory. See Copyright Notification.  */
#ifndef STOCH_TREE_CALLBACKS
#define STOCH_TREE_CALLBACKS

#include "DistributedTree.h"
#include "DistributedInputTree.h"

#include <memory>
#include <functional>

/** This class creates objects when  the problem is specified by C callbacks.
 *  Obsolete and present only to ensure compatibility with older versions of the code.
 *  The new sTree implementation, C++-like is sTreeImpl.
 */

class DistributedProblem;

class DistributedTreeCallbacks : public DistributedTree {
public:
   using InputNode = DistributedInputTree::DistributedInputNode;

   using DATA_MAT = FMAT InputNode::*;
   using DATA_VEC = FVEC InputNode::*;
   using DATA_NNZ = FNNZ InputNode::*;
   using DATA_INT = int InputNode::*;
   using TREE_SIZE = long long DistributedTree::*;
   [[nodiscard]] std::unique_ptr<DistributedTree> clone() const override;

   DistributedTreeCallbacks();
   explicit DistributedTreeCallbacks(DistributedInputTree* root);
   DistributedTreeCallbacks(const DistributedTreeCallbacks& other);

   ~DistributedTreeCallbacks() override = default;

   void addChild(std::unique_ptr<DistributedTree> child);

   [[nodiscard]] std::unique_ptr<DistributedSymmetricMatrix> createQ() const override;

   [[nodiscard]] std::unique_ptr<DistributedMatrix> createA() const override;
   [[nodiscard]] std::unique_ptr<DistributedMatrix> createC() const override;

   [[nodiscard]] std::unique_ptr<DistributedVector<double>> createc() const override;

   [[nodiscard]] std::unique_ptr<DistributedVector<double>> createxlow() const override;
   [[nodiscard]] std::unique_ptr<DistributedVector<double>> createixlow() const override;
   [[nodiscard]] std::unique_ptr<DistributedVector<double>> createxupp() const override;
   [[nodiscard]] std::unique_ptr<DistributedVector<double>> createixupp() const override;

   [[nodiscard]] std::unique_ptr<DistributedVector<double>> create_variable_integrality_type() const override;

   [[nodiscard]] std::unique_ptr<DistributedVector<double>> createb() const override;
   [[nodiscard]] std::unique_ptr<DistributedVector<double>> createclow() const override;
   [[nodiscard]] std::unique_ptr<DistributedVector<double>> createiclow() const override;
   [[nodiscard]] std::unique_ptr<DistributedVector<double>> createcupp() const override;
   [[nodiscard]] std::unique_ptr<DistributedVector<double>> createicupp() const override;


   [[nodiscard]] int nx() const override;
   [[nodiscard]] int my() const override;
   [[nodiscard]] int myl() const override;
   [[nodiscard]] int mz() const override;
   [[nodiscard]] int mzl() const override;
   [[nodiscard]] int id() const override;

   void computeGlobalSizes() override;

   virtual void switchToPresolvedData();
   virtual void switchToOriginalData();
   virtual bool isPresolved();
   virtual bool hasPresolved();
   virtual void initPresolvedData(const DistributedProblem& presolved_data);

   virtual void writeSizes(std::ostream& sout) const;

   std::unique_ptr<DistributedTree> switchToHierarchicalTree(DistributedProblem*& data_to_split, std::unique_ptr<DistributedTree> pointer_to_this) override;

   [[nodiscard]] const std::vector<unsigned int>& getMapBlockSubTrees() const { return map_node_sub_root; };
   [[nodiscard]] std::vector<MPI_Comm> getChildComms() const;

   void assertTreeStructureCorrect() const;
protected:
   void assertTreeStructureChildren() const;
   void assertSubRoot() const;
   void assertTreeStructureIsNotMyNode() const;
   void assertTreeStructureIsMyNodeChildren() const;
   void assertTreeStructureIsMyNodeSubRoot() const;
   void assertTreeStructureIsMyNode() const;

   unsigned int getMapChildrenToNthRootSubTrees(int& take_nth_root, std::vector<unsigned int>& map_child_to_sub_tree, unsigned int n_children,
         unsigned int n_procs, const std::vector<unsigned int>& child_procs);

   void initPresolvedData(const DistributedSymmetricMatrix& Q, const DistributedMatrix& A, const DistributedMatrix& C, const DistributedVector<double>& nxVec,
         const DistributedVector<double>& myVec, const DistributedVector<double>& mzVec, int mylParent, int mzlParent);

   [[nodiscard]] std::unique_ptr<DistributedMatrix>
   createMatrix(TREE_SIZE my, TREE_SIZE myl, DATA_INT m_ABmat, DATA_INT n_Mat, DATA_INT nnzAmat, DATA_NNZ fnnzAmat, DATA_MAT Amat, DATA_INT nnzBmat,
         DATA_NNZ fnnzBmat, DATA_MAT Bmat, DATA_INT m_Blmat, DATA_INT nnzBlmat, DATA_NNZ fnnzBlmat, DATA_MAT Blmat,
         const std::string& prefix_for_print) const;
   [[nodiscard]] std::unique_ptr<DistributedVector<double>> createVector(DATA_INT n_vec, DATA_VEC vec, DATA_INT n_linking_vec, DATA_VEC linking_vec) const;

   void createSubcommunicatorsAndChildren(int& take_nth_root, std::vector<unsigned int>& map_child_to_sub_tree);
   void countTwoLinksForChildTrees(const std::vector<int>& two_links_start_in_child_A, const std::vector<int>& two_links_start_in_child_C,
         std::vector<unsigned int>& two_links_children_eq, std::vector<unsigned int>& two_links_children_ineq, unsigned int& two_links_root_eq,
         unsigned int& two_links_root_ineq) const;

   void adjust_active_size_by(int DistributedTreeCallbacks::* active_size_to_adjust, long long DistributedTree::* glob_size_to_adjust, int adjustment);
   std::pair<int, int>
   adjustSizesAfterSplit(const std::vector<unsigned int>& two_links_children_eq, const std::vector<unsigned int>& two_links_children_ineq);

   std::pair<int, int> splitTree(int n_layers, DistributedProblem* data_to_split) override;

   [[nodiscard]] std::unique_ptr<DistributedTree> shaveDenseBorder(int nx_to_shave, int myl_to_shave, int mzl_to_shave, std::unique_ptr<DistributedTree> pointer_to_this) override;

   /* inactive sizes store the original state of the tree when switching to the presolved data */
   long long N_INACTIVE{-1};
   long long MY_INACTIVE{-1};
   long long MZ_INACTIVE{-1};
   long long MYL_INACTIVE{-1};
   long long MZL_INACTIVE{-1};


   int nx_active{0};
   int my_active{0};
   int mz_active{0};
   int myl_active{0};
   int mzl_active{0};

   int nx_inactive{-1};
   int my_inactive{-1};
   int mz_inactive{-1};
   int myl_inactive{-1};
   int mzl_inactive{-1};

   bool is_data_presolved{false};
   bool has_presolved_data{false};
   bool print_tree_sizes_on_reading{false};

   /* after this node has been split this will indicate how the children were assigned to the (new) sub_roots */
   std::vector<unsigned int> map_node_sub_root;

   InputNode* data{}; //input data
};


#endif
