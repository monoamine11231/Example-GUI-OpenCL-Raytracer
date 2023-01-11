#pragma once
#include <stdbool.h>

#include <CL/opencl.h>

#pragma pack(push, 16)
struct __rray {
    cl_float3   origin;
    cl_float3   dir;
    
    cl_float3   rgb;

    cl_int      depth;
};
#pragma pack(pop)

typedef struct __rray rray;

struct __rcamera {
    rray        pos_dir;

    cl_float    fov;
    cl_float    focal_length;
};

typedef struct __rcamera rcamera;


rcamera     rinit_camera(cl_float3 camera_origin, cl_float3 camera_lookdir,
                         cl_float fov, cl_float focal_length);
void        rlookat(rcamera *camera, cl_float3 dir);

/* Genereates the perspective values based on the camera location and dir that
   are essential to generate the rays on the GPU side */
bool        rgen_perspective(rcamera* camera, cl_float3* im_corner,
                             cl_float3* camera_origin,
                             cl_float3* up, cl_float3* right,
                             cl_float* w_factor, cl_float* h_factor,
                             cl_uint pwidth, cl_uint pheight);

int         png_dump(const char* filename, rray* rays, cl_int pwidth, cl_int pheight);