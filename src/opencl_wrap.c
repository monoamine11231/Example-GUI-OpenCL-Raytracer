#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>

#include <png.h>
#include "opencl_wrap.h"



void cl_wrap_init(cl_wrap* wrap, cl_device_type type, ...) {
    FILE*           source_reader;
    size_t          source_sizes[__MAX_KERNELS], log_size;
    char            *sources[__MAX_KERNELS], *log;
    const char      *current_source_file, *kernel_names[__MAX_KERNELS];
    cl_platform_id  platform;
    cl_int          cl_error;

    va_list         vars;


    /* No kernels when initializing */
    wrap->kernels_num = 0;


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
        printf("ERROR:\t Could not create a CL context from device\n");
        exit(1);
    }

    va_start(vars, type);
    current_source_file = va_arg(vars, const char*);
        
    while (current_source_file) {
        kernel_names[wrap->kernels_num] = va_arg(vars, const char*);
        if (!kernel_names[wrap->kernels_num]) {
            printf("ERROR:\tSource file was not followed by kernel name\n");
            va_end(vars);
            exit(1);
        }

        if ((source_reader = fopen(current_source_file, "r")) == NULL) {
            printf("ERROR:\tCannot open \"%s\" for reading\n", current_source_file);
            va_end(vars);
            exit(1);
        }

        /* No buffers when initializing */
        wrap->buffers_num[wrap->kernels_num] = 0;


        fseek(source_reader, 0, SEEK_END);
        source_sizes[wrap->kernels_num] = ftell(source_reader);
        rewind(source_reader);

        /* +1 for the null byte */
        sources[wrap->kernels_num] = malloc(source_sizes[wrap->kernels_num] + 1);
        /* Make the readin from the source file behave as a C string */
        sources[wrap->kernels_num][source_sizes[wrap->kernels_num]] = '\0';

        /* Read the kernel source file, close the file and continue for all kernels */
        fread(sources[wrap->kernels_num], 1, source_sizes[wrap->kernels_num],
              source_reader);
        fclose(source_reader);

        wrap->kernels_num++;
        current_source_file = va_arg(vars, const char*);
    }
    


    /* Firstly load the source to the compiler. We do explicit casting to `const char**`
       because the source codes are used once and are free'd after program build */
    wrap->program = clCreateProgramWithSource(wrap->context, wrap->kernels_num,
                                              (const char**)&sources[0],
                                              &source_sizes[0], &cl_error);
    if (cl_error < 0) {
        printf("ERROR:\tCouldn't load the source codes to the compiler\n");
        exit(1);
    }

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

    /* Free the sources and create the kernels at the same time */
    for (cl_uint i = 0; i < wrap->kernels_num; i++) {
        wrap->kernels[i] = clCreateKernel(wrap->program, kernel_names[i], &cl_error);
        if (cl_error < 0) {
            printf("ERROR:\tCouldn't create the CL kernel from: %s\n", kernel_names[i]);
            exit(1);
        }
        free(sources[i]);
    }

    /* Create the command queue */
    wrap->queue = clCreateCommandQueueWithProperties(wrap->context, wrap->device,
                                                     NULL, &cl_error);
    if (cl_error < 0) {
        printf("ERROR:\tCouldn't create a command queue for the given device\n");
        exit(1);
    }


    va_end(vars);
}

void cl_wrap_load_global_data(cl_wrap* wrap, cl_uint kernel_id, cl_uint arg_id, 
                              const void* data, size_t size, cl_mem_flags mem_flags) {

    cl_int      cl_error;


    /* Check if the kernel id is already in use */
    for (cl_uint i = 0; i < wrap->buffers_num[kernel_id]; i++) {
        if (wrap->buffers_ids[kernel_id][i] == arg_id) {
            printf("ERROR:\tGiven kernel argument already in use\n");
            exit(1);
        }
    }

    if (arg_id >= __MAX_BUFFERS) {
        printf("ERROR:\tWrong kernel ID given\n");
        exit(1);
    }

    /* Create the buffer and append it to the corresponding kernel */
    wrap->buffers[kernel_id][arg_id] = clCreateBuffer(wrap->context, mem_flags, size,
                                                      NULL, NULL);

    /* If data is not NULL, try to transfer the data from the host to the device */
    if (data) {
        cl_error = clEnqueueWriteBuffer(wrap->queue, wrap->buffers[kernel_id][arg_id],
                                        CL_TRUE, 0, size, data, 0, NULL, NULL);
        if (cl_error < 0) {
            printf("ERROR:\tCouldn't transfer the data from host to the device\n");
            exit(1);
        }
    }

    if (clSetKernelArg(wrap->kernels[kernel_id], arg_id, sizeof(cl_mem),
                       &wrap->buffers[kernel_id][arg_id]) < 0) {
        printf("ERROR:\tCouldn't pass the data argument to the kernel\n");
        exit(1);
    }

    /* Register the given kernel id as used */
    wrap->buffers_ids[kernel_id][wrap->buffers_num[kernel_id]++] = arg_id;
}

