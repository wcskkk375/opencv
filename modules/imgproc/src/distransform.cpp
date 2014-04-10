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
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Copyright (C) 2013, OpenCV Foundation, all rights reserved.
// Third party copyrights are property of their respective owners.
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
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/
#include "precomp.hpp"

namespace cv
{

static const int DIST_SHIFT = 16;
static const int INIT_DIST0 = (INT_MAX >> 2);
#define  CV_FLT_TO_FIX(x,n)  cvRound((x)*(1<<(n)))

static void
initTopBottom( Mat& temp, int border )
{
    Size size = temp.size();
    for( int i = 0; i < border; i++ )
    {
        int* ttop = temp.ptr<int>(i);
        int* tbottom = temp.ptr<int>(size.height - i - 1);

        for( int j = 0; j < size.width; j++ )
        {
            ttop[j] = INIT_DIST0;
            tbottom[j] = INIT_DIST0;
        }
    }
}


static void
distanceTransform_3x3( const Mat& _src, Mat& _temp, Mat& _dist, const float* metrics )
{
    const int BORDER = 1;
    int i, j;
    const int HV_DIST = CV_FLT_TO_FIX( metrics[0], DIST_SHIFT );
    const int DIAG_DIST = CV_FLT_TO_FIX( metrics[1], DIST_SHIFT );
    const float scale = 1.f/(1 << DIST_SHIFT);

    const uchar* src = _src.data;
    int* temp = _temp.ptr<int>();
    float* dist = _dist.ptr<float>();
    int srcstep = (int)(_src.step/sizeof(src[0]));
    int step = (int)(_temp.step/sizeof(temp[0]));
    int dststep = (int)(_dist.step/sizeof(dist[0]));
    Size size = _src.size();

    initTopBottom( _temp, BORDER );

    // forward pass
    for( i = 0; i < size.height; i++ )
    {
        const uchar* s = src + i*srcstep;
        int* tmp = (int*)(temp + (i+BORDER)*step) + BORDER;

        for( j = 0; j < BORDER; j++ )
            tmp[-j-1] = tmp[size.width + j] = INIT_DIST0;

        for( j = 0; j < size.width; j++ )
        {
            if( !s[j] )
                tmp[j] = 0;
            else
            {
                int t0 = tmp[j-step-1] + DIAG_DIST;
                int t = tmp[j-step] + HV_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-step+1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-1] + HV_DIST;
                if( t0 > t ) t0 = t;
                tmp[j] = t0;
            }
        }
    }

    // backward pass
    for( i = size.height - 1; i >= 0; i-- )
    {
        float* d = (float*)(dist + i*dststep);
        int* tmp = (int*)(temp + (i+BORDER)*step) + BORDER;

        for( j = size.width - 1; j >= 0; j-- )
        {
            int t0 = tmp[j];
            if( t0 > HV_DIST )
            {
                int t = tmp[j+step+1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step] + HV_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step-1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+1] + HV_DIST;
                if( t0 > t ) t0 = t;
                tmp[j] = t0;
            }
            d[j] = (float)(t0 * scale);
        }
    }
}


static void
distanceTransform_5x5( const Mat& _src, Mat& _temp, Mat& _dist, const float* metrics )
{
    const int BORDER = 2;
    int i, j;
    const int HV_DIST = CV_FLT_TO_FIX( metrics[0], DIST_SHIFT );
    const int DIAG_DIST = CV_FLT_TO_FIX( metrics[1], DIST_SHIFT );
    const int LONG_DIST = CV_FLT_TO_FIX( metrics[2], DIST_SHIFT );
    const float scale = 1.f/(1 << DIST_SHIFT);

    const uchar* src = _src.data;
    int* temp = _temp.ptr<int>();
    float* dist = _dist.ptr<float>();
    int srcstep = (int)(_src.step/sizeof(src[0]));
    int step = (int)(_temp.step/sizeof(temp[0]));
    int dststep = (int)(_dist.step/sizeof(dist[0]));
    Size size = _src.size();

    initTopBottom( _temp, BORDER );

    // forward pass
    for( i = 0; i < size.height; i++ )
    {
        const uchar* s = src + i*srcstep;
        int* tmp = (int*)(temp + (i+BORDER)*step) + BORDER;

        for( j = 0; j < BORDER; j++ )
            tmp[-j-1] = tmp[size.width + j] = INIT_DIST0;

        for( j = 0; j < size.width; j++ )
        {
            if( !s[j] )
                tmp[j] = 0;
            else
            {
                int t0 = tmp[j-step*2-1] + LONG_DIST;
                int t = tmp[j-step*2+1] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-step-2] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-step-1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-step] + HV_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-step+1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-step+2] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-1] + HV_DIST;
                if( t0 > t ) t0 = t;
                tmp[j] = t0;
            }
        }
    }

    // backward pass
    for( i = size.height - 1; i >= 0; i-- )
    {
        float* d = (float*)(dist + i*dststep);
        int* tmp = (int*)(temp + (i+BORDER)*step) + BORDER;

        for( j = size.width - 1; j >= 0; j-- )
        {
            int t0 = tmp[j];
            if( t0 > HV_DIST )
            {
                int t = tmp[j+step*2+1] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step*2-1] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step+2] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step+1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step] + HV_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step-1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step-2] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+1] + HV_DIST;
                if( t0 > t ) t0 = t;
                tmp[j] = t0;
            }
            d[j] = (float)(t0 * scale);
        }
    }
}


