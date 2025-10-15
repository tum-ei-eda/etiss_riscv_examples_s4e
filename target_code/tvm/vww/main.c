#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "vww_data/vww_input_data.h"
#include "vww_data/vww_model_settings.h"
#include "vww_data/vww_output_data_ref.h"
#include "tvm/runtime/c_runtime_api.h"
#include "tvm/runtime/crt/error_codes.h"
#include "tvmgen_default.h"

#ifndef CHECK
#define CHECK 1
#endif

#ifndef MAX_RUNS
#define MAX_RUNS vww_data_sample_cnt
#endif

#ifndef MIN_RUNS
#define MIN_RUNS 1
#endif

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) < (Y)) ? (Y) : (X))


void TVMLogf(const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vfprintf(stdout, msg, args);
    va_end(args);
}

tvm_crt_error_t TVMPlatformMemoryAllocate(size_t num_bytes, DLDevice dev, void **out_ptr)
{
    return kTvmErrorFunctionCallNotImplemented;
}

tvm_crt_error_t TVMPlatformMemoryFree(void *ptr, DLDevice dev) { return kTvmErrorFunctionCallNotImplemented; }

void __attribute__((noreturn)) TVMPlatformAbort(tvm_crt_error_t error_code)
{
    printf("TVMPlatformAbort: %d\n", error_code);
    exit(-1);
}

TVM_DLL int TVMFuncRegisterGlobal(const char *name, TVMFunctionHandle f, int override) { return 0; }

int run_test()
{
    for (size_t i = 0; i < MIN(vww_data_sample_cnt, MAX_RUNS); i++)
    {
        struct tvmgen_default_inputs tvmgen_default_inputs = {(int8_t *)vww_input_data[i % vww_data_sample_cnt]};
        int8_t output_data[256] = {0}; // TODO(fabianpedd): Make this precise by using defines for the array sizes
        struct tvmgen_default_outputs tvmgen_default_outputs = {output_data};

        int ret_val = tvmgen_default_run(&tvmgen_default_inputs, &tvmgen_default_outputs);
        if (ret_val)
        {
            TVMPlatformAbort(kTvmErrorPlatformCheckFailure);
        }

#if CHECK
        int8_t top_index = 0;
        for (size_t j = 0; j < vww_model_label_cnt; j++)
        {
            if (output_data[j] > output_data[top_index])
            {
                top_index = j;
            }
        }

        if (top_index != vww_output_data_ref[i % vww_data_sample_cnt])
        {
            printf("ERROR: at #%d, top_index %d vww_output_data_ref %d \n", i, top_index, vww_output_data_ref[i % vww_data_sample_cnt]);
            return -1;
        }
        else
        {
            printf("Sample #%d pass, top_index %d matches ref %d \n", i, top_index, vww_output_data_ref[i % vww_data_sample_cnt]);
        }
#endif
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int ret = run_test();
    if (ret != 0)
    {
        printf("Test Failed!\n");
        // Make sure RISC-V simulators detect a failed test
#if defined(__riscv) || defined(__riscv__)
        // __asm__ volatile("unimp");
#endif
    }
    else
    {
#if CHECK
        printf("Test Success!\n");
#endif
    }

    return ret;
}
