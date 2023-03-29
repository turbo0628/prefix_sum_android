import taichi as ti
from taichi.lang.simt import block, subgroup
import tempfile
import argparse
import math

ti.init(arch = ti.vulkan)

WARP_SZ = 64
BLOCK_SZ = 128
arch = ti.vulkan

@ti.kernel
def blit_from_ndarray_to_ndarray(
        dst: ti.types.ndarray(ndim=1), src: ti.types.ndarray(ndim=1),
        offset: ti.i32, size: ti.i32):
    for i in range(size):
        dst[i + offset] = src[i]


@ti.kernel
def uniform_add(
        arr_in: ti.types.ndarray(ndim=1), in_beg: ti.i32, in_end: ti.i32):
    ti.loop_config(block_dim=BLOCK_SZ)
    for i in range(in_beg + BLOCK_SZ, in_end):
        block_id = int((i - in_beg) // BLOCK_SZ)
        arr_in[i] += arr_in[in_end + block_id - 1]


if arch == ti.cuda:
    inclusive_add = ti.warp_shfl_up_i32
elif arch == ti.vulkan or arch == ti.metal:
    inclusive_add = subgroup.inclusive_add


@ti.kernel
def scan_add_inclusive(arr_in: ti.types.ndarray(ndim=1), in_beg: ti.i32,
                       in_end: ti.i32, single_block: ti.i32):

    ti.loop_config(block_dim=BLOCK_SZ)
    for i in range(in_beg, in_end):
        val = arr_in[i]

        thread_id = i % BLOCK_SZ
        block_id = int((i - in_beg) // BLOCK_SZ)
        lane_id = thread_id % WARP_SZ
        warp_id = thread_id // WARP_SZ

        pad_shared = block.SharedArray((65, ), ti.i32)

        val = inclusive_add(val)
        block.sync()

        # Put warp scan results to smem
        # TODO replace smem with real smem when available
        if thread_id % WARP_SZ == WARP_SZ - 1:
            pad_shared[warp_id] = val
        block.sync()

        # Inter-warp scan, use the first thread in the first warp
        if warp_id == 0 and lane_id == 0:
            for k in range(1, BLOCK_SZ / WARP_SZ):
                pad_shared[k] += pad_shared[k - 1]
        block.sync()

        # Update data with warp sums
        warp_sum = 0
        if warp_id > 0:
            warp_sum = pad_shared[warp_id - 1]
        val += warp_sum
        arr_in[i] = val

        # Update partial sums except the final block
        if not single_block and (thread_id == BLOCK_SZ - 1):
            arr_in[in_end + block_id] = val


if __name__ == "__main__":
    # Serialize!
    #save_dir = args.dir
    arch = ti.vulkan
    save_dir = "./prefix_sum_module"
    mod = ti.aot.Module(ti.vulkan)

    arr_a = ti.ndarray(ti.i32, 100)
    arr_b = ti.ndarray(ti.i32, 100)

    mod.add_kernel(
        blit_from_ndarray_to_ndarray,
        template_args={
            "dst": arr_a,
            "src": arr_b
        },
    )

    mod.add_kernel(
        uniform_add,
        template_args={
            "arr_in": arr_a,
        },
    )

    mod.add_kernel(
        scan_add_inclusive,
        template_args={
            "arr_in": arr_a,
        },
    )

    mod.save(save_dir)
    mod.archive(save_dir + "/kernels.tcm")
