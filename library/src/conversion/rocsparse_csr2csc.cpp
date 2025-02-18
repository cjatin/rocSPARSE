/*! \file */
/* ************************************************************************
 * Copyright (C) 2018-2023 Advanced Micro Devices, Inc. All rights Reserved.
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

#include "rocsparse_csr2csc.hpp"
#include "common.h"
#include "definitions.h"
#include "utility.h"

#include "csr2csc_device.h"
#include "rocsparse_coo2csr.hpp"
#include "rocsparse_csr2coo.hpp"
#include "rocsparse_identity.hpp"
#include <rocprim/rocprim.hpp>

template <typename I, typename J, typename T>
rocsparse_status rocsparse_csr2csc_core(rocsparse_handle     handle,
                                        J                    m,
                                        J                    n,
                                        I                    nnz,
                                        const T*             csr_val,
                                        const I*             csr_row_ptr_begin,
                                        const I*             csr_row_ptr_end,
                                        const J*             csr_col_ind,
                                        T*                   csc_val,
                                        J*                   csc_row_ind,
                                        I*                   csc_col_ptr,
                                        rocsparse_action     copy_values,
                                        rocsparse_index_base idx_base,
                                        void*                temp_buffer)
{
    // Stream
    hipStream_t stream = handle->stream;

    unsigned int startbit = 0;
    unsigned int endbit   = rocsparse_clz(n);

    // Temporary buffer entry points
    char* ptr = reinterpret_cast<char*>(temp_buffer);

    // work1 buffer
    J* tmp_work1 = reinterpret_cast<J*>(ptr);
    ptr += ((sizeof(J) * nnz - 1) / 256 + 1) * 256;

    // Load CSR column indices into work1 buffer
    RETURN_IF_HIP_ERROR(
        hipMemcpyAsync(tmp_work1, csr_col_ind, sizeof(J) * nnz, hipMemcpyDeviceToDevice, stream));

    if(copy_values == rocsparse_action_symbolic)
    {
        // action symbolic

        // work2 buffer
        J* tmp_work2 = reinterpret_cast<J*>(ptr);
        ptr += ((sizeof(J) * nnz - 1) / 256 + 1) * 256;

        // perm buffer
        J* tmp_perm = reinterpret_cast<J*>(ptr);
        ptr += ((sizeof(J) * nnz - 1) / 256 + 1) * 256;

        // rocprim buffer
        void* tmp_rocprim = reinterpret_cast<void*>(ptr);

        // Create row indices
        RETURN_IF_ROCSPARSE_ERROR(rocsparse_csr2coo_core(
            handle, csr_row_ptr_begin, csr_row_ptr_end, nnz, m, csc_row_ind, idx_base));
        // Stable sort COO by columns
        rocprim::double_buffer<J> keys(tmp_work1, tmp_perm);
        rocprim::double_buffer<J> vals(csc_row_ind, tmp_work2);

        size_t size = 0;

        RETURN_IF_HIP_ERROR(
            rocprim::radix_sort_pairs(nullptr, size, keys, vals, nnz, startbit, endbit, stream));
        RETURN_IF_HIP_ERROR(rocprim::radix_sort_pairs(
            tmp_rocprim, size, keys, vals, nnz, startbit, endbit, stream));

        // Create column pointers
        RETURN_IF_ROCSPARSE_ERROR(
            rocsparse_coo2csr_core(handle, keys.current(), nnz, n, csc_col_ptr, idx_base));

        // Copy csc_row_ind if not current
        if(vals.current() != csc_row_ind)
        {
            RETURN_IF_HIP_ERROR(hipMemcpyAsync(
                csc_row_ind, vals.current(), sizeof(J) * nnz, hipMemcpyDeviceToDevice, stream));
        }
    }
    else
    {
        // action numeric

        // work2 buffer
        I* tmp_work2 = reinterpret_cast<I*>(ptr);
        ptr += ((sizeof(I) * nnz - 1) / 256 + 1) * 256;

        // perm buffer
        I* tmp_perm = reinterpret_cast<I*>(ptr);
        ptr += ((sizeof(I) * nnz - 1) / 256 + 1) * 256;

        // rocprim buffer
        void* tmp_rocprim = reinterpret_cast<void*>(ptr);

        // Create identitiy permutation
        RETURN_IF_ROCSPARSE_ERROR(
            rocsparse_create_identity_permutation_core(handle, nnz, tmp_perm));

        // Stable sort COO by columns
        rocprim::double_buffer<J> keys(tmp_work1, csc_row_ind);
        rocprim::double_buffer<I> vals(tmp_perm, tmp_work2);

        size_t size = 0;

        RETURN_IF_HIP_ERROR(
            rocprim::radix_sort_pairs(nullptr, size, keys, vals, nnz, startbit, endbit, stream));
        RETURN_IF_HIP_ERROR(rocprim::radix_sort_pairs(
            tmp_rocprim, size, keys, vals, nnz, startbit, endbit, stream));

        // Create column pointers
        RETURN_IF_ROCSPARSE_ERROR(
            rocsparse_coo2csr_core(handle, keys.current(), nnz, n, csc_col_ptr, idx_base));

        // Create row indices
        RETURN_IF_ROCSPARSE_ERROR(rocsparse_csr2coo_core(
            handle, csr_row_ptr_begin, csr_row_ptr_end, nnz, m, tmp_work1, idx_base));

// Permute row indices and values
#define CSR2CSC_DIM 512
        dim3 csr2csc_blocks((nnz - 1) / CSR2CSC_DIM + 1);
        dim3 csr2csc_threads(CSR2CSC_DIM);
        hipLaunchKernelGGL((csr2csc_permute_kernel<CSR2CSC_DIM>),
                           csr2csc_blocks,
                           csr2csc_threads,
                           0,
                           stream,
                           nnz,
                           tmp_work1,
                           csr_val,
                           vals.current(),
                           csc_row_ind,
                           csc_val);
#undef CSR2CSC_DIM
    }

    return rocsparse_status_success;
}

template <typename I, typename J, typename T>
rocsparse_status rocsparse_csr2csc_template(rocsparse_handle     handle,
                                            J                    m,
                                            J                    n,
                                            I                    nnz,
                                            const T*             csr_val,
                                            const I*             csr_row_ptr,
                                            const J*             csr_col_ind,
                                            T*                   csc_val,
                                            J*                   csc_row_ind,
                                            I*                   csc_col_ptr,
                                            rocsparse_action     copy_values,
                                            rocsparse_index_base idx_base,
                                            void*                temp_buffer)
{

    // Quick return if possible
    if(m == 0 || n == 0)
    {
        return rocsparse_status_success;
    }

    if(nnz == 0)
    {
        hipLaunchKernelGGL((set_array_to_value<256>),
                           dim3(n / 256 + 1),
                           dim3(256),
                           0,
                           handle->stream,
                           (n + 1),
                           csc_col_ptr,
                           static_cast<I>(idx_base));

        return rocsparse_status_success;
    }

    if(temp_buffer == nullptr)
    {
        return rocsparse_status_invalid_pointer;
    }

    return rocsparse_csr2csc_core(handle,
                                  m,
                                  n,
                                  nnz,
                                  csr_val,
                                  csr_row_ptr,
                                  csr_row_ptr + 1,
                                  csr_col_ind,
                                  csc_val,
                                  csc_row_ind,
                                  csc_col_ptr,
                                  copy_values,
                                  idx_base,
                                  temp_buffer);
}

template rocsparse_status rocsparse_csr2csc_template<rocsparse_int, rocsparse_int, rocsparse_int>(
    rocsparse_handle     handle,
    rocsparse_int        m,
    rocsparse_int        n,
    rocsparse_int        nnz,
    const rocsparse_int* csr_val,
    const rocsparse_int* csr_row_ptr,
    const rocsparse_int* csr_col_ind,
    rocsparse_int*       csc_val,
    rocsparse_int*       csc_row_ind,
    rocsparse_int*       csc_col_ptr,
    rocsparse_action     copy_values,
    rocsparse_index_base idx_base,
    void*                temp_buffer);

template <typename I, typename J, typename T>
rocsparse_status rocsparse_csr2csc_impl(rocsparse_handle     handle,
                                        J                    m,
                                        J                    n,
                                        I                    nnz,
                                        const T*             csr_val,
                                        const I*             csr_row_ptr,
                                        const J*             csr_col_ind,
                                        T*                   csc_val,
                                        J*                   csc_row_ind,
                                        I*                   csc_col_ptr,
                                        rocsparse_action     copy_values,
                                        rocsparse_index_base idx_base,
                                        void*                temp_buffer)
{
    // Check for valid handle and matrix descriptor
    if(handle == nullptr)
    {
        return rocsparse_status_invalid_handle;
    }

    // Logging
    log_trace(handle,
              replaceX<T>("rocsparse_Xcsr2csc"),
              m,
              n,
              nnz,
              (const void*&)csr_val,
              (const void*&)csr_row_ptr,
              (const void*&)csr_col_ind,
              (const void*&)csc_val,
              (const void*&)csc_row_ind,
              (const void*&)csc_col_ptr,
              copy_values,
              idx_base,
              (const void*&)temp_buffer);

    log_bench(handle, "./rocsparse-bench -f csr2csc -r", replaceX<T>("X"), "--mtx <matrix.mtx>");

    // Check action
    if(rocsparse_enum_utils::is_invalid(copy_values))
    {
        return rocsparse_status_invalid_value;
    }

    // Check index base
    if(rocsparse_enum_utils::is_invalid(idx_base))
    {
        return rocsparse_status_invalid_value;
    }

    // Check sizes
    if(m < 0 || n < 0 || nnz < 0)
    {
        return rocsparse_status_invalid_size;
    }

    // Check pointer arguments
    if((m > 0 && csr_row_ptr == nullptr) || (n > 0 && csc_col_ptr == nullptr))
    {
        return rocsparse_status_invalid_pointer;
    }

    if(copy_values == rocsparse_action_numeric)
    {
        // value arrays and column indices arrays must both be null (zero matrix) or both not null
        if((csr_val == nullptr && csr_col_ind != nullptr)
           || (csr_val != nullptr && csr_col_ind == nullptr))
        {
            return rocsparse_status_invalid_pointer;
        }

        if((csc_val == nullptr && csc_row_ind != nullptr)
           || (csc_val != nullptr && csc_row_ind == nullptr))
        {
            return rocsparse_status_invalid_pointer;
        }

        if(nnz != 0 && (csr_val == nullptr && csr_col_ind == nullptr))
        {
            return rocsparse_status_invalid_pointer;
        }

        if(nnz != 0 && (csc_val == nullptr && csc_row_ind == nullptr))
        {
            return rocsparse_status_invalid_pointer;
        }
    }
    else
    {
        // if copying symbolically, then column/row indices arrays can only be null if the zero matrix
        if(nnz != 0 && (csr_col_ind == nullptr || csc_row_ind == nullptr))
        {
            return rocsparse_status_invalid_pointer;
        }
    }

    return rocsparse_csr2csc_template(handle,
                                      m,
                                      n,
                                      nnz,
                                      csr_val,
                                      csr_row_ptr,
                                      csr_col_ind,
                                      csc_val,
                                      csc_row_ind,
                                      csc_col_ptr,
                                      copy_values,
                                      idx_base,
                                      temp_buffer);
}

#define INSTANTIATE(ITYPE, JTYPE, TTYPE)                                       \
    template rocsparse_status rocsparse_csr2csc_core<ITYPE, JTYPE, TTYPE>(     \
        rocsparse_handle     handle,                                           \
        JTYPE                m,                                                \
        JTYPE                n,                                                \
        ITYPE                nnz,                                              \
        const TTYPE*         csr_val,                                          \
        const ITYPE*         csr_row_ptr_begin,                                \
        const ITYPE*         csr_row_ptr_end,                                  \
        const JTYPE*         csr_col_ind,                                      \
        TTYPE*               csc_val,                                          \
        JTYPE*               csc_row_ind,                                      \
        ITYPE*               csc_col_ptr,                                      \
        rocsparse_action     copy_values,                                      \
        rocsparse_index_base idx_base,                                         \
        void*                temp_buffer);                                                    \
    template rocsparse_status rocsparse_csr2csc_impl<ITYPE, JTYPE, TTYPE>(     \
        rocsparse_handle     handle,                                           \
        JTYPE                m,                                                \
        JTYPE                n,                                                \
        ITYPE                nnz,                                              \
        const TTYPE*         csr_val,                                          \
        const ITYPE*         csr_row_ptr,                                      \
        const JTYPE*         csr_col_ind,                                      \
        TTYPE*               csc_val,                                          \
        JTYPE*               csc_row_ind,                                      \
        ITYPE*               csc_col_ptr,                                      \
        rocsparse_action     copy_values,                                      \
        rocsparse_index_base idx_base,                                         \
        void*                temp_buffer);                                                    \
    template rocsparse_status rocsparse_csr2csc_template<ITYPE, JTYPE, TTYPE>( \
        rocsparse_handle     handle,                                           \
        JTYPE                m,                                                \
        JTYPE                n,                                                \
        ITYPE                nnz,                                              \
        const TTYPE*         csr_val,                                          \
        const ITYPE*         csr_row_ptr,                                      \
        const JTYPE*         csr_col_ind,                                      \
        TTYPE*               csc_val,                                          \
        JTYPE*               csc_row_ind,                                      \
        ITYPE*               csc_col_ptr,                                      \
        rocsparse_action     copy_values,                                      \
        rocsparse_index_base idx_base,                                         \
        void*                temp_buffer)
INSTANTIATE(int32_t, int32_t, int8_t);
INSTANTIATE(int64_t, int32_t, int8_t);
INSTANTIATE(int64_t, int64_t, int8_t);

INSTANTIATE(int32_t, int32_t, float);
INSTANTIATE(int64_t, int32_t, float);
INSTANTIATE(int64_t, int64_t, float);

INSTANTIATE(int32_t, int32_t, double);
INSTANTIATE(int64_t, int32_t, double);
INSTANTIATE(int64_t, int64_t, double);

INSTANTIATE(int32_t, int32_t, rocsparse_float_complex);
INSTANTIATE(int64_t, int32_t, rocsparse_float_complex);
INSTANTIATE(int64_t, int64_t, rocsparse_float_complex);

INSTANTIATE(int32_t, int32_t, rocsparse_double_complex);
INSTANTIATE(int64_t, int32_t, rocsparse_double_complex);
INSTANTIATE(int64_t, int64_t, rocsparse_double_complex);
#undef INSTANTIATE

template <typename I, typename J>
rocsparse_status rocsparse_csr2csc_buffer_size_core(rocsparse_handle handle,
                                                    J                m,
                                                    J                n,
                                                    I                nnz,
                                                    const I*         csr_row_ptr_begin,
                                                    const I*         csr_row_ptr_end,
                                                    const J*         csr_col_ind,
                                                    rocsparse_action copy_values,
                                                    size_t*          buffer_size)
{

    hipStream_t stream = handle->stream;

    // Determine rocprim buffer size
    J* ptr = reinterpret_cast<J*>(buffer_size);

    rocprim::double_buffer<J> dummy(ptr, ptr);

    RETURN_IF_HIP_ERROR(
        rocprim::radix_sort_pairs(nullptr, *buffer_size, dummy, dummy, nnz, 0, 32, stream));

    *buffer_size = ((*buffer_size - 1) / 256 + 1) * 256;

    // rocPRIM does not support in-place sorting, so we need additional buffer
    // for all temporary arrays
    *buffer_size += ((sizeof(J) * nnz - 1) / 256 + 1) * 256;
    *buffer_size += ((std::max(sizeof(I), sizeof(J)) * nnz - 1) / 256 + 1) * 256;
    *buffer_size += ((std::max(sizeof(I), sizeof(J)) * nnz - 1) / 256 + 1) * 256;

    return rocsparse_status_success;
}

template <typename I, typename J>
rocsparse_status rocsparse_csr2csc_buffer_size_template(rocsparse_handle handle,
                                                        J                m,
                                                        J                n,
                                                        I                nnz,
                                                        const I*         csr_row_ptr,
                                                        const J*         csr_col_ind,
                                                        rocsparse_action copy_values,
                                                        size_t*          buffer_size)
{
    // Quick return if possible
    if(m == 0 || n == 0 || nnz == 0)
    {
        *buffer_size = 0;
        return rocsparse_status_success;
    }

    return rocsparse_csr2csc_buffer_size_core(
        handle, m, n, nnz, csr_row_ptr, csr_row_ptr + 1, csr_col_ind, copy_values, buffer_size);
}

template <typename I, typename J>
rocsparse_status rocsparse_csr2csc_buffer_size_impl(rocsparse_handle handle,
                                                    J                m,
                                                    J                n,
                                                    I                nnz,
                                                    const I*         csr_row_ptr,
                                                    const J*         csr_col_ind,
                                                    rocsparse_action copy_values,
                                                    size_t*          buffer_size)
{
    // Check for valid handle
    if(handle == nullptr)
    {
        return rocsparse_status_invalid_handle;
    }

    // Logging
    log_trace(handle,
              "rocsparse_csr2csc_buffer_size",
              m,
              n,
              nnz,
              (const void*&)csr_row_ptr,
              (const void*&)csr_col_ind,
              copy_values,
              (const void*&)buffer_size);

    // Check action
    if(rocsparse_enum_utils::is_invalid(copy_values))
    {
        return rocsparse_status_invalid_value;
    }

    // Check sizes
    if(m < 0 || n < 0 || nnz < 0)
    {
        return rocsparse_status_invalid_size;
    }

    // Check buffer size argument
    if(buffer_size == nullptr)
    {
        return rocsparse_status_invalid_pointer;
    }

    // Check pointer arguments
    if(m > 0 && csr_row_ptr == nullptr)
    {
        return rocsparse_status_invalid_pointer;
    }
    if(nnz > 0 && csr_col_ind == nullptr)
    {
        return rocsparse_status_invalid_pointer;
    }

    return rocsparse_csr2csc_buffer_size_template(
        handle, m, n, nnz, csr_row_ptr, csr_col_ind, copy_values, buffer_size);
}

#define INSTANTIATE(ITYPE, JTYPE)                                                   \
    template rocsparse_status rocsparse_csr2csc_buffer_size_core<ITYPE, JTYPE>(     \
        rocsparse_handle handle,                                                    \
        JTYPE            m,                                                         \
        JTYPE            n,                                                         \
        ITYPE            nnz,                                                       \
        const ITYPE*     csr_row_ptr_begin,                                         \
        const ITYPE*     csr_row_ptr_end,                                           \
        const JTYPE*     csr_col_ind,                                               \
        rocsparse_action copy_values,                                               \
        size_t*          buffer_size);                                                       \
                                                                                    \
    template rocsparse_status rocsparse_csr2csc_buffer_size_template<ITYPE, JTYPE>( \
        rocsparse_handle handle,                                                    \
        JTYPE            m,                                                         \
        JTYPE            n,                                                         \
        ITYPE            nnz,                                                       \
        const ITYPE*     csr_row_ptr,                                               \
        const JTYPE*     csr_col_ind,                                               \
        rocsparse_action copy_values,                                               \
        size_t*          buffer_size);                                                       \
                                                                                    \
    template rocsparse_status rocsparse_csr2csc_buffer_size_impl<ITYPE, JTYPE>(     \
        rocsparse_handle handle,                                                    \
        JTYPE            m,                                                         \
        JTYPE            n,                                                         \
        ITYPE            nnz,                                                       \
        const ITYPE*     csr_row_ptr,                                               \
        const JTYPE*     csr_col_ind,                                               \
        rocsparse_action copy_values,                                               \
        size_t*          buffer_size)

INSTANTIATE(int32_t, int32_t);
INSTANTIATE(int64_t, int32_t);
INSTANTIATE(int64_t, int64_t);
#undef INSTANTIATE

/*
 * ===========================================================================
 *    C wrapper
 * ===========================================================================
 */
