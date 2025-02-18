/*! \file */
/* ************************************************************************
 * Copyright (C) 2020-2023 Advanced Micro Devices, Inc. All rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * ************************************************************************ */

#include "rocsparse_enum.hpp"
#include "testing.hpp"

template <typename T>
void testing_gebsr2gebsr_bad_arg(const Arguments& arg)
{
    static const size_t safe_size = 100;

    // Create rocsparse handle
    rocsparse_local_handle local_handle;

    // Create descriptors
    rocsparse_local_mat_descr local_descr_A;
    rocsparse_local_mat_descr local_descr_C;

    rocsparse_handle          handle          = local_handle;
    rocsparse_direction       dir             = rocsparse_direction_row;
    rocsparse_int             mb              = safe_size;
    rocsparse_int             nb              = safe_size;
    rocsparse_int             nnzb            = safe_size;
    const rocsparse_mat_descr descr_A         = local_descr_A;
    const T*                  bsr_val_A       = (const T*)0x4;
    const rocsparse_int*      bsr_row_ptr_A   = (const rocsparse_int*)0x4;
    const rocsparse_int*      bsr_col_ind_A   = (const rocsparse_int*)0x4;
    rocsparse_int             row_block_dim_A = safe_size;
    rocsparse_int             col_block_dim_A = safe_size;
    const rocsparse_mat_descr descr_C         = local_descr_C;
    T*                        bsr_val_C       = (T*)0x4;
    rocsparse_int*            bsr_row_ptr_C   = (rocsparse_int*)0x4;
    rocsparse_int*            bsr_col_ind_C   = (rocsparse_int*)0x4;
    rocsparse_int             row_block_dim_C = safe_size;
    rocsparse_int             col_block_dim_C = safe_size;
    rocsparse_int*            nnzb_C          = (rocsparse_int*)0x4;
    ;
    size_t* buffer_size = (size_t*)0x4;
    void*   temp_buffer = (void*)0x4;

#define PARAMS_BUFFER_SIZE                                                                        \
    handle, dir, mb, nb, nnzb, descr_A, bsr_val_A, bsr_row_ptr_A, bsr_col_ind_A, row_block_dim_A, \
        col_block_dim_A, row_block_dim_C, col_block_dim_C, buffer_size
#define PARAMS_NNZ                                                                         \
    handle, dir, mb, nb, nnzb, descr_A, bsr_row_ptr_A, bsr_col_ind_A, row_block_dim_A,     \
        col_block_dim_A, descr_C, bsr_row_ptr_C, row_block_dim_C, col_block_dim_C, nnzb_C, \
        temp_buffer
#define PARAMS                                                                                    \
    handle, dir, mb, nb, nnzb, descr_A, bsr_val_A, bsr_row_ptr_A, bsr_col_ind_A, row_block_dim_A, \
        col_block_dim_A, descr_C, bsr_val_C, bsr_row_ptr_C, bsr_col_ind_C, row_block_dim_C,       \
        col_block_dim_C, temp_buffer

    auto_testing_bad_arg(rocsparse_gebsr2gebsr_buffer_size<T>, PARAMS_BUFFER_SIZE);
    auto_testing_bad_arg(rocsparse_gebsr2gebsr_nnz, PARAMS_NNZ);
    auto_testing_bad_arg(rocsparse_gebsr2gebsr<T>, PARAMS);

    CHECK_ROCSPARSE_ERROR(rocsparse_set_mat_storage_mode(descr_A, rocsparse_storage_mode_unsorted));
    CHECK_ROCSPARSE_ERROR(rocsparse_set_mat_storage_mode(descr_C, rocsparse_storage_mode_unsorted));
    EXPECT_ROCSPARSE_STATUS(rocsparse_gebsr2gebsr_buffer_size<T>(PARAMS_BUFFER_SIZE),
                            rocsparse_status_not_implemented);
    EXPECT_ROCSPARSE_STATUS(rocsparse_gebsr2gebsr_nnz(PARAMS_NNZ),
                            rocsparse_status_not_implemented);
    EXPECT_ROCSPARSE_STATUS(rocsparse_gebsr2gebsr<T>(PARAMS), rocsparse_status_not_implemented);

    CHECK_ROCSPARSE_ERROR(rocsparse_set_mat_storage_mode(descr_A, rocsparse_storage_mode_sorted));
    CHECK_ROCSPARSE_ERROR(rocsparse_set_mat_storage_mode(descr_C, rocsparse_storage_mode_sorted));

    // Check row_block_dim_A == 0
    row_block_dim_A = 0;
    col_block_dim_A = safe_size;
    EXPECT_ROCSPARSE_STATUS(rocsparse_gebsr2gebsr_buffer_size<T>(PARAMS_BUFFER_SIZE),
                            rocsparse_status_invalid_size);
    EXPECT_ROCSPARSE_STATUS(rocsparse_gebsr2gebsr_nnz(PARAMS_NNZ), rocsparse_status_invalid_size);
    EXPECT_ROCSPARSE_STATUS(rocsparse_gebsr2gebsr<T>(PARAMS), rocsparse_status_invalid_size);

    // Check col_block_dim_A == 0
    row_block_dim_A = safe_size;
    col_block_dim_A = 0;
    EXPECT_ROCSPARSE_STATUS(rocsparse_gebsr2gebsr_buffer_size<T>(PARAMS_BUFFER_SIZE),
                            rocsparse_status_invalid_size);
    EXPECT_ROCSPARSE_STATUS(rocsparse_gebsr2gebsr_nnz(PARAMS_NNZ), rocsparse_status_invalid_size);
    EXPECT_ROCSPARSE_STATUS(rocsparse_gebsr2gebsr<T>(PARAMS), rocsparse_status_invalid_size);

    // Check row_block_dim_C == 0
    row_block_dim_C = 0;
    col_block_dim_C = safe_size;
    EXPECT_ROCSPARSE_STATUS(rocsparse_gebsr2gebsr_buffer_size<T>(PARAMS_BUFFER_SIZE),
                            rocsparse_status_invalid_size);
    EXPECT_ROCSPARSE_STATUS(rocsparse_gebsr2gebsr_nnz(PARAMS_NNZ), rocsparse_status_invalid_size);
    EXPECT_ROCSPARSE_STATUS(rocsparse_gebsr2gebsr<T>(PARAMS), rocsparse_status_invalid_size);

    // Check col_block_dim_C == 0
    row_block_dim_C = safe_size;
    col_block_dim_C = 0;
    EXPECT_ROCSPARSE_STATUS(rocsparse_gebsr2gebsr_buffer_size<T>(PARAMS_BUFFER_SIZE),
                            rocsparse_status_invalid_size);
    EXPECT_ROCSPARSE_STATUS(rocsparse_gebsr2gebsr_nnz(PARAMS_NNZ), rocsparse_status_invalid_size);
    EXPECT_ROCSPARSE_STATUS(rocsparse_gebsr2gebsr<T>(PARAMS), rocsparse_status_invalid_size);
#undef PARAMS
#undef PARAMS_NNZ
#undef PARAMS_BUFFER_SIZE
}

