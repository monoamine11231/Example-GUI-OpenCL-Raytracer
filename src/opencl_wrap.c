#include <stdio.h>
#include <math.h>
#include "opencl_wrap.h"


void cl_wrap_init(cl_wrap* wrap, cl_device_type type, const char* filename) {
    FILE*           source_reader;
    size_t          source_size, log_size;
    char            *source, *log;
    cl_platform_id  platform;
    cl_int          cl_error;


    /* No kernel arguments when initializing */
    wrap->args_num = 0;

    if (clGetPlatformIDs(1, &platform, NULL) < 0) {
        printf("ERROR:\tCannot find a CL platform\n");
        exit(1);
    }

    if (clGetDeviceIDs(platform, type, 1, &wrap->device, NULL) == CL_DEVICE_NOT_FOUND) {
        printf("ERROR:\tCannot find a device of the given type\n");
        exit(1);
    }

    wrap->context = clCreateContext(NULL, 1, &wrap->device, NULL, NULL, &cl_error);
    if (cl_error < 0) {
        printf("ERROR:\t Could not create a CL context from the device\n");
        exit(1);
    }

    
    if ((source_reader = fopen(filename, "r")) == NULL) {
        printf("ERROR:\tCannot open the source file for reading\n");
        exit(1);
    }

    fseek(source_reader, 0, SEEK_END);
    source_size = ftell(source_reader);
    rewind(source_reader);

    source = malloc(source_size + 1);
    /* Make the readin from the source file behave as a C string */
    source[source_size] = '\0';
    fread(source, 1, source_size, source_reader);
    fclose(source_reader);

    /* Firstly load the source to the compiler */
    wrap->program = clCreateProgramWithSource(wrap->context, 1, (const char**)&source,
                                              &source_size, &cl_error);
    if (cl_error < 0) {
        printf("ERROR:\tCouldn't load the source code to the compiler\n");
        exit(1);
    }

    free(source);

    if (clBuildProgram(wrap->program, 0, NULL, NULL, NULL, NULL) < 0) {
        /* Try to get the log from the compiler and output it */
        clGetProgramBuildInfo(wrap->program, wrap->device, CL_PROGRAM_BUILD_LOG, 0,
                              NULL, &log_size);
        log = malloc(log_size + 1);
        log[log_size] = '\0';
        clGetProgramBuildInfo(wrap->program, wrap->device, CL_PROGRAM_BUILD_LOG,
                              log_size + 1, log, NULL);
        printf("ERROR:\tCouldn't compile the source. Compiler LOG is shown below:\n");
        printf("%s\n", log);
        free(log);
        exit(1);
    }

    wrap->queue = clCreateCommandQueueWithProperties(wrap->context, wrap->device,
                                                     NULL, &cl_error);
    if (cl_error < 0) {
        printf("ERROR:\tCouldn't create a command queue for the given device\n");
        exit(1);
    }

    wrap->kernel = clCreateKernel(wrap->program, "raytracer", &cl_error);
    if (cl_error < 0) {
        printf("ERROR:\tCouldn't create a CL kernel\n");
        exit(1);
    }
}

void cl_wrap_load_global_data(cl_wrap* wrap, cl_uint arg_id, const void* data, 
                              size_t size, cl_mem_flags mem_flags) {

    cl_int      cl_error;


    /* Check if the kernel id is already in use */
    for (cl_uint i = 0; i < wrap->args_num; i++) {
        if (wrap->used_args[i] == arg_id) {
            printf("ERROR:\tGiven kernel argument already in use\n");
            exit(1);
        }
    }

    if (arg_id >= 32) {
        printf("ERROR:\tWrong kernel ID given\n");
        exit(1);
    }

    wrap->buffers[arg_id] = clCreateBuffer(wrap->context, mem_flags, size, NULL, NULL);
    /* If data is not NULL, try to transfer the data from the host to the device */
    if (data) {
        cl_error = clEnqueueWriteBuffer(wrap->queue, wrap->buffers[arg_id], CL_TRUE, 0,
                                        size, data, 0, NULL, NULL);
        if (cl_error < 0) {
            printf("ERROR:\tCouldn't transfer the data from host to the device\n");
            exit(1);
        }
    }

    if (clSetKernelArg(wrap->kernel,arg_id,sizeof(cl_mem),&wrap->buffers[arg_id]) < 0) {
        printf("ERROR:\tCouldn't pass the data argument to the kernel\n");
        exit(1);
    }

    /* Register the given kernel id as used */
    wrap->used_args[wrap->args_num++] = arg_id;
}

void cl_wrap_load_single_data(cl_wrap* wrap, cl_uint arg_id, const void* data,
                              size_t obj_size) {

    /* Check if the kernel id is already in use */
    for (cl_uint i = 0; i < wrap->args_num; i++) {
        if (wrap->used_args[i] == arg_id) {
            printf("ERROR:\tGiven kernel argument already in use\n");
            exit(1);
        }
    }

    if (clSetKernelArg(wrap->kernel, arg_id, obj_size, data) < 0) {
        printf("ERROR:\tCouldn't pass the data argument to the kernel\n");
        exit(1);
    }
}

void cl_wrap_output(cl_wrap* wrap, size_t array_size, size_t output_size, cl_int arg_id,
                    void* host_output) {
    size_t global_size, local_size;
    cl_int cl_error;

    cl_error = clGetKernelWorkGroupInfo(wrap->kernel, wrap->device,
                                        CL_KERNEL_WORK_GROUP_SIZE, sizeof(local_size),
                                        &local_size, NULL);

    if (cl_error < 0) {
        printf("ERROR:\tCannot get the work group size for the given device\n");
        exit(1);
    }
    /* Calculate the global size based on the local one */
    global_size = ceil(array_size/(float)local_size)*local_size;

    printf("global=%lu, local=%lu\n", global_size, local_size);

    cl_error = clEnqueueNDRangeKernel(wrap->queue, wrap->kernel, 1, NULL, &global_size,
                                      &local_size, 0, NULL, NULL);
    if (cl_error < 0) {
        printf("ERROR:\tCouldn't run the kernel\n");
        exit(1);
    }

    cl_error = clFinish(wrap->queue);
    if (cl_error < 0) {
        printf("ERROR:\tThe device kernel failed\n");
        exit(1);
    }

    /* Transfer the data from the device to the host */
    cl_error = clEnqueueReadBuffer(wrap->queue, wrap->buffers[arg_id], CL_TRUE, 0, output_size,
                        host_output, 0, NULL, NULL);

    if (cl_error < 0) {
        printf("ERROR:\tFailed to transfer device memory to host\n");
        exit(1);
    }
}

void cl_wrap_release(cl_wrap* wrap) {
    clReleaseKernel(wrap->kernel);

    /* Release all the global buffers */
    for (cl_uint i = 0; i < wrap->args_num; i++) {
        clReleaseMemObject(wrap->buffers[wrap->used_args[i]]);
    }

    clReleaseCommandQueue(wrap->queue);
    clReleaseProgram(wrap->program);
    clReleaseContext(wrap->context);
}