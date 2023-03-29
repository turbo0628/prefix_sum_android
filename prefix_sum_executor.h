#pragma once

#include <cassert>
#include <iostream>
#include <vector>

#include <taichi/cpp/taichi.hpp>

namespace soft2d {

class PrefixSumExecutor {
  static int BLOCK_SZ;
  // static int GRID_SZ;

  int sorting_length_ = 0;
  ti::Runtime &runtime_;

  std::vector<int> ele_nums{};
  std::vector<int> ele_nums_pos{};

  ti::NdArray<int> large_arr;

  ti::Kernel k_blit_from_ndarray_to_ndarray_;
  ti::Kernel k_uniform_add_;
  ti::Kernel k_scan_add_inclusive_;

  ti::AotModule module_prefix_sum_;

public:
  explicit PrefixSumExecutor(int sorting_length, ti::Runtime &runtime);
  void Run(ti::NdArray<int> &input_arr);
};

} // namespace soft2d
