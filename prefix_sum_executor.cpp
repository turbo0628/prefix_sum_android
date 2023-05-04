#include "prefix_sum_executor.h"

using namespace soft2d;

int PrefixSumExecutor::BLOCK_SZ = 64;

PrefixSumExecutor::PrefixSumExecutor(int sorting_length, ti::Runtime &runtime)
    : sorting_length_(sorting_length), runtime_(runtime) {

  module_prefix_sum_ = runtime_.load_aot_module(
      "prefix_sum_module");

  k_blit_from_ndarray_to_ndarray_ =
      module_prefix_sum_.get_kernel("blit_from_ndarray_to_ndarray");
  k_uniform_add_ = module_prefix_sum_.get_kernel("uniform_add");
  k_scan_add_inclusive_ = module_prefix_sum_.get_kernel("scan_add_inclusive");

  int ele_num = sorting_length;
  ele_nums.push_back(ele_num);
  int start_pos = 0;
  ele_nums_pos.push_back(start_pos);

  while (ele_num > 1) {
    ele_num = (ele_num + BLOCK_SZ - 1) / BLOCK_SZ;
    ele_nums.push_back(ele_num);
    start_pos += BLOCK_SZ * ele_num;
    ele_nums_pos.push_back(start_pos);
  }

  large_arr = runtime.allocate_ndarray<int>(
      std::vector<uint32_t>{(uint32_t)start_pos}, {});
}

void PrefixSumExecutor::Run(ti::NdArray<int> &input_arr) {

  if (input_arr.elem_type() != TI_DATA_TYPE_I32) {
    assert(0 && "Only ti.i32 type is supported for prefix sum.");
  }

  // blit_from_ndarray_to_ndarray()
  k_blit_from_ndarray_to_ndarray_.clear_args();
  k_blit_from_ndarray_to_ndarray_.push_arg(large_arr);
  k_blit_from_ndarray_to_ndarray_.push_arg(input_arr);
  k_blit_from_ndarray_to_ndarray_.push_arg(0);
  k_blit_from_ndarray_to_ndarray_.push_arg(sorting_length_);
  k_blit_from_ndarray_to_ndarray_.launch();

  for (int i = 0; i < ele_nums.size() - 1; ++i) {
    if (i == ele_nums.size() - 2) {
      // scan_add_inclusive()
      k_scan_add_inclusive_.clear_args();
      k_scan_add_inclusive_.push_arg(large_arr);
      k_scan_add_inclusive_.push_arg(ele_nums_pos[i]);
      k_scan_add_inclusive_.push_arg(ele_nums_pos[i + 1]);
      k_scan_add_inclusive_.push_arg(1);
      k_scan_add_inclusive_.launch();
    } else {
      // scan_add_inclusive()
      k_scan_add_inclusive_.clear_args();
      k_scan_add_inclusive_.push_arg(large_arr);
      k_scan_add_inclusive_.push_arg(ele_nums_pos[i]);
      k_scan_add_inclusive_.push_arg(ele_nums_pos[i + 1]);
      k_scan_add_inclusive_.push_arg(0);
      k_scan_add_inclusive_.launch();
    }
  }

  for (int i = ele_nums.size() - 3; i >= 0; --i) {
    // uniform_add()
    k_uniform_add_.clear_args();
    k_uniform_add_.push_arg(large_arr);
    k_uniform_add_.push_arg(ele_nums_pos[i]);
    k_uniform_add_.push_arg(ele_nums_pos[i + 1]);
    k_uniform_add_.launch();
  }

  // blit_from_ndarray_to_ndarray()
  k_blit_from_ndarray_to_ndarray_.clear_args();
  k_blit_from_ndarray_to_ndarray_.push_arg(input_arr);
  k_blit_from_ndarray_to_ndarray_.push_arg(large_arr);
  k_blit_from_ndarray_to_ndarray_.push_arg(0);
  k_blit_from_ndarray_to_ndarray_.push_arg(sorting_length_);
  k_blit_from_ndarray_to_ndarray_.launch();
}
