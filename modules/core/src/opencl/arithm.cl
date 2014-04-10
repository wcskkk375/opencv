/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2010-2012, Institute Of Software Chinese Academy Of Science, all rights reserved.
// Copyright (C) 2010-2012, Advanced Micro Devices, Inc., all rights reserved.
// Copyright (C) 2013, OpenCV Foundation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// @Authors
//    Jia Haipeng, jiahaipeng95@gmail.com
//
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors as is and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the copyright holders or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

/*
  Usage:
     after compiling this program user gets a single kernel called KF.
     the following flags should be passed:
     1) one of "-D BINARY_OP", "-D UNARY_OP", "-D MASK_BINARY_OP" or "-D MASK_UNARY_OP"
     2) the actual operation performed, one of "-D OP_...", see below the list of operations.
     2a) "-D dstDepth=<destination depth> [-D cn=<num channels]"
         for some operations, like min/max/and/or/xor it's enough
     2b) "-D srcDepth1=<source1 depth> -D srcDepth2=<source2 depth> -D dstDepth=<destination depth>
          -D workDepth=<work depth> [-D cn=<num channels>]" - for mixed-type operations
*/

#ifdef DOUBLE_SUPPORT
#ifdef cl_amd_fp64
#pragma OPENCL EXTENSION cl_amd_fp64:enable
#elif defined cl_khr_fp64
#pragma OPENCL EXTENSION cl_khr_fp64:enable
#endif
#endif

#if depth <= 5
#define CV_PI M_PI_F
#else
#define CV_PI M_PI
#endif

#ifndef cn
#define cn 1
#endif

#if cn == 1
#undef srcT1_C1
#undef srcT2_C1
#undef dstT_C1
#define srcT1_C1 srcT1
#define srcT2_C1 srcT2
#define dstT_C1 dstT
#endif

#if cn != 3
    #define storedst(val) *(__global dstT *)(dstptr + dst_index) = val
    #define storedst2(val) *(__global dstT *)(dstptr2 + dst_index2) = val
#else
    #define storedst(val) vstore3(val, 0, (__global dstT_C1 *)(dstptr + dst_index))
    #define storedst2(val) vstore3(val, 0, (__global dstT_C1 *)(dstptr2 + dst_index2))
#endif

#define noconvert

#ifndef workT

    #ifndef srcT1
    #define srcT1 dstT
    #endif

    #ifndef srcT1_C1
    #define srcT1_C1 dstT_C1
    #endif

    #ifndef srcT2
    #define srcT2 dstT
    #endif

    #ifndef srcT2_C1
    #define srcT2_C1 dstT_C1
    #endif

    #define workT dstT
    #if cn != 3
        #define srcelem1 *(__global srcT1 *)(srcptr1 + src1_index)
        #define srcelem2 *(__global srcT2 *)(srcptr2 + src2_index)
    #else
        #define srcelem1 vload3(0, (__global srcT1_C1 *)(srcptr1 + src1_index))
        #define srcelem2 vload3(0, (__global srcT2_C1 *)(srcptr2 + src2_index))
    #endif
    #ifndef convertToDT
    #define convertToDT noconvert
    #endif

#else

    #ifndef convertToWT2
    #define convertToWT2 convertToWT1
    #endif
    #if cn != 3
        #define srcelem1 convertToWT1(*(__global srcT1 *)(srcptr1 + src1_index))
        #define srcelem2 convertToWT2(*(__global srcT2 *)(srcptr2 + src2_index))
    #else
        #define srcelem1 convertToWT1(vload3(0, (__global srcT1_C1 *)(srcptr1 + src1_index)))
        #define srcelem2 convertToWT2(vload3(0, (__global srcT2_C1 *)(srcptr2 + src2_index)))
    #endif

#endif

#ifndef workST
#define workST workT
#endif

#define EXTRA_PARAMS
#define EXTRA_INDEX

#if defined OP_ADD
#define PROCESS_ELEM storedst(convertToDT(srcelem1 + srcelem2))

#elif defined OP_SUB
#define PROCESS_ELEM storedst(convertToDT(srcelem1 - srcelem2))

#elif defined OP_RSUB
#define PROCESS_ELEM storedst(convertToDT(srcelem2 - srcelem1))