void cl_wrap_load_single_data(cl_wrap* wrap, cl_uint kernel_id, cl_uint arg_id,
                              const void* data, size_t obj_size) {

    /* Check if the kernel id is already in use */
    for (cl_uint i = 0; i < wrap->buffers_num[kernel_id]; i++) {
        if (wrap->buffers_ids[kernel_id][i] == arg_id) {
            printf("ERROR:\tGiven kernel argument already in use\n");
            exit(1);
        }
    }

    if (clSetKernelArg(wrap->kernels[kernel_id], arg_id, obj_size, data) < 0) {
        printf("ERROR:\tCouldn't pass the data argument to the kernel\n");
        exit(1);
    }
}

void cl_wrap_load_images(cl_wrap* wrap, cl_uint kernel_id, cl_uint arg_id,
                         cl_mem_flags mem_flags, cl_uint image_num, ...) {

    FILE            *ireader;
    cl_uchar        *images;
    cl_uint         image_count;

    va_list         vars;
    
    png_uint_32     prev_iwidth;
    png_uint_32     prev_iheight;

    cl_image_format iformat;
    cl_image_desc   idesc;

    cl_int          cl_error;


    image_count     = 0;

    prev_iwidth     = 0;
    prev_iheight    = 0;
    
    iformat         = (cl_image_format){CL_RGBA, CL_UNSIGNED_INT8};
    idesc           = (cl_image_desc){
                        CL_MEM_OBJECT_IMAGE2D_ARRAY,
                        0,
                        0,
                        0,
                        image_num,
                        0,
                        0,
                        0,
                        0,
                        NULL
                    };

    /* Check if the kernel id is already in use */
    for (cl_uint i = 0; i < wrap->buffers_num[kernel_id]; i++) {
        if (wrap->buffers_ids[kernel_id][i] == arg_id) {
            printf("ERROR:\tGiven kernel argument already in use\n");
            exit(1);
        }
    }

    va_start(vars, image_num);
    /* Read all given images and append the raw data into the same buffer */
    for (cl_uint i = 0; i < image_num; i++) {
        const char* filename = va_arg(vars, const char*);
        ireader = fopen(filename, "rb");
        if (!ireader) {
            printf("ERROR:\tCannot open file \"%s\"\n", filename);
            exit(1);
        }

        cl_uchar header[8];
        fread(&header[0], 1, 8, ireader);
        if (png_sig_cmp(&header[0], 0, 8)) {
            fclose(ireader);

            printf("ERROR:\t\"%s\" is not a PNG file\n", filename);
            exit(1);
        }

        png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL,
                                                     NULL);
        if (!png_ptr) {
            printf("ERROR:\tCould not create a PNG general file structure\n");
            exit(1);
        }
        png_infop png_info = png_create_info_struct(png_ptr);
        if (!png_info) {
            png_destroy_read_struct(&png_ptr, NULL, NULL);
            fclose(ireader);

            printf("ERROR:\tCould not create a PNG info file structure\n");
            exit(1);
        }

        png_init_io(png_ptr, ireader);
        png_set_sig_bytes(png_ptr, 8);
        png_read_info(png_ptr, png_info);

        png_uint_32     iwidth;
        png_uint_32     iheight;
        int bdepth, ctype;

        png_get_IHDR(png_ptr, png_info, &iwidth, &iheight, &bdepth, &ctype, NULL, NULL,
                     NULL);

        if (prev_iwidth == 0 && prev_iheight == 0) {
            prev_iwidth = iwidth;
            prev_iheight = iheight;

            /* RGBA */
            images = malloc(4*iwidth*iheight*image_num);

            idesc.image_width = iwidth;
            idesc.image_height = iheight;
            //idesc.image_slice_pitch = 4*iwidth*iheight;
        }

        if (prev_iwidth != iwidth || prev_iheight != iheight) {
            png_destroy_read_struct(&png_ptr, NULL, NULL);
            free(images);
            fclose(ireader);

            printf("ERROR:\tAll images must have same dimensions\n");
            exit(1);
        }

        if (bdepth != 8 || ctype != PNG_COLOR_TYPE_RGB) {
            png_destroy_read_struct(&png_ptr, NULL, NULL);
            free(images);
            fclose(ireader);

            printf("ERROR:\t\"%s\" must have a depth of 8 bits and be RGB\n", filename);
            exit(1);
        }

        png_set_filler(png_ptr, 255, PNG_FILLER_AFTER);

        /* Apply the transformations */
        png_read_update_info(png_ptr, png_info);
        
        size_t bytesrow = png_get_rowbytes(png_ptr, png_info);
        png_bytep row_pointers[iheight];
        /* Make the row points point in the right place in the raw image array buffer */
        for (png_uint_32 i = 0; i < iheight; i++) {
            row_pointers[i] = images+image_count*4*iwidth*iheight + i*bytesrow;
        }

        png_read_image(png_ptr, row_pointers);

        image_count++;

        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(ireader);
    }
    va_end(vars);

    wrap->buffers[kernel_id][arg_id] = clCreateImage(wrap->context, mem_flags,
                                                        &iformat, &idesc, images,
                                                        &cl_error);
    if (cl_error < 0) {
        free(images);
        printf("ERROR:\tCouldn't create an image array %d\n", cl_error);
        exit(1);
    }

    if (clSetKernelArg(wrap->kernels[kernel_id], arg_id, sizeof(cl_mem),
                       &wrap->buffers[kernel_id][arg_id]) < 0) {
        free(images);
        printf("ERROR:\tCouldn't pass the image array to the kernel\n");
        exit(1);
    }

    /* Register the given kernel id as used */
    wrap->buffers_ids[kernel_id][wrap->buffers_num[kernel_id]++] = arg_id;

    free(images);
}

