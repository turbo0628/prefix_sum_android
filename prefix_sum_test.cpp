#include "prefix_sum_executor.h"

int main()
{
  TiArch arch;
  arch = TiArch::TI_ARCH_VULKAN;
  ti::Runtime runtime(arch);
  constexpr int N = 300;
  ti::NdArray<int> arr =
     runtime.allocate_ndarray<int>(std::vector<uint32_t>{N}, {}, true);
  std::vector<int> arr_host;
  for (int i = 0; i < N; ++i)
    arr_host.push_back(i);
  arr.write(arr_host);
  soft2d::PrefixSumExecutor executor(N, runtime);
  executor.Run(arr);
  runtime.wait();
  arr.read(arr_host);
  for (size_t i = 0; i < arr_host.size(); ++i)
  {
   printf("%d vs %d\n", arr_host[i], i * (i + 1) / 2);
   assert(arr_host[i] == i * (i + 1) / 2);
  }
  printf("Good!\n");
}