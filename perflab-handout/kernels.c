/********************************************************
 * Kernels to be optimized for the CS:APP Performance Lab
 ********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

/*
 * Please fill in the following team struct
 */
team_t team = {
    "mzzdx_god", /* Team name */

    "mzzdx",        /* First member full name */
    "mzzdx@qq.com", /* First member email address */

    "", /* Second member full name (leave blank if none) */
    ""  /* Second member email addr (leave blank if none) */
};

/***************
 * ROTATE KERNEL
 ***************/

/******************************************************
 * Your different versions of the rotate kernel go here
 ******************************************************/

/*
 * naive_rotate - The naive baseline version of rotate
 */
char naive_rotate_descr[] = "naive_rotate: Naive baseline implementation";
void naive_rotate(int dim, pixel *src, pixel *dst)
{
    int i, j;

    for (i = 0; i < dim; i++)
    {
        for (j = 0; j < dim; j++)
        {
            dst[RIDX(dim - 1 - j, i, dim)] = src[RIDX(i, j, dim)];
        }
    }
}

/*
 * rotate - Your current working version of rotate
 * IMPORTANT: This is the version you will be graded on
 */
char rotate_descr[] = "rotate: 使用循环展开, 矩阵分块和交换数组元素读写顺序";
void rotate(int dim, pixel *src, pixel *dst)
{
    int i, j, k, t;

    for (i = 0; i < dim; i += 32)
    {
        for (j = 0; j < dim; j += 32)
        {
            for (k = i; k < i + 32; k++)
            {
                for (t = j; t < j + 32; t++)
                {
                    dst[RIDX(k, t, dim)] = src[RIDX(t, dim - k - 1, dim)];
                }
            }
        }
    }
}

/*********************************************************************
 * register_rotate_functions - Register all of your different versions
 *     of the rotate kernel with the driver by calling the
 *     add_rotate_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.
 *********************************************************************/

void register_rotate_functions()
{
    add_rotate_function(&naive_rotate, naive_rotate_descr);
    add_rotate_function(&rotate, rotate_descr);
}

/***************
 * SMOOTH KERNEL
 **************/

/***************************************************************
 * Various typedefs and helper functions for the smooth function
 * You may modify these any way you like.
 **************************************************************/

/* A struct used to compute averaged pixel value */
typedef struct
{
    int red;
    int green;
    int blue;
    int num;
} pixel_sum;

/* Compute min and max of two integers, respectively */
static int min(int a, int b) { return (a < b ? a : b); }
static int max(int a, int b) { return (a > b ? a : b); }

/*
 * initialize_pixel_sum - Initializes all fields of sum to 0
 */
static void initialize_pixel_sum(pixel_sum *sum)
{
    sum->red = sum->green = sum->blue = 0;
    sum->num = 0;
    return;
}

/*
 * accumulate_sum - Accumulates field values of p in corresponding
 * fields of sum
 */
static void accumulate_sum(pixel_sum *sum, pixel p)
{
    sum->red += (int)p.red;
    sum->green += (int)p.green;
    sum->blue += (int)p.blue;
    sum->num++;
    return;
}

/*
 * assign_sum_to_pixel - Computes averaged pixel value in current_pixel
 */
static void assign_sum_to_pixel(pixel *current_pixel, pixel_sum sum)
{
    current_pixel->red = (unsigned short)(sum.red / sum.num);
    current_pixel->green = (unsigned short)(sum.green / sum.num);
    current_pixel->blue = (unsigned short)(sum.blue / sum.num);
    return;
}

/*
 * avg - Returns averaged pixel value at (i,j)
 */
static pixel avg(int dim, int i, int j, pixel *src)
{
    int ii, jj;
    pixel_sum sum;
    pixel current_pixel;

    initialize_pixel_sum(&sum);
    for (ii = max(i - 1, 0); ii <= min(i + 1, dim - 1); ii++)
    {
        for (jj = max(j - 1, 0); jj <= min(j + 1, dim - 1); jj++)
        {
            accumulate_sum(&sum, src[RIDX(ii, jj, dim)]);
        }
    }
    assign_sum_to_pixel(&current_pixel, sum);
    return current_pixel;
}

static void set_corner_val(pixel *src, pixel *dst, int a, int b, int c, int d)
{
    dst[a].red = (src[a].red + src[b].red + src[c].red + src[d].red) >> 2;
    dst[a].green = (src[a].green + src[b].green + src[c].green + src[d].green) >> 2;
    dst[a].blue = (src[a].blue + src[b].blue + src[c].blue + src[d].blue) >> 2;
}

static void set_top_val(pixel *s, pixel *d, int i, int dim)
{
    d[i].red = (s[i].red + s[i - 1].red + s[i + 1].red + s[i + dim].red + s[i + dim - 1].red + s[i + dim + 1].red) / 6;
    d[i].green = (s[i].green + s[i - 1].green + s[i + 1].green + s[i + dim].green + s[i + dim - 1].green + s[i + dim + 1].green) / 6;
    d[i].blue = (s[i].blue + s[i - 1].blue + s[i + 1].blue + s[i + dim].blue + s[i + dim - 1].blue + s[i + dim + 1].blue) / 6;
}

