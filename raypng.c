#include <CL/opencl.h>
#include <sys/time.h>
#include "opencl_wrap.h"
#include "cpu_ray.h"
#include "cpu_obj.h"


#define WIDTH 800
#define HEIGHT 600

int main() {
    cl_uint pwidth  = WIDTH;
    cl_uint pheight = HEIGHT;

    struct timeval start, stop;

    rcamera camera = rinit_camera(
        (cl_float3){.x = 0.8f, .y = 2.5f, .z = -8.0f},
        (cl_float3){.x = 0.2f, .y = 0.0f, .z = 1.0f},
        90.0f, 1.0f
    );

    cl_uchar sphere_num, plane_num, light_num;
    rsphere *ext_spheres;
    rplane  *ext_planes;
    rlight  *ext_lights;

    extract_robj("scenes/render.map", &ext_spheres, &sphere_num, &ext_planes, &plane_num,
                    &ext_lights, &light_num);

    cl_wrap cl_wrap;
    cl_wrap_init(&cl_wrap, CL_DEVICE_TYPE_GPU,
                 "src/cl/raygen.cl", "raygen",
                 "src/cl/raytracing.cl", "raytracer", NULL);

    /* Camera perspective values for ray generation */
    cl_float3 im_corner, camera_origin, up, right;
    cl_float  w_factor, h_factor;
    rgen_perspective(&camera, &im_corner, &camera_origin, &up, &right,
                     &w_factor, &h_factor, WIDTH, HEIGHT);    


    cl_uint pixels = WIDTH*HEIGHT;
    cl_uint ray_size = sizeof(rray)*pixels;

    cl_uint buffer_size = pixels*sizeof(cl_uint);
    cl_uint *buffer = malloc(buffer_size);


    cl_wrap_load_single_data(&cl_wrap, 0, 0, &im_corner, sizeof(cl_float3));
    cl_wrap_load_single_data(&cl_wrap, 0, 1, &camera_origin, sizeof(cl_float3));
    cl_wrap_load_single_data(&cl_wrap, 0, 2, &up, sizeof(cl_float3));
    cl_wrap_load_single_data(&cl_wrap, 0, 3, &right, sizeof(cl_float3));
    cl_wrap_load_single_data(&cl_wrap, 0, 4, &w_factor, sizeof(cl_float));
    cl_wrap_load_single_data(&cl_wrap, 0, 5, &h_factor, sizeof(cl_float));
    cl_wrap_load_single_data(&cl_wrap, 0, 6, &pwidth, sizeof(cl_uint));
    cl_wrap_load_single_data(&cl_wrap, 0, 7, &pheight, sizeof(cl_uint));

    cl_wrap_load_global_data(&cl_wrap, 0, 8, NULL, ray_size, CL_MEM_READ_WRITE);
    
    cl_wrap_load_single_data(&cl_wrap, 1, 0, &cl_wrap.buffers[0][8], sizeof(cl_mem));
    cl_wrap_load_global_data(&cl_wrap, 1, 1, ext_spheres, sizeof(rsphere)*sphere_num,
                             CL_MEM_READ_ONLY);
    cl_wrap_load_global_data(&cl_wrap, 1, 2, ext_planes, sizeof(rplane)*plane_num,
                             CL_MEM_READ_ONLY);
    cl_wrap_load_global_data(&cl_wrap, 1, 3, ext_lights, sizeof(rlight)*light_num,
                             CL_MEM_READ_ONLY);
    cl_wrap_load_single_data(&cl_wrap, 1, 4, &sphere_num, sizeof(cl_uchar));
    cl_wrap_load_single_data(&cl_wrap, 1, 5, &plane_num, sizeof(cl_uchar));
    cl_wrap_load_single_data(&cl_wrap, 1, 6, &light_num, sizeof(cl_uchar));
    cl_wrap_load_single_data(&cl_wrap, 1, 7, &pixels, sizeof(cl_uint));


    cl_wrap_load_images(&cl_wrap, 1, 8,  CL_MEM_COPY_HOST_PTR, 4, 
                        "assets/cobblestone.png",
                        "assets/sand.png",
                        "assets/check.png",
                        "assets/grass.png");

    cl_wrap_load_images(&cl_wrap, 1, 9,  CL_MEM_COPY_HOST_PTR, 1, 
                        "assets/bg/stormydays.png");

    cl_wrap_load_global_data(&cl_wrap, 1, 10, NULL, buffer_size, CL_MEM_WRITE_ONLY);

    gettimeofday(&start, NULL);
    cl_wrap_output(&cl_wrap, WIDTH*HEIGHT, 0, 0, 0, 0, NULL);
    /* Because we do not copy the ray memory buffer from the first kernel to the second,
       we just say to read the ray buffer which is stored on the first kernel 8:th arg */
    cl_wrap_output(&cl_wrap, WIDTH*HEIGHT, buffer_size, 1, 1, 10, buffer);
    gettimeofday(&stop, NULL);

    long milli_time, seconds, useconds;
    seconds = stop.tv_sec - start.tv_sec; //seconds
    useconds = stop.tv_usec - start.tv_usec; //milliseconds
    milli_time = ((seconds) * 1000 + useconds/1000.0);
    printf("Done, took: %ld ms\n", milli_time);

    cl_wrap_release(&cl_wrap);

    png_dump("out/scene.png", buffer, WIDTH, HEIGHT);

    free(ext_spheres);
    free(ext_planes);
    free(ext_lights);
    free(buffer);
    return 0;
}