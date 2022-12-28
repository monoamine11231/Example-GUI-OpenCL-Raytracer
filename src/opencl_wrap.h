#pragma once

#include <CL/opencl.h>


/*All functions for the opencl wrapper handles error checking and terminates the program*/

typedef struct {
    cl_uint             used_args[32];
    cl_uint             args_num;
    /* Max 32 global cl buffers*/
    cl_mem              buffers[32];

    cl_context          context;
    cl_device_id        device;
    cl_program          program;
    cl_kernel           kernel;
    cl_command_queue    queue;
}   cl_wrap;


/* Sets the device and builds the program from source */
void cl_wrap_init(cl_wrap* wrap, cl_device_type type, const char* filename);
/* If `data` is NULL, then no data is transfered, only a cl buffer is created.
It sets the data buffer to the kernel after transfering*/
void cl_wrap_load_global_data(cl_wrap* wrap, cl_uint arg_id, const void* data,
                              size_t size, cl_mem_flags mem_flags);
void cl_wrap_load_single_data(cl_wrap* wrap, cl_uint arg_id, const void* data,
                              size_t obj_size);
/* Runs the kernel and outputs the result to host */
void cl_wrap_output(cl_wrap* wrap, size_t array_size, size_t output_size, cl_int arg_id,
                    void* host_output);
void cl_wrap_release(cl_wrap* wrap);