static void
distanceTransformEx_5x5( const Mat& _src, Mat& _temp, Mat& _dist, Mat& _labels, const float* metrics )
{
    const int BORDER = 2;

    int i, j;
    const int HV_DIST = CV_FLT_TO_FIX( metrics[0], DIST_SHIFT );
    const int DIAG_DIST = CV_FLT_TO_FIX( metrics[1], DIST_SHIFT );
    const int LONG_DIST = CV_FLT_TO_FIX( metrics[2], DIST_SHIFT );
    const float scale = 1.f/(1 << DIST_SHIFT);

    const uchar* src = _src.data;
    int* temp = _temp.ptr<int>();
    float* dist = _dist.ptr<float>();
    int* labels = _labels.ptr<int>();
    int srcstep = (int)(_src.step/sizeof(src[0]));
    int step = (int)(_temp.step/sizeof(temp[0]));
    int dststep = (int)(_dist.step/sizeof(dist[0]));
    int lstep = (int)(_labels.step/sizeof(dist[0]));
    Size size = _src.size();

    initTopBottom( _temp, BORDER );

    // forward pass
    for( i = 0; i < size.height; i++ )
    {
        const uchar* s = src + i*srcstep;
        int* tmp = (int*)(temp + (i+BORDER)*step) + BORDER;
        int* lls = (int*)(labels + i*lstep);

        for( j = 0; j < BORDER; j++ )
            tmp[-j-1] = tmp[size.width + j] = INIT_DIST0;

        for( j = 0; j < size.width; j++ )
        {
            if( !s[j] )
            {
                tmp[j] = 0;
                //assert( lls[j] != 0 );
            }
            else
            {
                int t0 = INIT_DIST0, t;
                int l0 = 0;

                t = tmp[j-step*2-1] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep*2-1];
                }
                t = tmp[j-step*2+1] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep*2+1];
                }
                t = tmp[j-step-2] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep-2];
                }
                t = tmp[j-step-1] + DIAG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep-1];
                }
                t = tmp[j-step] + HV_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep];
                }
                t = tmp[j-step+1] + DIAG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep+1];
                }
                t = tmp[j-step+2] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep+2];
                }
                t = tmp[j-1] + HV_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-1];
                }

                tmp[j] = t0;
                lls[j] = l0;
            }
        }
    }

    // backward pass
    for( i = size.height - 1; i >= 0; i-- )
    {
        float* d = (float*)(dist + i*dststep);
        int* tmp = (int*)(temp + (i+BORDER)*step) + BORDER;
        int* lls = (int*)(labels + i*lstep);

        for( j = size.width - 1; j >= 0; j-- )
        {
            int t0 = tmp[j];
            int l0 = lls[j];
            if( t0 > HV_DIST )
            {
                int t = tmp[j+step*2+1] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep*2+1];
                }
                t = tmp[j+step*2-1] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep*2-1];
                }
                t = tmp[j+step+2] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep+2];
                }
                t = tmp[j+step+1] + DIAG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep+1];
                }
                t = tmp[j+step] + HV_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep];
                }
                t = tmp[j+step-1] + DIAG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep-1];
                }
                t = tmp[j+step-2] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep-2];
                }
                t = tmp[j+1] + HV_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+1];
                }
                tmp[j] = t0;
                lls[j] = l0;
            }
            d[j] = (float)(t0 * scale);
        }
    }
}