#elif defined OP_ABSDIFF
#define PROCESS_ELEM \
    workT v = srcelem1 - srcelem2; \
    storedst(convertToDT(v >= (workT)(0) ? v : -v))

#elif defined OP_AND
#define PROCESS_ELEM storedst(srcelem1 & srcelem2)

#elif defined OP_OR
#define PROCESS_ELEM storedst(srcelem1 | srcelem2)

#elif defined OP_XOR
#define PROCESS_ELEM storedst(srcelem1 ^ srcelem2)

#elif defined OP_NOT
#define PROCESS_ELEM storedst(~srcelem1)

#elif defined OP_MIN
#define PROCESS_ELEM storedst(min(srcelem1, srcelem2))

#elif defined OP_MAX
#define PROCESS_ELEM storedst(max(srcelem1, srcelem2))

#elif defined OP_MUL
#define PROCESS_ELEM storedst(convertToDT(srcelem1 * srcelem2))

#elif defined OP_MUL_SCALE
#undef EXTRA_PARAMS
#ifdef UNARY_OP
#define EXTRA_PARAMS , workST srcelem2_, scaleT scale
#undef srcelem2
#define srcelem2 srcelem2_
#else
#define EXTRA_PARAMS , scaleT scale
#endif
#define PROCESS_ELEM storedst(convertToDT(srcelem1 * scale * srcelem2))

#elif defined OP_DIV
#define PROCESS_ELEM \
        workT e2 = srcelem2, zero = (workT)(0); \
        storedst(convertToDT(e2 != zero ? srcelem1 / e2 : zero))

#elif defined OP_DIV_SCALE
#undef EXTRA_PARAMS
#ifdef UNARY_OP
#define EXTRA_PARAMS , workST srcelem2_, scaleT scale
#undef srcelem2
#define srcelem2 srcelem2_
#else
#define EXTRA_PARAMS , scaleT scale
#endif
#define PROCESS_ELEM \
        workT e2 = srcelem2, zero = (workT)(0); \
        storedst(convertToDT(e2 == zero ? zero : (srcelem1 * (workT)(scale) / e2)))

#elif defined OP_RDIV_SCALE
#undef EXTRA_PARAMS
#ifdef UNARY_OP
#define EXTRA_PARAMS , workST srcelem2_, scaleT scale
#undef srcelem2
#define srcelem2 srcelem2_
#else
#define EXTRA_PARAMS , scaleT scale
#endif
#define PROCESS_ELEM \
        workT e1 = srcelem1, zero = (workT)(0); \
        storedst(convertToDT(e1 == zero ? zero : (srcelem2 * (workT)(scale) / e1)))

#elif defined OP_RECIP_SCALE
#undef EXTRA_PARAMS
#define EXTRA_PARAMS , scaleT scale
#define PROCESS_ELEM \
        workT e1 = srcelem1, zero = (workT)(0); \
        storedst(convertToDT(e1 != zero ? scale / e1 : zero))

#elif defined OP_ADDW
#undef EXTRA_PARAMS
#define EXTRA_PARAMS , scaleT alpha, scaleT beta, scaleT gamma
#if wdepth <= 4
#define PROCESS_ELEM storedst(convertToDT(mad24(srcelem1, alpha, mad24(srcelem2, beta, gamma))))
#else
#define PROCESS_ELEM storedst(convertToDT(mad(srcelem1, alpha, mad(srcelem2, beta, gamma))))
#endif

#elif defined OP_MAG
#define PROCESS_ELEM storedst(hypot(srcelem1, srcelem2))

#elif defined OP_ABS_NOSAT
#define PROCESS_ELEM \
    dstT v = convertToDT(srcelem1); \
    storedst(v >= 0 ? v : -v)

#elif defined OP_PHASE_RADIANS
#define PROCESS_ELEM \
        workT tmp = atan2(srcelem2, srcelem1); \
        if(tmp < 0) tmp += 6.283185307179586232f; \
        storedst(tmp)

#elif defined OP_PHASE_DEGREES
    #define PROCESS_ELEM \
    workT tmp = atan2(srcelem2, srcelem1)*57.29577951308232286465f; \
    if(tmp < 0) tmp += 360; \
    storedst(tmp)

#elif defined OP_EXP
#define PROCESS_ELEM storedst(exp(srcelem1))

