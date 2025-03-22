/* mmult main.c
 *
 * Author: Ahmed Abdulaal
 * Date  : March 2025
 *
 * Description:
 * - Executes matrix-matrix multiplication (mmult) using different implementations.
 * - Profiles runtime using CLOCK_MONOTONIC.
 * - Verifies results against reference implementation.
 * - Supports CLI arguments for M, N, P dimensions, threads, and implementation.
 */

#define _GNU_SOURCE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

#include "impl/ref.h"
#include "impl/naive.h"
#include "impl/opt.h"
#include "impl/vec.h"
#include "impl/para.h"

#include "common/types.h"
#include "common/macros.h"
#include "include/types.h"

int main(int argc, char** argv) {
    setbuf(stdout, NULL);

    /* Arguments */
    int nthreads = 1, cpu = 0;
    int nruns = 100, nstdevs = 2;
    size_t M = 0, N = 0, P = 0;

    void* (*impl)(void* args) = NULL;
    const char* impl_str = NULL;

    /* Parse CLI args */
    for (int i = 1; i < argc; i++) {
      if ((strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--impl") == 0) && ++i < argc) {
            if      (strcmp(argv[i], "naive") == 0) { impl = impl_scalar_naive; impl_str = "naive"; }
            else if (strcmp(argv[i], "opt")   == 0) { impl = impl_scalar_opt;   impl_str = "opt"; }
            else if (strcmp(argv[i], "vec")   == 0) { impl = impl_vector;       impl_str = "vec"; }
            else if (strcmp(argv[i], "para")  == 0) { impl = impl_parallel;     impl_str = "para"; }
            else if (strcmp(argv[i], "ref")   == 0) { impl = impl_ref;          impl_str = "ref"; }
            else { impl = NULL; impl_str = "unknown"; }
        }
    
        else if (strcmp(argv[i], "--M") == 0 && ++i < argc) M = atoi(argv[i]);
        else if (strcmp(argv[i], "--N") == 0 && ++i < argc) N = atoi(argv[i]);
        else if (strcmp(argv[i], "--P") == 0 && ++i < argc) P = atoi(argv[i]);
        else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--nthreads") == 0) { ++i; nthreads = atoi(argv[i]); }
        else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--cpu") == 0) { ++i; cpu = atoi(argv[i]); }
        else if (strcmp(argv[i], "--nruns") == 0 && ++i < argc) nruns = atoi(argv[i]);
        else if (strcmp(argv[i], "--nstdevs") == 0 && ++i < argc) nstdevs = atoi(argv[i]);
      }

    if (!impl || !M || !N || !P) {
        printf("Usage: %s -i [impl] --M [rows] --N [shared] --P [cols]\n", argv[0]);
        return 1;
    }

    /* Set scheduling (same as template) */
#if !defined(__APPLE__)
    printf("Setting scheduler...\n");
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    sched_setscheduler(0, SCHED_FIFO, &param);
    cpu_set_t cpumask;
    CPU_ZERO(&cpumask);
    for (int i = 0; i < nthreads; ++i)
        CPU_SET(cpu + i, &cpumask);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpumask);
#endif

    __DECLARE_STATS(nruns, nstdevs);
    srand(0xDEADBEEF);

    size_t size_A = M * N;
    size_t size_B = N * P;
    size_t size_R = M * P;

    float* A     = __ALLOC_INIT_DATA(float, size_A);
    float* B     = __ALLOC_INIT_DATA(float, size_B);
    float* R_ref = __ALLOC_DATA(float, size_R + 4);
    float* R_out = __ALLOC_DATA(float, size_R + 4);

    __SET_GUARD(R_ref, size_R * sizeof(float));
    __SET_GUARD(R_out, size_R * sizeof(float));

    args_t args_ref = { A, B, R_ref, M, N, P, cpu, nthreads };
    args_t args     = { A, B, R_out, M, N, P, cpu, nthreads };

    /* Compute reference */
    impl_ref(&args_ref);

    /* Run implementation */
    printf("Running \"%s\"...\n", impl_str);
    for (int i = 0; i < nruns; ++i) {
        __SET_START_TIME();
        (*impl)(&args);
        __SET_END_TIME();
        runtimes[i] = __CALC_RUNTIME();
    }

    /* Check correctness */
    printf("Checking correctness...\n");
    bool match = __CHECK_FLOAT_MATCH(R_ref, R_out, size_R, 0.01f);
    bool guard = __CHECK_GUARD(R_out, size_R * sizeof(float));

    if (match && guard) printf("  ✔ Success!\n");
    else if (!match && guard) printf("  ✖ Wrong results.\n");
    else if (match && !guard) printf("  ⚠ Guard failed.\n");
    else printf("  ❌ Total failure.\n");

    /* Stats */
    uint64_t sum = 0, min = UINT64_MAX, max = 0;
    for (int i = 0; i < nruns; ++i) {
        if (runtimes[i] < min) min = runtimes[i];
        if (runtimes[i] > max) max = runtimes[i];
        sum += runtimes[i];
    }
    uint64_t avg = sum / nruns;

    printf("Runtime avg: %" PRIu64 " ns | min: %" PRIu64 " | max: %" PRIu64 "\n", avg, min, max);

    /* Dump runtimes */
    FILE* fp;
    char filename[128];
    snprintf(filename, sizeof(filename), "%s_runtimes.csv", impl_str);
    fp = fopen(filename, "w");
    if (fp) {
        fprintf(fp, "run,time_ns\n");
        for (int i = 0; i < nruns; ++i)
            fprintf(fp, "%d,%" PRIu64 "\n", i, runtimes[i]);
        fclose(fp);
        printf("Dumped runtimes to %s\n", filename);
    }

    /* Cleanup */
    __DESTROY_STATS();
    free(A); free(B); free(R_ref); free(R_out);

    return 0;
}
