#include <CL/opencl.h>
#include "opencl_wrap.h"
#include "cpu_ray.h"
#include "cpu_obj.h"


#define WIDTH 2000
#define HEIGHT 1000

int main() {
    rcamera camera = rinit_camera(
        (cl_float3){.x = 0.0f, .y = 1.0f, .z = -1.0f},
        (cl_float3){.x = 0.0f, .y = 0.0f, .z = 1.0f},
        120.0f, 1.0f
    );

    rray* rays = rgen_rays(&camera, WIDTH, HEIGHT);

    uint8_t sphere_num, plane_num, light_num;
    rsphere *ext_spheres;
    rplane  *ext_planes;
    rlight  *ext_lights;

    extract_robj("scenes/render.map", &ext_spheres, &sphere_num, &ext_planes, &plane_num,
                    &ext_lights, &light_num);

    cl_wrap cl;
    cl_wrap_init(&cl, CL_DEVICE_TYPE_GPU, "src/raytracing.cl");
    cl_uint total_size = sizeof(rray)*WIDTH*HEIGHT;

    cl_wrap_load_global_data(&cl, 0, rays, total_size, CL_MEM_READ_ONLY);
    cl_wrap_load_global_data(&cl, 1, NULL, total_size, CL_MEM_WRITE_ONLY);
    cl_wrap_load_global_data(&cl, 2, ext_spheres, sizeof(rsphere)*sphere_num, 
                             CL_MEM_READ_ONLY);
    cl_wrap_load_global_data(&cl, 3, ext_planes, sizeof(rplane)*plane_num, 
                             CL_MEM_READ_ONLY);
    cl_wrap_load_global_data(&cl, 4, ext_lights, sizeof(rlight)*light_num, 
                             CL_MEM_READ_ONLY);
    cl_wrap_load_single_data(&cl, 5, &sphere_num, sizeof(uint8_t));
    cl_wrap_load_single_data(&cl, 6, &plane_num, sizeof(uint8_t));
    cl_wrap_load_single_data(&cl, 7, &light_num, sizeof(uint8_t));
    cl_wrap_load_single_data(&cl, 8, &total_size, sizeof(cl_uint));
    rray* test = malloc(sizeof(rray)*WIDTH*HEIGHT);
    cl_wrap_output(&cl, WIDTH*HEIGHT, total_size, 1, test);
    cl_wrap_release(&cl);

    png_dump("out/scene.png", test, WIDTH, HEIGHT);

    free(ext_spheres);
    free(ext_planes);
    free(ext_lights);
    free(rays);
    return 0;
}