#include <CL/opencl.h>
#include "opencl_wrap.h"
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
    
    // const int ARRAY_SIZE = 2048;
    // cl_float arr[ARRAY_SIZE];
    // for (int i = 0; i < ARRAY_SIZE; i++) {
    //     arr[i] = 1.0f*i;
    // }

    // cl_float out[ARRAY_SIZE];

    // cl_wrap cl;
    // cl_wrap_init(&cl, CL_DEVICE_TYPE_GPU, "test.cl");
    // cl_wrap_load_global_data(&cl, 0, &arr[0], ARRAY_SIZE*sizeof(cl_float), CL_MEM_READ_ONLY);
    // cl_wrap_load_global_data(&cl, 1, NULL, ARRAY_SIZE*sizeof(cl_float), CL_MEM_WRITE_ONLY);
    // cl_wrap_load_single_data(&cl, 2, &ARRAY_SIZE, sizeof(const int));
    // cl_wrap_output(&cl, ARRAY_SIZE*sizeof(cl_float), ARRAY_SIZE*sizeof(cl_float), 1, &out[0]);
    // cl_wrap_release(&cl);

    // for (int i = 0; i < ARRAY_SIZE; i++) {
    //     printf("%f\n", out[i]);
    // }

    // png_dump("out/scene.png", rays, 800, 600);

    free(ext_spheres);
    free(ext_planes);
    free(ext_lights);
    free(rays);
    return 0;
}