extern "C" rocsparse_status rocsparse_csr2csc_buffer_size(rocsparse_handle     handle,
                                                          rocsparse_int        m,
                                                          rocsparse_int        n,
                                                          rocsparse_int        nnz,
                                                          const rocsparse_int* csr_row_ptr,
                                                          const rocsparse_int* csr_col_ind,
                                                          rocsparse_action     copy_values,
                                                          size_t*              buffer_size)
try
{
    return rocsparse_csr2csc_buffer_size_impl(
        handle, m, n, nnz, csr_row_ptr, csr_col_ind, copy_values, buffer_size);
}
catch(...)
{
    return exception_to_rocsparse_status();
}

extern "C" rocsparse_status rocsparse_scsr2csc(rocsparse_handle     handle,
                                               rocsparse_int        m,
                                               rocsparse_int        n,
                                               rocsparse_int        nnz,
                                               const float*         csr_val,
                                               const rocsparse_int* csr_row_ptr,
                                               const rocsparse_int* csr_col_ind,
                                               float*               csc_val,
                                               rocsparse_int*       csc_row_ind,
                                               rocsparse_int*       csc_col_ptr,
                                               rocsparse_action     copy_values,
                                               rocsparse_index_base idx_base,
                                               void*                temp_buffer)
try
{
    return rocsparse_csr2csc_impl(handle,
                                  m,
                                  n,
                                  nnz,
                                  csr_val,
                                  csr_row_ptr,
                                  csr_col_ind,
                                  csc_val,
                                  csc_row_ind,
                                  csc_col_ptr,
                                  copy_values,
                                  idx_base,
                                  temp_buffer);
}
catch(...)
{
    return exception_to_rocsparse_status();
}

