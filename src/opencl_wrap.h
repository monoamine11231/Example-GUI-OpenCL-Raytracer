#pragma once

#include <CL/opencl.h>


#define __MAX_KERNELS           16
#define __MAX_BUFFERS           32

/*All functions for the opencl wrapper handles error checking and terminates the program*/

typedef struct {
    cl_context          context;
    cl_device_id        device;
    cl_program          program;
    cl_command_queue    queue;

    cl_uint             kernels_num;
    cl_kernel           kernels[__MAX_KERNELS];

    cl_mem              buffers[__MAX_KERNELS][__MAX_BUFFERS];
    cl_uint             buffers_ids[__MAX_KERNELS][__MAX_BUFFERS];
    cl_uint             buffers_num[__MAX_KERNELS];
}   cl_wrap;


/* Sets the device and builds the program from source.
   Every source code is followed by kernel name and terminated by NULL, for example:
   "src/raygen.cl", "raygen", "src/raytracing.cl", "raytracer", NULL */
void cl_wrap_init(cl_wrap* wrap, cl_device_type type, ...);
/* If `data` is NULL, then no data is transfered, only a cl buffer is created.
It sets the data buffer to the kernel after transfering*/
void cl_wrap_load_global_data(cl_wrap* wrap, cl_uint kernel_id, cl_uint arg_id,
                              const void* data, size_t size, cl_mem_flags mem_flags);
void cl_wrap_load_single_data(cl_wrap* wrap, cl_uint kernel_id, cl_uint arg_id,
                              const void* data, size_t obj_size);
void cl_wrap_load_images(cl_wrap* wrap, cl_uint kernel_id, cl_uint arg_id,
                         cl_mem_flags mem_flags, cl_uint image_num, ...);
/* Runs the kernel and outputs the result to host */
void cl_wrap_output(cl_wrap* wrap, size_t array_size, size_t output_size, 
                    cl_uint kernel_run_id, cl_uint kernel_id, cl_int arg_id,
                    void* host_output);
void cl_wrap_release(cl_wrap* wrap);