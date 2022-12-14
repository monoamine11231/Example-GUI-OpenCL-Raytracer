#pragma once
#include <CL/opencl.h>


struct __rray {
    cl_float3   origin;
    cl_float3   dir;
    
    cl_float3   rgb;

    cl_int      depth;
} __attribute__((packed));

typedef struct __rray rray;

typedef struct {
    rray        pos_dir;

    cl_float    fov;
    cl_float    focal_length;
} rcamera;


rcamera     rinit_camera(cl_float3 camera_origin, cl_float3 camera_lookdir,
                         cl_float fov, cl_float focal_length);
void        rlookat(rcamera *camera, cl_float3 dir);

rray*       rgen_rays(rcamera* camera, cl_int pwidth, cl_int pheight);

int         png_dump(const char* filename, rray* rays, cl_int pwidth, cl_int pheight);