extern "C" rocsparse_status rocsparse_dcsr2csc(rocsparse_handle     handle,
                                               rocsparse_int        m,
                                               rocsparse_int        n,
                                               rocsparse_int        nnz,
                                               const double*        csr_val,
                                               const rocsparse_int* csr_row_ptr,
                                               const rocsparse_int* csr_col_ind,
                                               double*              csc_val,
                                               rocsparse_int*       csc_row_ind,
                                               rocsparse_int*       csc_col_ptr,
                                               rocsparse_action     copy_values,
                                               rocsparse_index_base idx_base,
                                               void*                temp_buffer)
try
{
    return rocsparse_csr2csc_impl(handle,
                                  m,
                                  n,
                                  nnz,
                                  csr_val,
                                  csr_row_ptr,
                                  csr_col_ind,
                                  csc_val,
                                  csc_row_ind,
                                  csc_col_ptr,
                                  copy_values,
                                  idx_base,
                                  temp_buffer);
}
catch(...)
{
    return exception_to_rocsparse_status();
}

extern "C" rocsparse_status rocsparse_ccsr2csc(rocsparse_handle               handle,
                                               rocsparse_int                  m,
                                               rocsparse_int                  n,
                                               rocsparse_int                  nnz,
                                               const rocsparse_float_complex* csr_val,
                                               const rocsparse_int*           csr_row_ptr,
                                               const rocsparse_int*           csr_col_ind,
                                               rocsparse_float_complex*       csc_val,
                                               rocsparse_int*                 csc_row_ind,
                                               rocsparse_int*                 csc_col_ptr,
                                               rocsparse_action               copy_values,
                                               rocsparse_index_base           idx_base,
                                               void*                          temp_buffer)
