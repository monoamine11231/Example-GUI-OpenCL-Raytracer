#pragma once
#include <stdio.h>
#include <CL/opencl.h>

#include "cpu_ray.h"


#pragma scalar_storage_order little-endian
struct __rsphere {
    cl_float3   origin;
    cl_float    radius;

    cl_float3   rgb;
} __attribute__((packed));

struct __rplane {
    cl_float3   normal;
    cl_float3   point_in_plane;

    cl_float3   rgb;
} __attribute__((packed));

/* Light objects are spheres */
struct __rlight {
    cl_float3   origin;
    cl_float    radius;
    cl_float    intensity;

    cl_float3   rgb;   
} __attribute__((packed));
#pragma scalar_storage_order default

typedef struct __rsphere    rsphere;
typedef struct __rplane     rplane;
typedef struct __rlight     rlight;

/* Have written some archive protocol to store everything in the same file */
/* Dumps all the data to the same file */
/* Returns 0 on fail and 1 on success */
int dump_robj(const char* filename, rsphere* rspheres, uint8_t rsphere_num,
                rplane* rplanes, uint8_t rplane_num, rlight* rlights,
                uint8_t rlight_num);

/* Does the memory allocation automatically don't forget to free */
void extract_robj(const char* filename, rsphere** rspheres, uint8_t* rsphere_num,
                    rplane** rplanes, uint8_t* rplane_num, rlight** rlights,
                    uint8_t* rlight_num);
