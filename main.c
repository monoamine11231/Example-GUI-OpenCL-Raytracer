#include <CL/opencl.h>
#include "cpu_ray.h"
#include "cpu_obj.h"


int main() {
    rcamera camera = rinit_camera(
        (cl_float3){.x = 0.0f, .y = 1.0f, .z = 0.0f},
        (cl_float3){.x = 0.0f, .y = 0.0f, .z = 1.0f},
        90.0f, 1.0f
    );

    rray* rays = rgen_rays(&camera, 800, 600);

    uint8_t sphere_num, plane_num, light_num;
    rsphere *ext_spheres;
    rplane  *ext_planes;
    rlight  *ext_lights;

    extract_robj("scenes/render.map", &ext_spheres, &sphere_num, &ext_planes, &plane_num,
                    &ext_lights, &light_num);
    
    png_dump("out/scene.png", rays, 800, 600);

    free(ext_spheres);
    free(ext_planes);
    free(ext_lights);
    free(rays);
    return 0;
}