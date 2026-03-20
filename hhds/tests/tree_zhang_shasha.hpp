#include <gtest/gtest.h>

#include "tree.hpp"
#include "tree_zhang_shasha.hpp"

namespace {

hhds::tree<int> make_base_tree() {
  hhds::tree<int> t;
  const auto      r  = t.add_root(1);
  const auto      c1 = t.add_child(r, 2);
  const auto      c2 = t.add_child(r, 3);
  (void)t.add_child(c1, 4);
  (void)c2;
  return t;
}

}  // namespace

TEST(TreeZhangShasha, IdenticalTreesDistanceZero) {
  const auto t1 = make_base_tree();
  const auto t2 = make_base_tree();
  EXPECT_EQ(hhds::zhang_shasha_distance(t1, t2), 0);
}

TEST(TreeZhangShasha, RenameCostOneForDifferentLabels) {
  hhds::tree<int> t1;
  hhds::tree<int> t2;
  (void)t1.add_root(10);
  (void)t2.add_root(11);

  EXPECT_EQ(hhds::zhang_shasha_distance(t1, t2), 1);
}

TEST(TreeZhangShasha, InsertAndDeleteLeafCostOne) {
  hhds::tree<int> t1;
  hhds::tree<int> t2;
  const auto      r1 = t1.add_root(7);
  const auto      r2 = t2.add_root(7);
  (void)r1;
  (void)t2.add_child(r2, 9);

  EXPECT_EQ(hhds::zhang_shasha_distance(t1, t2), 1);
  EXPECT_EQ(hhds::zhang_shasha_distance(t2, t1), 1);
}

TEST(TreeZhangShasha, DeletingInternalChainCostsTwo) {
  hhds::tree<int> t1;
  hhds::tree<int> t2;
  const auto      r1 = t1.add_root(1);
  const auto      n1 = t1.add_child(r1, 2);
  (void)t1.add_child(n1, 3);  // chain length 2 under root
  (void)t2.add_root(1);

  EXPECT_EQ(hhds::zhang_shasha_distance(t1, t2), 2);
}

TEST(TreeZhangShasha, SubtreeRootsOverloadWorks) {
  hhds::tree<int> t1;
  hhds::tree<int> t2;

  const auto r1 = t1.add_root(100);
  const auto a1 = t1.add_child(r1, 1);
  (void)t1.add_child(a1, 2);

  const auto r2 = t2.add_root(999);
  const auto a2 = t2.add_child(r2, 1);
  (void)t2.add_child(a2, 2);

  EXPECT_EQ(hhds::zhang_shasha_distance(t1, a1, t2, a2), 0);
}

TEST(TreeZhangShasha, CustomRenameCostIsUsed) {
  struct RenameAbsCost {
    int operator()(int lhs, int rhs) const { return lhs > rhs ? lhs - rhs : rhs - lhs; }
  };

  hhds::tree<int> t1;
  hhds::tree<int> t2;
  (void)t1.add_root(1);
  (void)t2.add_root(4);

  EXPECT_EQ(hhds::zhang_shasha_distance(t1, t2, RenameAbsCost{}), 3);
}