#elif defined OP_POW
#define PROCESS_ELEM storedst(pow(srcelem1, srcelem2))

#elif defined OP_POWN
#undef workT
#define workT int
#define PROCESS_ELEM storedst(pown(srcelem1, srcelem2))

#elif defined OP_SQRT
#define PROCESS_ELEM storedst(sqrt(srcelem1))

#elif defined OP_LOG
#define PROCESS_ELEM \
    dstT v = (dstT)(srcelem1);\
    storedst(v > (dstT)(0) ? log(v) : log(-v))

#elif defined OP_CMP
#define srcT2 srcT1
#ifndef convertToWT1
#define convertToWT1
#endif
#define PROCESS_ELEM \
    workT __s1 = srcelem1; \
    workT __s2 = srcelem2; \
    storedst(((__s1 CMP_OPERATOR __s2) ? (dstT)(255) : (dstT)(0)))

#elif defined OP_CONVERT_SCALE_ABS
#undef EXTRA_PARAMS
#define EXTRA_PARAMS , workT1 alpha, workT1 beta
#if wdepth <= 4
#define PROCESS_ELEM \
    workT value = mad24(srcelem1, (workT)(alpha), (workT)(beta)); \
    storedst(convertToDT(value >= 0 ? value : -value))
#else
#define PROCESS_ELEM \
    workT value = mad(srcelem1, (workT)(alpha), (workT)(beta)); \
    storedst(convertToDT(value >= 0 ? value : -value))
#endif

#elif defined OP_SCALE_ADD
#undef EXTRA_PARAMS
#define EXTRA_PARAMS , workT1 alpha
#if wdepth <= 4
#define PROCESS_ELEM storedst(convertToDT(mad24(srcelem1, (workT)(alpha), srcelem2)))
#else
#define PROCESS_ELEM storedst(convertToDT(mad(srcelem1, (workT)(alpha), srcelem2)))
#endif

#elif defined OP_CTP_AD || defined OP_CTP_AR
#if depth <= 5
#define CV_EPSILON FLT_EPSILON
#else
#define CV_EPSILON DBL_EPSILON
#endif
#ifdef OP_CTP_AD
#define TO_DEGREE cartToPolar *= (180 / CV_PI);
#elif defined OP_CTP_AR
#define TO_DEGREE
#endif
#define PROCESS_ELEM \
    dstT x = srcelem1, y = srcelem2; \
    dstT x2 = x * x, y2 = y * y; \
    dstT magnitude = sqrt(x2 + y2); \
    dstT tmp = y >= 0 ? 0 : CV_PI * 2; \
    tmp = x < 0 ? CV_PI : tmp; \
    dstT tmp1 = y >= 0 ? CV_PI * 0.5f : CV_PI * 1.5f; \
    dstT cartToPolar = y2 <= x2 ? x * y / mad((dstT)(0.28f), y2, x2 + CV_EPSILON) + tmp : (tmp1 - x * y / mad((dstT)(0.28f), x2, y2 + CV_EPSILON)); \
    TO_DEGREE \
    storedst(magnitude); \
    storedst2(cartToPolar)

#elif defined OP_PTC_AD || defined OP_PTC_AR
#ifdef OP_PTC_AD
#define FROM_DEGREE \
    dstT ascale = CV_PI/180.0f; \
    dstT alpha = y * ascale
#else
#define FROM_DEGREE \
    dstT alpha = y
#endif
#define PROCESS_ELEM \
    dstT x = srcelem1, y = srcelem2; \
    FROM_DEGREE; \
    storedst(cos(alpha) * x); \
    storedst2(sin(alpha) * x)

#elif defined OP_PATCH_NANS
#undef EXTRA_PARAMS
#define EXTRA_PARAMS , int val
#define PROCESS_ELEM \
    if (( srcelem1 & 0x7fffffff) > 0x7f800000 ) \
        storedst(val)

#else
#error "unknown op type"
#endif

#if defined OP_CTP_AD || defined OP_CTP_AR || defined OP_PTC_AD || defined OP_PTC_AR
    #undef EXTRA_PARAMS
    #define EXTRA_PARAMS , __global uchar* dstptr2, int dststep2, int dstoffset2
    #undef EXTRA_INDEX
    #define EXTRA_INDEX int dst_index2 = mad24(y, dststep2, mad24(x, (int)sizeof(dstT_C1) * cn, dstoffset2))