void cl_wrap_output(cl_wrap* wrap, size_t array_size, size_t output_size,
                    cl_uint kernel_run_id, cl_uint kernel_id, cl_int arg_id,
                    void* host_output) {
    size_t global_size, local_size;
    cl_int cl_error;


    /* Get the suitable local size for the used device */
    cl_error = clGetKernelWorkGroupInfo(wrap->kernels[kernel_run_id], wrap->device,
                                        CL_KERNEL_WORK_GROUP_SIZE, sizeof(local_size),
                                        &local_size, NULL);
    if (cl_error < 0) {
        printf("ERROR:\tCannot get the work group size for the given device\n");
        exit(1);
    }

    /* Calculate the global size based on the local one */
    global_size = ceil(array_size/(float)local_size)*local_size;

    printf("global=%lu, local=%lu\n", global_size, local_size);


    cl_error = clEnqueueNDRangeKernel(wrap->queue, wrap->kernels[kernel_run_id], 1, NULL,
                                      &global_size, &local_size, 0, NULL, NULL);
    if (cl_error < 0) {
        printf("ERROR:\tCouldn't run the kernel\n");
        exit(1);
    }

    cl_error = clFinish(wrap->queue);
    if (cl_error < 0) {
        printf("%d\n", cl_error);
        printf("ERROR:\tThe device kernel failed\n");
        exit(1);
    }

    /* Ignoring memory copy from device to host if `host_output` is set to NULL */
    if (!host_output) { return; }

    /* Transfer the data from the device to the host otherwise */
    cl_error = clEnqueueReadBuffer(wrap->queue, wrap->buffers[kernel_id][arg_id],
                        CL_TRUE, 0, output_size, host_output, 0, NULL, NULL);

    if (cl_error < 0) {
        printf("ERROR:\tFailed to transfer device memory to host\n");
        exit(1);
    }
}

void cl_wrap_release(cl_wrap* wrap) {

    /* Release every kernel and its associated buffers */
    for (cl_uint kernel_id = 0; kernel_id < wrap->kernels_num; kernel_id++) {
        clReleaseKernel(wrap->kernels[kernel_id]);

        /* Release all the global buffers for the kernel */
        for (cl_uint i = 0; i < wrap->buffers_num[kernel_id]; i++) {
           clReleaseMemObject(wrap->buffers[kernel_id][wrap->buffers_ids[kernel_id][i]]);
        }
    }


    clReleaseCommandQueue(wrap->queue);
    clReleaseProgram(wrap->program);
    clReleaseContext(wrap->context);
}