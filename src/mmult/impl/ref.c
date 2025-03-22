/* ref.c
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

/* Reference Implementation */
void* impl_ref(void* void_args) {
    args_t* args = (args_t*) void_args;
    size_t M = args->M;
    size_t N = args->N;
    size_t P = args->P;

    float* A = args->A;
    float* B = args->B;
    float* R = args->R;

    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < P; ++j) {
            float sum = 0.0f;
            for (size_t k = 0; k < N; ++k) {
                sum += A[i * N + k] * B[k * P + j];
            }
            R[i * P + j] = sum;
        }
    }

    return NULL;
}


