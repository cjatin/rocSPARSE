/* ************************************************************************
 * Copyright 2018 Advanced Micro Devices, Inc.
 * ************************************************************************ */

#include "unit.hpp"

#include <rocsparse.h>
#include <hip/hip_runtime_api.h>
#include <algorithm>
#include <limits>

#ifdef GOOGLE_TEST
#include <gtest/gtest.h>
#else
#ifdef NDEBUG
#undef NDEBUG
#include <assert.h>
#define NDEBUG
#else
#include <assert.h>
#endif
#endif

/* ========================================Gtest Unit Check
 * ==================================================== */

/*! \brief Template: gtest unit compare two matrices float/double/complex */
// Do not put a wrapper over ASSERT_FLOAT_EQ, since assert exit the current function NOT the test
// case
// a wrapper will cause the loop keep going

template <>
void unit_check_general(
    rocsparse_int M, rocsparse_int N, rocsparse_int lda, float* hCPU, float* hGPU)
{
    for(rocsparse_int j = 0; j < N; j++)
    {
        for(rocsparse_int i = 0; i < M; i++)
        {
#ifdef GOOGLE_TEST
            ASSERT_FLOAT_EQ(hCPU[i + j * lda], hGPU[i + j * lda]);
#else
            assert(hCPU[i + j * lda] == hGPU[i + j * lda]);
#endif
        }
    }
}

template <>
void unit_check_general(
    rocsparse_int M, rocsparse_int N, rocsparse_int lda, double* hCPU, double* hGPU)
{
    for(rocsparse_int j = 0; j < N; j++)
    {
        for(rocsparse_int i = 0; i < M; i++)
        {
#ifdef GOOGLE_TEST
            ASSERT_DOUBLE_EQ(hCPU[i + j * lda], hGPU[i + j * lda]);
#else
            assert(hCPU[i + j * lda] == hGPU[i + j * lda]);
#endif
        }
    }
}

template <>
void unit_check_general(
    rocsparse_int M, rocsparse_int N, rocsparse_int lda, rocsparse_int* hCPU, rocsparse_int* hGPU)
{
    for(rocsparse_int j = 0; j < N; j++)
    {
        for(rocsparse_int i = 0; i < M; i++)
        {
#ifdef GOOGLE_TEST
            ASSERT_EQ(hCPU[i + j * lda], hGPU[i + j * lda]);
#else
            assert(hCPU[i + j * lda] == hGPU[i + j * lda]);
#endif
        }
    }
}

template <>
void unit_check_general(
    rocsparse_int M, rocsparse_int N, rocsparse_int lda, size_t* hCPU, size_t* hGPU)
{
    for(rocsparse_int j = 0; j < N; j++)
    {
        for(rocsparse_int i = 0; i < M; i++)
        {
#ifdef GOOGLE_TEST
            ASSERT_EQ(hCPU[i + j * lda], hGPU[i + j * lda]);
#else
            assert(hCPU[i + j * lda] == hGPU[i + j * lda]);
#endif
        }
    }
}

/*! \brief Template: gtest unit compare two matrices float/double/complex */
// Do not put a wrapper over ASSERT_FLOAT_EQ, since assert exit the current function NOT the test
// case
// a wrapper will cause the loop keep going

template <>
void unit_check_near(rocsparse_int M, rocsparse_int N, rocsparse_int lda, float* hCPU, float* hGPU)
{
    for(rocsparse_int j = 0; j < N; j++)
    {
        for(rocsparse_int i = 0; i < M; i++)
        {
            float compare_val = std::max(std::abs(hCPU[i + j * lda] * 1e-3f),
                                         10 * std::numeric_limits<float>::epsilon());
#ifdef GOOGLE_TEST
            ASSERT_NEAR(hCPU[i + j * lda], hGPU[i + j * lda], compare_val);
#else
            assert(std::abs(hCPU[i + j * lda] - hGPU[i + j * lda]) < compare_val);
#endif
        }
    }
}

template <>
void unit_check_near(
    rocsparse_int M, rocsparse_int N, rocsparse_int lda, double* hCPU, double* hGPU)
{
    for(rocsparse_int j = 0; j < N; j++)
    {
        for(rocsparse_int i = 0; i < M; i++)
        {
            double compare_val = std::max(std::abs(hCPU[i + j * lda] * 1e-12),
                                          10 * std::numeric_limits<double>::epsilon());
#ifdef GOOGLE_TEST
            ASSERT_NEAR(hCPU[i + j * lda], hGPU[i + j * lda], compare_val);
#else
            assert(std::abs(hCPU[i + j * lda] - hGPU[i + j * lda]) < compare_val);
#endif
        }
    }
}