try
{
    return rocsparse_csr2csc_impl(handle,
                                  m,
                                  n,
                                  nnz,
                                  csr_val,
                                  csr_row_ptr,
                                  csr_col_ind,
                                  csc_val,
                                  csc_row_ind,
                                  csc_col_ptr,
                                  copy_values,
                                  idx_base,
                                  temp_buffer);
}
catch(...)
{
    return exception_to_rocsparse_status();
}

extern "C" rocsparse_status rocsparse_zcsr2csc(rocsparse_handle                handle,
                                               rocsparse_int                   m,
                                               rocsparse_int                   n,
                                               rocsparse_int                   nnz,
                                               const rocsparse_double_complex* csr_val,
                                               const rocsparse_int*            csr_row_ptr,
                                               const rocsparse_int*            csr_col_ind,
                                               rocsparse_double_complex*       csc_val,
                                               rocsparse_int*                  csc_row_ind,
                                               rocsparse_int*                  csc_col_ptr,
                                               rocsparse_action                copy_values,
                                               rocsparse_index_base            idx_base,
                                               void*                           temp_buffer)
try
{
    return rocsparse_csr2csc_impl(handle,
                                  m,
                                  n,
                                  nnz,
                                  csr_val,
                                  csr_row_ptr,
                                  csr_col_ind,
                                  csc_val,
                                  csc_row_ind,
                                  csc_col_ptr,
                                  copy_values,
                                  idx_base,
                                  temp_buffer);
}
catch(...)
{
    return exception_to_rocsparse_status();
}
