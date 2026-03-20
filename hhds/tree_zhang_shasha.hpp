#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "tree.hpp"

namespace hhds {

template <typename X>
struct ZhangShashaUnitCosts {
  int operator()(const X& lhs, const X& rhs) const { return lhs == rhs ? 0 : 1; }
};

namespace detail {

template <typename X>
struct PostorderIndex {
  std::vector<Tree_pos>              post;          // 1-based: post[i] is node id
  std::unordered_map<Tree_pos, int>  post_index;    // node id -> postorder index
  std::vector<int>                   leftmost_desc; // 1-based
  std::vector<int>                   keyroots;
};

template <typename X>
auto build_postorder_index(const tree<X>& t, Tree_pos root) -> PostorderIndex<X> {
  PostorderIndex<X> out;
  out.post.push_back(INVALID);  // 1-based indexing sentinel

  // Iterative postorder DFS.
  std::vector<std::pair<Tree_pos, bool>> st;
  st.emplace_back(root, false);
  while (!st.empty()) {
    auto [n, seen] = st.back();
    st.pop_back();
    if (n == INVALID) {
      continue;
    }
    if (!seen) {
      st.emplace_back(n, true);

      std::vector<Tree_pos> children;
      for (Tree_pos c = t.get_first_child(n); c != INVALID; c = t.get_sibling_next(c)) {
        children.push_back(c);
      }
      for (auto it = children.rbegin(); it != children.rend(); ++it) {
        st.emplace_back(*it, false);
      }
      continue;
    }
    out.post.push_back(n);
  }

  const int n = static_cast<int>(out.post.size()) - 1;
  out.leftmost_desc.assign(static_cast<size_t>(n) + 1U, 0);
  out.post_index.reserve(static_cast<size_t>(n) * 2U + 1U);

  for (int i = 1; i <= n; ++i) {
    out.post_index[out.post[static_cast<size_t>(i)]] = i;
  }

  for (int i = 1; i <= n; ++i) {
    Tree_pos cur = out.post[static_cast<size_t>(i)];
    while (true) {
      Tree_pos fc = t.get_first_child(cur);
      if (fc == INVALID) {
        break;
      }
      cur = fc;
    }
    out.leftmost_desc[static_cast<size_t>(i)] = out.post_index[cur];
  }

  std::vector<int> last_for_lmd(static_cast<size_t>(n) + 1U, 0);
  for (int i = 1; i <= n; ++i) {
    last_for_lmd[static_cast<size_t>(out.leftmost_desc[static_cast<size_t>(i)])] = i;
  }
  for (int i = 1; i <= n; ++i) {
    if (last_for_lmd[static_cast<size_t>(i)] != 0) {
      out.keyroots.push_back(last_for_lmd[static_cast<size_t>(i)]);
    }
  }

  return out;
}

}  // namespace detail

template <typename X, typename RenameCost = ZhangShashaUnitCosts<X>>
auto zhang_shasha_distance(const tree<X>& t1, Tree_pos root1, const tree<X>& t2, Tree_pos root2, RenameCost rename_cost = RenameCost{},
                           int insert_cost = 1, int delete_cost = 1) -> int {
  const auto a = detail::build_postorder_index(t1, root1);
  const auto b = detail::build_postorder_index(t2, root2);

  const int n = static_cast<int>(a.post.size()) - 1;
  const int m = static_cast<int>(b.post.size()) - 1;
  if (n == 0) {
    return m * insert_cost;
  }
  if (m == 0) {
    return n * delete_cost;
  }

  std::vector<int> treedist(static_cast<size_t>(n + 1) * static_cast<size_t>(m + 1), 0);
  auto td = [&](int i, int j) -> int& { return treedist[static_cast<size_t>(i) * static_cast<size_t>(m + 1) + static_cast<size_t>(j)]; };

  for (int i_key : a.keyroots) {
    for (int j_key : b.keyroots) {
      const int i0 = a.leftmost_desc[static_cast<size_t>(i_key)] - 1;
      const int j0 = b.leftmost_desc[static_cast<size_t>(j_key)] - 1;
      const int rows = i_key - i0 + 1;
      const int cols = j_key - j0 + 1;

      std::vector<int> forestdist(static_cast<size_t>(rows) * static_cast<size_t>(cols), 0);
      auto fd = [&](int i, int j) -> int& {
        return forestdist[static_cast<size_t>(i - i0) * static_cast<size_t>(cols) + static_cast<size_t>(j - j0)];
      };

      fd(i0, j0) = 0;
      for (int i = i0 + 1; i <= i_key; ++i) {
        fd(i, j0) = fd(i - 1, j0) + delete_cost;
      }
      for (int j = j0 + 1; j <= j_key; ++j) {
        fd(i0, j) = fd(i0, j - 1) + insert_cost;
      }

      for (int i = i0 + 1; i <= i_key; ++i) {
        for (int j = j0 + 1; j <= j_key; ++j) {
          const int li = a.leftmost_desc[static_cast<size_t>(i)];
          const int lj = b.leftmost_desc[static_cast<size_t>(j)];

          if (li == i0 + 1 && lj == j0 + 1) {
            const int relabel = rename_cost(t1.get_data(a.post[static_cast<size_t>(i)]), t2.get_data(b.post[static_cast<size_t>(j)]));
            fd(i, j) = std::min({fd(i - 1, j) + delete_cost, fd(i, j - 1) + insert_cost, fd(i - 1, j - 1) + relabel});
            td(i, j) = fd(i, j);
          } else {
            fd(i, j) = std::min({fd(i - 1, j) + delete_cost, fd(i, j - 1) + insert_cost, fd(li - 1, lj - 1) + td(i, j)});
          }
        }
      }
    }
  }

  return td(n, m);
}

template <typename X, typename RenameCost = ZhangShashaUnitCosts<X>>
auto zhang_shasha_distance(const tree<X>& t1, const tree<X>& t2, RenameCost rename_cost = RenameCost{}, int insert_cost = 1, int delete_cost = 1)
    -> int {
  return zhang_shasha_distance(t1, t1.get_root(), t2, t2.get_root(), std::move(rename_cost), insert_cost, delete_cost);
}

}  // namespace hhds

