/* naive.c
 *
 * Author:
 * Date  :
 *
 *  Description
 */

/* Standard C includes */
#include <stdlib.h>

/* Include common headers */
#include "common/macros.h"
#include "common/types.h"

/* Include application-specific headers */
#include "include/types.h"

/* Naive Implementation */
void* impl_scalar_naive(void* void_args)
{
    args_t* args = (args_t*) void_args;
    float* A = args->A;
    float* B = args->B;
    float* R = args->R;
    int M = args->M, N = args->N, P = args->P;

    for (int i = 0; i < M; i++) {
        for (int j = 0; j < P; j++) {
            float sum = 0.0f;
            for (int k = 0; k < N; k++) {
                sum += A[i * N + k] * B[k * P + j];
            }
            R[i * P + j] = sum;
        }
    }
    return NULL;
}
#pragma GCC pop_options