static void set_left_val(pixel *s, pixel *d, int i, int dim)
{
    d[i].red = (s[i].red + s[i + 1].red + s[i - dim].red + s[i + dim].red + s[i - dim + 1].red + s[i + dim + 1].red) / 6;
    d[i].green = (s[i].green + s[i + 1].green + s[i - dim].green + s[i + dim].green + s[i - dim + 1].green + s[i + dim + 1].green) / 6;
    d[i].blue = (s[i].blue + s[i + 1].blue + s[i - dim].blue + s[i + dim].blue + s[i - dim + 1].blue + s[i + dim + 1].blue) / 6;
}

static void set_right_val(pixel *s, pixel *d, int i, int dim)
{
    d[i].red = (s[i].red + s[i - 1].red + s[i - dim].red + s[i + dim].red + s[i - dim - 1].red + s[i + dim - 1].red) / 6;
    d[i].green = (s[i].green + s[i - 1].green + s[i - dim].green + s[i + dim].green + s[i - dim - 1].green + s[i + dim - 1].green) / 6;
    d[i].blue = (s[i].blue + s[i - 1].blue + s[i - dim].blue + s[i + dim].blue + s[i - dim - 1].blue + s[i + dim - 1].blue) / 6;
}

static void set_bottom_val(pixel *s, pixel *d, int i, int dim)
{
    d[i].red = (s[i].red + s[i - 1].red + s[i + 1].red + s[i - dim].red + s[i - dim - 1].red + s[i - dim + 1].red) / 6;
    d[i].green = (s[i].green + s[i - 1].green + s[i + 1].green + s[i - dim].green + s[i - dim - 1].green + s[i - dim + 1].green) / 6;
    d[i].blue = (s[i].blue + s[i - 1].blue + s[i + 1].blue + s[i - dim].blue + s[i - dim - 1].blue + s[i - dim + 1].blue) / 6;
}

static void set_inner_val(pixel *s, pixel *d, int i, int dim)
{
    d[i].red = (s[i].red + s[i + 1].red + s[i + dim + 1].red + s[i + dim].red + s[i + dim - 1].red +
                s[i - 1].red + s[i - dim - 1].red + s[i - dim].red + s[i - dim + 1].red) / 9;
    d[i].green = (s[i].green + s[i + 1].green + s[i + dim + 1].green + s[i + dim].green + s[i + dim - 1].green +
                s[i - 1].green + s[i - dim - 1].green + s[i - dim].green + s[i - dim + 1].green) / 9;
    d[i].blue = (s[i].blue + s[i + 1].blue + s[i + dim + 1].blue + s[i + dim].blue + s[i + dim - 1].blue +
                s[i - 1].blue + s[i - dim - 1].blue + s[i - dim].blue + s[i - dim + 1].blue) / 9;
}

/******************************************************
 * Your different versions of the smooth kernel go here
 ******************************************************/

/*
 * naive_smooth - The naive baseline version of smooth
 */
char naive_smooth_descr[] = "naive_smooth: Naive baseline implementation";
void naive_smooth(int dim, pixel *src, pixel *dst)
{
    int i, j;

    for (i = 0; i < dim; i++)
    {
        for (j = 0; j < dim; j++)
        {
            dst[RIDX(i, j, dim)] = avg(dim, i, j, src);
        }
    }
}

/*
 * smooth - Your current working version of smooth.
 * IMPORTANT: This is the version you will be graded on
 */
char smooth_descr[] = "smooth: 使用特殊判断边界条件来优化max和min函数, 降低CPU的预测错误处罚";
void smooth(int dim, pixel *src, pixel *dst)
{
    int i, j;

    set_corner_val(src, dst, RIDX(0, 0, dim), RIDX(0, 1, dim), RIDX(1, 0, dim), RIDX(1, 1, dim));
    set_corner_val(src, dst, RIDX(0, dim - 1, dim), RIDX(0, dim - 2, dim), RIDX(1, dim - 2, dim), RIDX(1, dim - 1, dim));
    set_corner_val(src, dst, RIDX(dim - 1, 0, dim), RIDX(dim - 2, 0, dim), RIDX(dim - 2, 1, dim), RIDX(dim - 1, 1, dim));
    set_corner_val(src, dst, RIDX(dim - 1, dim - 1, dim), RIDX(dim - 1, dim - 2, dim), RIDX(dim - 2, dim - 1, dim), RIDX(dim - 2, dim - 2, dim));
    for (i = 1; i < dim - 1; i++)
    {
        set_top_val(src, dst, RIDX(0, i, dim), dim);
        set_bottom_val(src, dst, RIDX(dim - 1, i, dim), dim);
        set_left_val(src, dst, RIDX(i, 0, dim), dim);
        set_right_val(src, dst, RIDX(i, dim - 1, dim), dim);
    }
    for (i = 1; i < dim - 1; i++)
    {
        for (j = 1; j < dim - 1; j++)
        {
            set_inner_val(src, dst, RIDX(i, j, dim), dim);
        }
    }
}

/*********************************************************************
 * register_smooth_functions - Register all of your different versions
 *     of the smooth kernel with the driver by calling the
 *     add_smooth_function() for each test function.  When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.
 *********************************************************************/

void register_smooth_functions()
{
    add_smooth_function(&smooth, smooth_descr);
    add_smooth_function(&naive_smooth, naive_smooth_descr);
}