#endif

#if defined UNARY_OP || defined MASK_UNARY_OP

#if defined OP_AND || defined OP_OR || defined OP_XOR || defined OP_ADD || defined OP_SAT_ADD || \
    defined OP_SUB || defined OP_SAT_SUB || defined OP_RSUB || defined OP_SAT_RSUB || \
    defined OP_ABSDIFF || defined OP_CMP || defined OP_MIN || defined OP_MAX || defined OP_POW || \
    defined OP_MUL || defined OP_DIV || defined OP_POWN
    #undef EXTRA_PARAMS
    #define EXTRA_PARAMS , workST srcelem2_
    #undef srcelem2
    #define srcelem2 srcelem2_
#endif

#if cn == 3
#undef srcelem2
#define srcelem2 (workT)(srcelem2_.x, srcelem2_.y, srcelem2_.z)
#endif

#endif

#if defined BINARY_OP

__kernel void KF(__global const uchar * srcptr1, int srcstep1, int srcoffset1,
                 __global const uchar * srcptr2, int srcstep2, int srcoffset2,
                 __global uchar * dstptr, int dststep, int dstoffset,
                 int rows, int cols EXTRA_PARAMS )
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x < cols && y < rows)
    {
        int src1_index = mad24(y, srcstep1, mad24(x, (int)sizeof(srcT1_C1) * cn, srcoffset1));
#if !(defined(OP_RECIP_SCALE) || defined(OP_NOT))
        int src2_index = mad24(y, srcstep2, mad24(x, (int)sizeof(srcT2_C1) * cn, srcoffset2));
#endif
        int dst_index  = mad24(y, dststep, mad24(x, (int)sizeof(dstT_C1) * cn, dstoffset));
        EXTRA_INDEX;

        PROCESS_ELEM;
    }
}

#elif defined MASK_BINARY_OP

__kernel void KF(__global const uchar * srcptr1, int srcstep1, int srcoffset1,
                 __global const uchar * srcptr2, int srcstep2, int srcoffset2,
                 __global const uchar * mask, int maskstep, int maskoffset,
                 __global uchar * dstptr, int dststep, int dstoffset,
                 int rows, int cols EXTRA_PARAMS )
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x < cols && y < rows)
    {
        int mask_index = mad24(y, maskstep, x + maskoffset);
        if( mask[mask_index] )
        {
            int src1_index = mad24(y, srcstep1, mad24(x, (int)sizeof(srcT1_C1) * cn, srcoffset1));
            int src2_index = mad24(y, srcstep2, mad24(x, (int)sizeof(srcT2_C1) * cn, srcoffset2));
            int dst_index  = mad24(y, dststep, mad24(x, (int)sizeof(dstT_C1) * cn, dstoffset));

            PROCESS_ELEM;
        }
    }
}

#elif defined UNARY_OP

__kernel void KF(__global const uchar * srcptr1, int srcstep1, int srcoffset1,
                 __global uchar * dstptr, int dststep, int dstoffset,
                 int rows, int cols EXTRA_PARAMS )
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x < cols && y < rows)
    {
        int src1_index = mad24(y, srcstep1, mad24(x, (int)sizeof(srcT1_C1) * cn, srcoffset1));
        int dst_index  = mad24(y, dststep, mad24(x, (int)sizeof(dstT_C1) * cn, dstoffset));

        PROCESS_ELEM;
    }
}

#elif defined MASK_UNARY_OP

__kernel void KF(__global const uchar * srcptr1, int srcstep1, int srcoffset1,
                 __global const uchar * mask, int maskstep, int maskoffset,
                 __global uchar * dstptr, int dststep, int dstoffset,
                 int rows, int cols EXTRA_PARAMS )
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x < cols && y < rows)
    {
        int mask_index = mad24(y, maskstep, x + maskoffset);
        if( mask[mask_index] )
        {
            int src1_index = mad24(y, srcstep1, mad24(x, (int)sizeof(srcT1_C1) * cn, srcoffset1));
            int dst_index  = mad24(y, dststep, mad24(x, (int)sizeof(dstT_C1) * cn, dstoffset));

            PROCESS_ELEM;
        }
    }
}

#else

#error "Unknown operation type"

#endif