static void getDistanceTransformMask( int maskType, float *metrics )
{
    CV_Assert( metrics != 0 );

    switch (maskType)
    {
    case 30:
        metrics[0] = 1.0f;
        metrics[1] = 1.0f;
        break;

    case 31:
        metrics[0] = 1.0f;
        metrics[1] = 2.0f;
        break;

    case 32:
        metrics[0] = 0.955f;
        metrics[1] = 1.3693f;
        break;

    case 50:
        metrics[0] = 1.0f;
        metrics[1] = 1.0f;
        metrics[2] = 2.0f;
        break;

    case 51:
        metrics[0] = 1.0f;
        metrics[1] = 2.0f;
        metrics[2] = 3.0f;
        break;

    case 52:
        metrics[0] = 1.0f;
        metrics[1] = 1.4f;
        metrics[2] = 2.1969f;
        break;
    default:
        CV_Error(CV_StsBadArg, "Uknown metric type");
    }
}

struct DTColumnInvoker : ParallelLoopBody
{
    DTColumnInvoker( const Mat* _src, Mat* _dst, const int* _sat_tab, const float* _sqr_tab)
    {
        src = _src;
        dst = _dst;
        sat_tab = _sat_tab + src->rows*2 + 1;
        sqr_tab = _sqr_tab;
    }

    void operator()( const Range& range ) const
    {
        int i, i1 = range.start, i2 = range.end;
        int m = src->rows;
        size_t sstep = src->step, dstep = dst->step/sizeof(float);
        AutoBuffer<int> _d(m);
        int* d = _d;

        for( i = i1; i < i2; i++ )
        {
            const uchar* sptr = src->ptr(m-1) + i;
            float* dptr = dst->ptr<float>() + i;
            int j, dist = m-1;

            for( j = m-1; j >= 0; j--, sptr -= sstep )
            {
                dist = (dist + 1) & (sptr[0] == 0 ? 0 : -1);
                d[j] = dist;
            }

            dist = m-1;
            for( j = 0; j < m; j++, dptr += dstep )
            {
                dist = dist + 1 - sat_tab[dist - d[j]];
                d[j] = dist;
                dptr[0] = sqr_tab[dist];
            }
        }
    }

    const Mat* src;
    Mat* dst;
    const int* sat_tab;
    const float* sqr_tab;
};


struct DTRowInvoker : ParallelLoopBody
{
    DTRowInvoker( Mat* _dst, const float* _sqr_tab, const float* _inv_tab )
    {
        dst = _dst;
        sqr_tab = _sqr_tab;
        inv_tab = _inv_tab;
    }

    void operator()( const Range& range ) const
    {
        const float inf = 1e15f;
        int i, i1 = range.start, i2 = range.end;
        int n = dst->cols;
        AutoBuffer<uchar> _buf((n+2)*2*sizeof(float) + (n+2)*sizeof(int));
        float* f = (float*)(uchar*)_buf;
        float* z = f + n;
        int* v = alignPtr((int*)(z + n + 1), sizeof(int));

        for( i = i1; i < i2; i++ )
        {
            float* d = dst->ptr<float>(i);
            int p, q, k;

            v[0] = 0;
            z[0] = -inf;
            z[1] = inf;
            f[0] = d[0];

            for( q = 1, k = 0; q < n; q++ )
            {
                float fq = d[q];
                f[q] = fq;

                for(;;k--)
                {
                    p = v[k];
                    float s = (fq + sqr_tab[q] - d[p] - sqr_tab[p])*inv_tab[q - p];
                    if( s > z[k] )
                    {
                        k++;
                        v[k] = q;
                        z[k] = s;
                        z[k+1] = inf;
                        break;
                    }
                }
            }

            for( q = 0, k = 0; q < n; q++ )
            {
                while( z[k+1] < q )
                    k++;
                p = v[k];
                d[q] = std::sqrt(sqr_tab[std::abs(q - p)] + f[p]);
            }
        }
    }

    Mat* dst;
    const float* sqr_tab;
    const float* inv_tab;
};