template <typename T>
void testing_gebsr2gebsr(const Arguments& arg)
{
    rocsparse_matrix_factory<T> matrix_factory(arg);
    rocsparse_int               M               = arg.M;
    rocsparse_int               N               = arg.N;
    rocsparse_index_base        base_A          = arg.baseA;
    rocsparse_index_base        base_C          = arg.baseB;
    rocsparse_direction         direction       = arg.direction;
    rocsparse_int               row_block_dim_A = arg.row_block_dimA;
    rocsparse_int               col_block_dim_A = arg.col_block_dimA;
    rocsparse_int               row_block_dim_C = arg.row_block_dimB;
    rocsparse_int               col_block_dim_C = arg.col_block_dimB;

    // Create rocsparse handle
    rocsparse_local_handle handle;

    rocsparse_local_mat_descr descr_A;
    rocsparse_local_mat_descr descr_C;

    CHECK_ROCSPARSE_ERROR(rocsparse_set_mat_index_base(descr_A, base_A));
    CHECK_ROCSPARSE_ERROR(rocsparse_set_mat_index_base(descr_C, base_C));

    // Declare and initialize matrices.
    host_gebsr_matrix<T> hA;
    matrix_factory.init_gebsr(hA);

    device_gebsr_matrix<T> dA(hA);

    M = dA.mb * dA.row_block_dim;
    N = dA.nb * dA.col_block_dim;

    rocsparse_int Mb_A = dA.mb;
    rocsparse_int Nb_A = dA.nb;
    rocsparse_int Mb_C = (M + row_block_dim_C - 1) / row_block_dim_C;
    rocsparse_int Nb_C = (N + col_block_dim_C - 1) / col_block_dim_C;

    device_gebsr_matrix<T> dC(direction, Mb_C, Nb_C, 0, row_block_dim_C, col_block_dim_C, base_C);

    size_t buffer_size = 0;
    CHECK_ROCSPARSE_ERROR(rocsparse_gebsr2gebsr_buffer_size<T>(handle,
                                                               direction,
                                                               dA.mb,
                                                               dA.nb,
                                                               dA.nnzb,
                                                               descr_A,
                                                               dA.val,
                                                               dA.ptr,
                                                               dA.ind,
                                                               row_block_dim_A,
                                                               col_block_dim_A,
                                                               row_block_dim_C,
                                                               col_block_dim_C,
                                                               &buffer_size));

    void* dbuffer;
    CHECK_HIP_ERROR(rocsparse_hipMalloc(&dbuffer, buffer_size));

    CHECK_ROCSPARSE_ERROR(rocsparse_set_pointer_mode(handle, rocsparse_pointer_mode_host));

    host_scalar<rocsparse_int> hnnzb_C;
    CHECK_ROCSPARSE_ERROR(rocsparse_gebsr2gebsr_nnz(handle,
                                                    direction,
                                                    dA.mb,
                                                    dA.nb,
                                                    dA.nnzb,
                                                    descr_A,
                                                    dA.ptr,
                                                    dA.ind,
                                                    row_block_dim_A,
                                                    col_block_dim_A,
                                                    descr_C,
                                                    dC.ptr,
                                                    row_block_dim_C,
                                                    col_block_dim_C,
                                                    hnnzb_C,
                                                    dbuffer));

    CHECK_ROCSPARSE_ERROR(rocsparse_set_pointer_mode(handle, rocsparse_pointer_mode_device));

    device_scalar<rocsparse_int> dnnzb_C;
    CHECK_ROCSPARSE_ERROR(rocsparse_gebsr2gebsr_nnz(handle,
                                                    direction,
                                                    dA.mb,
                                                    dA.nb,
                                                    dA.nnzb,
                                                    descr_A,
                                                    dA.ptr,
                                                    dA.ind,
                                                    row_block_dim_A,
                                                    col_block_dim_A,
                                                    descr_C,
                                                    dC.ptr,
                                                    row_block_dim_C,
                                                    col_block_dim_C,
                                                    dnnzb_C,
                                                    dbuffer));

    dC.define(direction, Mb_C, Nb_C, *hnnzb_C, row_block_dim_C, col_block_dim_C, base_C);

    if(arg.unit_check)
    {
        dnnzb_C.unit_check(hnnzb_C);

        // Finish conversion
        CHECK_ROCSPARSE_ERROR(rocsparse_set_pointer_mode(handle, rocsparse_pointer_mode_host));
        CHECK_ROCSPARSE_ERROR(rocsparse_gebsr2gebsr<T>(handle,
                                                       direction,
                                                       dA.mb,
                                                       dA.nb,
                                                       dA.nnzb,
                                                       descr_A,
                                                       dA.val,
                                                       dA.ptr,
                                                       dA.ind,
                                                       row_block_dim_A,
                                                       col_block_dim_A,
                                                       descr_C,
                                                       dC.val,
                                                       dC.ptr,
                                                       dC.ind,
                                                       row_block_dim_C,
                                                       col_block_dim_C,
                                                       dbuffer));

        // Host solution
        host_gebsr_matrix<T> hC_gold(
            direction, Mb_C, Nb_C, *hnnzb_C, row_block_dim_C, col_block_dim_C, base_C);
        host_gebsr_to_gebsr(direction,
                            hA.mb,
                            hA.nb,
                            hA.nnzb,
                            hA.val,
                            hA.ptr,
                            hA.ind,
                            row_block_dim_A,
                            col_block_dim_A,
                            base_A,
                            hC_gold.val,
                            hC_gold.ptr,
                            hC_gold.ind,
                            row_block_dim_C,
                            col_block_dim_C,
                            base_C);

        hC_gold.near_check(dC);
    }

    if(arg.timing)
    {
        int number_cold_calls = 2;
        int number_hot_calls  = arg.iters;

        // Warm up
        for(int iter = 0; iter < number_cold_calls; ++iter)
        {
            CHECK_ROCSPARSE_ERROR(rocsparse_gebsr2gebsr<T>(handle,
                                                           direction,
                                                           dA.mb,
                                                           dA.nb,
                                                           dA.nnzb,
                                                           descr_A,
                                                           dA.val,
                                                           dA.ptr,
                                                           dA.ind,
                                                           row_block_dim_A,
                                                           col_block_dim_A,
                                                           descr_C,
                                                           dC.val,
                                                           dC.ptr,
                                                           dC.ind,
                                                           row_block_dim_C,
                                                           col_block_dim_C,
                                                           dbuffer));
        }

        double gpu_time_used = get_time_us();

        // Performance run
        for(int iter = 0; iter < number_hot_calls; ++iter)
        {
            CHECK_ROCSPARSE_ERROR(rocsparse_gebsr2gebsr<T>(handle,
                                                           direction,
                                                           dA.mb,
                                                           dA.nb,
                                                           dA.nnzb,
                                                           descr_A,
                                                           dA.val,
                                                           dA.ptr,
                                                           dA.ind,
                                                           row_block_dim_A,
                                                           col_block_dim_A,
                                                           descr_C,
                                                           dC.val,
                                                           dC.ptr,
                                                           dC.ind,
                                                           row_block_dim_C,
                                                           col_block_dim_C,
                                                           dbuffer));
        }

        gpu_time_used = (get_time_us() - gpu_time_used) / number_hot_calls;

        double gbyte_count = gebsr2gebsr_gbyte_count<T>(dA.mb,
                                                        dC.mb,
                                                        row_block_dim_A,
                                                        col_block_dim_A,
                                                        row_block_dim_C,
                                                        col_block_dim_C,
                                                        dA.nnzb,
                                                        dC.nnzb);

        double gpu_gbyte = get_gpu_gbyte(gpu_time_used, gbyte_count);

        display_timing_info("M",
                            M,
                            "N",
                            N,
                            "Mb_A",
                            Mb_A,
                            "Nb_A",
                            Nb_A,
                            "Mb_C",
                            Mb_C,
                            "Nb_C",
                            Nb_C,
                            "rblockdimA",
                            row_block_dim_A,
                            "cblockdimA",
                            col_block_dim_A,
                            "rblockdimC",
                            row_block_dim_C,
                            "cblockdimC",
                            col_block_dim_C,
                            "nnzbA",
                            dA.nnzb,
                            "nnzbC",
                            dC.nnzb,
                            s_timing_info_bandwidth,
                            gpu_gbyte,
                            s_timing_info_time,
                            get_gpu_time_msec(gpu_time_used));
    }

    // Free buffer
    CHECK_HIP_ERROR(rocsparse_hipFree(dbuffer));
}

#define INSTANTIATE(TYPE)                                                  \
    template void testing_gebsr2gebsr_bad_arg<TYPE>(const Arguments& arg); \
    template void testing_gebsr2gebsr<TYPE>(const Arguments& arg)
INSTANTIATE(float);
INSTANTIATE(double);
INSTANTIATE(rocsparse_float_complex);
INSTANTIATE(rocsparse_double_complex);
void testing_gebsr2gebsr_extra(const Arguments& arg) {}
