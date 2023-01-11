#include "src/cl/types.cl"
#include "src/cl/primitives.cl"


__kernel void raygen(float3 image_lt_corner, float3 camera_origin,
                     float3 up, float3 right,
                     float w_factor, float h_factor,
                     uint pwidth, uint pheight, __global rray* rays) {

    uint id = get_global_id(0);
    if (id >= pwidth*pheight) { return; }

    float w = (float)(id % pwidth);
    float h = (float)(id / pwidth);

    float3 vec = image_lt_corner+right*w_factor*w-up*h_factor*h;

    //printf("%f\n", h);

    rays[id].depth = 0;
    rays[id].dir = normalize(vec);
    //PRINT_VEC(rays[id].dir);
    rays[id].origin = camera_origin;
    rays[id].rgb = (float3){0.0f, 0.0f, 0.0f};
}