static void
trueDistTrans( const Mat& src, Mat& dst )
{
    const float inf = 1e15f;

    CV_Assert( src.size() == dst.size() );

    CV_Assert( src.type() == CV_8UC1 && dst.type() == CV_32FC1 );
    int i, m = src.rows, n = src.cols;

    cv::AutoBuffer<uchar> _buf(std::max(m*2*sizeof(float) + (m*3+1)*sizeof(int), n*2*sizeof(float)));
    // stage 1: compute 1d distance transform of each column
    float* sqr_tab = (float*)(uchar*)_buf;
    int* sat_tab = cv::alignPtr((int*)(sqr_tab + m*2), sizeof(int));
    int shift = m*2;

    for( i = 0; i < m; i++ )
        sqr_tab[i] = (float)(i*i);
    for( i = m; i < m*2; i++ )
        sqr_tab[i] = inf;
    for( i = 0; i < shift; i++ )
        sat_tab[i] = 0;
    for( ; i <= m*3; i++ )
        sat_tab[i] = i - shift;

    cv::parallel_for_(cv::Range(0, n), cv::DTColumnInvoker(&src, &dst, sat_tab, sqr_tab));

    // stage 2: compute modified distance transform for each row
    float* inv_tab = sqr_tab + n;

    inv_tab[0] = sqr_tab[0] = 0.f;
    for( i = 1; i < n; i++ )
    {
        inv_tab[i] = (float)(0.5/i);
        sqr_tab[i] = (float)(i*i);
    }

    cv::parallel_for_(cv::Range(0, m), cv::DTRowInvoker(&dst, sqr_tab, inv_tab));
}


/****************************************************************************************\
 Non-inplace and Inplace 8u->8u Distance Transform for CityBlock (a.k.a. L1) metric
 (C) 2006 by Jay Stavinzky.
\****************************************************************************************/

//BEGIN ATS ADDITION
// 8-bit grayscale distance transform function
static void
distanceATS_L1_8u( const Mat& src, Mat& dst )
{
    int width = src.cols, height = src.rows;

    int a;
    uchar lut[256];
    int x, y;

    const uchar *sbase = src.data;
    uchar *dbase = dst.data;
    int srcstep = (int)src.step;
    int dststep = (int)dst.step;

    CV_Assert( src.type() == CV_8UC1 && dst.type() == CV_8UC1 );
    CV_Assert( src.size() == dst.size() );

    ////////////////////// forward scan ////////////////////////
    for( x = 0; x < 256; x++ )
        lut[x] = cv::saturate_cast<uchar>(x+1);

    //init first pixel to max (we're going to be skipping it)
    dbase[0] = (uchar)(sbase[0] == 0 ? 0 : 255);

    //first row (scan west only, skip first pixel)
    for( x = 1; x < width; x++ )
        dbase[x] = (uchar)(sbase[x] == 0 ? 0 : lut[dbase[x-1]]);

    for( y = 1; y < height; y++ )
    {
        sbase += srcstep;
        dbase += dststep;

        //for left edge, scan north only
        a = sbase[0] == 0 ? 0 : lut[dbase[-dststep]];
        dbase[0] = (uchar)a;

        for( x = 1; x < width; x++ )
        {
            a = sbase[x] == 0 ? 0 : lut[MIN(a, dbase[x - dststep])];
            dbase[x] = (uchar)a;
        }
    }

    ////////////////////// backward scan ///////////////////////

    a = dbase[width-1];

    // do last row east pixel scan here (skip bottom right pixel)
    for( x = width - 2; x >= 0; x-- )
    {
        a = lut[a];
        dbase[x] = (uchar)(CV_CALC_MIN_8U(a, dbase[x]));
    }

    // right edge is the only error case
    for( y = height - 2; y >= 0; y-- )
    {
        dbase -= dststep;

        // do right edge
        a = lut[dbase[width-1+dststep]];
        dbase[width-1] = (uchar)(MIN(a, dbase[width-1]));

        for( x = width - 2; x >= 0; x-- )
        {
            int b = dbase[x+dststep];
            a = lut[MIN(a, b)];
            dbase[x] = (uchar)(MIN(a, dbase[x]));
        }
    }
}
//END ATS ADDITION

}


