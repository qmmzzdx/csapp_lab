/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 *
 * Cache: s = 5, E = 1, b = 5
 */
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;
    int a, b, c, d, e, f, g, h;

    if (M == 32 && N == 32)
    {
        int k;
        for (i = 0; i < N; i += 8)
        {
            for (j = 0; j < M; j += 8)
            {
                for (k = i; k < i + 8; k++)
                {
                    a = A[k][j], b = A[k][j + 1], c = A[k][j + 2], d = A[k][j + 3];
                    e = A[k][j + 4], f = A[k][j + 5], g = A[k][j + 6], h = A[k][j + 7];
                    B[j][k] = a, B[j + 1][k] = b, B[j + 2][k] = c, B[j + 3][k] = d;
                    B[j + 4][k] = e, B[j + 5][k] = f, B[j + 6][k] = g, B[j + 7][k] = h;
                }
            }
        }
        return;
    }
    if (M == 64 && N == 64)
    {
        int k;
        for (i = 0; i < N; i += 8)
        {
            for (j = 0; j < M; j += 8)
            {
                for (k = i; k < i + 4; k++)
                {
                    a = A[k][j], b = A[k][j + 1], c = A[k][j + 2], d = A[k][j + 3];
                    e = A[k][j + 4], f = A[k][j + 5], g = A[k][j + 6], h = A[k][j + 7];
                    B[j][k] = a, B[j + 1][k] = b, B[j + 2][k] = c, B[j + 3][k] = d;
                    B[j][k + 4] = e, B[j + 1][k + 4] = f, B[j + 2][k + 4] = g, B[j + 3][k + 4] = h;
                }
                for (k = j; k < j + 4; k++)
                {
                    a = B[k][i + 4], b = B[k][i + 5], c = B[k][i + 6], d = B[k][i + 7];
                    e = A[i + 4][k], f = A[i + 5][k], g = A[i + 6][k], h = A[i + 7][k];
                    B[k][i + 4] = e, B[k][i + 5] = f, B[k][i + 6] = g, B[k][i + 7] = h;
                    B[k + 4][i] = a, B[k + 4][i + 1] = b, B[k + 4][i + 2] = c, B[k + 4][i + 3] = d;
                }
                for (k = j + 4; k < j + 8; k++)
                {
                    a = A[i + 4][k], b = A[i + 5][k], c = A[i + 6][k], d = A[i + 7][k];
                    B[k][i + 4] = a, B[k][i + 5] = b, B[k][i + 6] = c, B[k][i + 7] = d;
                }
            }
        }
        return;
    }
    int n = N / 8 * 8;
    int m = M / 8 * 8;
    for (j = 0; j < m; j += 8)
    {
        for (i = 0; i < n; i++)
        {
            a = A[i][j], b = A[i][j + 1], c = A[i][j + 2], d = A[i][j + 3];
            e = A[i][j + 4], f = A[i][j + 5], g = A[i][j + 6], h = A[i][j + 7];
            B[j][i] = a, B[j + 1][i] = b, B[j + 2][i] = c, B[j + 3][i] = d;
            B[j + 4][i] = e, B[j + 5][i] = f, B[j + 6][i] = g, B[j + 7][i] = h;
        }
    }
    for (i = n; i < N; i++)
    {
        for (j = m; j < M; j++)
        {
            a = A[i][j];
            B[j][i] = a;
        }
    }
    for (i = 0; i < N; i++)
    {
        for (j = m; j < M; j++)
        {
            a = A[i][j];
            B[j][i] = a;
        }
    }
    for (i = n; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            a = A[i][j];
            B[j][i] = a;
        }
    }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; ++j)
        {
            if (A[i][j] != B[j][i])
            {
                return 0;
            }
        }
    }
    return 1;
}