// Wrapper function for distance transform group
void cv::distanceTransform( InputArray _src, OutputArray _dst, OutputArray _labels,
                            int distType, int maskSize, int labelType )
{
    Mat src = _src.getMat(), dst = _dst.getMat(), labels;
    bool need_labels = _labels.needed();

    CV_Assert( src.type() == CV_8U );
    if( dst.size == src.size && dst.type() == CV_8U && !need_labels && distType == CV_DIST_L1 )
    {
        distanceATS_L1_8u(src, dst);
        return;
    }

    _dst.create( src.size(), CV_32F );
    dst = _dst.getMat();

    if( need_labels )
    {
        CV_Assert( labelType == DIST_LABEL_PIXEL || labelType == DIST_LABEL_CCOMP );

        _labels.create(src.size(), CV_32S);
        labels = _labels.getMat();
        maskSize = CV_DIST_MASK_5;
    }

    CV_Assert( src.type() == CV_8UC1 );
    float _mask[5] = {0};

    if( maskSize != CV_DIST_MASK_3 && maskSize != CV_DIST_MASK_5 && maskSize != CV_DIST_MASK_PRECISE )
        CV_Error( CV_StsBadSize, "Mask size should be 3 or 5 or 0 (presize)" );

    if( distType == CV_DIST_C || distType == CV_DIST_L1 )
        maskSize = !need_labels ? CV_DIST_MASK_3 : CV_DIST_MASK_5;
    else if( distType == CV_DIST_L2 && need_labels )
        maskSize = CV_DIST_MASK_5;

    if( maskSize == CV_DIST_MASK_PRECISE )
    {
        trueDistTrans( src, dst );
        return;
    }

    CV_Assert( distType == CV_DIST_C || distType == CV_DIST_L1 || distType == CV_DIST_L2 );

    getDistanceTransformMask( (distType == CV_DIST_C ? 0 :
        distType == CV_DIST_L1 ? 1 : 2) + maskSize*10, _mask );

    Size size = src.size();

    int border = maskSize == CV_DIST_MASK_3 ? 1 : 2;
    Mat temp( size.height + border*2, size.width + border*2, CV_32SC1 );

    if( !need_labels )
    {
        if( maskSize == CV_DIST_MASK_3 )
            distanceTransform_3x3(src, temp, dst, _mask);
        else
            distanceTransform_5x5(src, temp, dst, _mask);
    }
    else
    {
        labels.setTo(Scalar::all(0));

        if( labelType == CV_DIST_LABEL_CCOMP )
        {
        #if defined (HAVE_IPP) && (IPP_VERSION_MAJOR >= 7)
            if( maskSize == CV_DIST_MASK_5 )
            {
                IppiSize roi = { src.cols, src.rows };
                if( ippiDistanceTransform_5x5_8u32f_C1R(
                        src.ptr<uchar>(), (int)src.step,
                        dst.ptr<float>(), (int)dst.step, roi, _mask) >= 0 )
                    return;
            }
        #endif
            Mat zpix = src == 0;
            connectedComponents(zpix, labels, 8, CV_32S);
        }
        else
        {
            int k = 1;
            for( int i = 0; i < src.rows; i++ )
            {
                const uchar* srcptr = src.ptr(i);
                int* labelptr = labels.ptr<int>(i);

                for( int j = 0; j < src.cols; j++ )
                    if( srcptr[j] == 0 )
                        labelptr[j] = k++;
            }
        }

        distanceTransformEx_5x5( src, temp, dst, labels, _mask );
    }
}


void cv::distanceTransform( InputArray _src, OutputArray _dst,
                            int distanceType, int maskSize )
{
    distanceTransform(_src, _dst, noArray(), distanceType, maskSize, DIST_LABEL_PIXEL);
}


CV_IMPL void
cvDistTransform( const void* srcarr, void* dstarr,
                int distType, int maskSize,
                const float * /*mask*/,
                void* labelsarr, int labelType )
{
    cv::Mat src = cv::cvarrToMat(srcarr);
    const cv::Mat dst = cv::cvarrToMat(dstarr);
    const cv::Mat labels = cv::cvarrToMat(labelsarr);

    cv::distanceTransform(src, dst, labelsarr ? cv::_OutputArray(labels) : cv::_OutputArray(),
                          distType, maskSize, labelType);

}


/* End of file. */
