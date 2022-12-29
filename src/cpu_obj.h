#pragma once
#include <stdio.h>
#include <CL/opencl.h>

#include "cpu_ray.h"


#pragma scalar_storage_order little-endian
#pragma pack(push, 1)
struct __rmaterial {
    cl_float3           rgb;

    cl_float            ambient;
    cl_float            diffuse;
    cl_float            specular;
    cl_uint             shininess;
    
    cl_bool             transperent;
    cl_float            fresnel;
    cl_float            reflectivity;
};

struct __rsphere {
    cl_float3           origin;
    cl_float            radius;

    struct __rmaterial  material;
};

struct __rplane {
    cl_float3           normal;
    cl_float3           point_in_plane;

    struct __rmaterial  material;
};

/* Light objects are spheres */
struct __rlight {
    cl_float3           origin;
    cl_float            radius;
    cl_float            intensity;

    cl_float3           rgb;   
};
#pragma pack(pop)
#pragma scalar_storage_order default

typedef struct __rmaterial  rmaterial;
typedef struct __rsphere    rsphere;
typedef struct __rplane     rplane;
typedef struct __rlight     rlight;

extern const rmaterial      plastic;
extern const rmaterial      mirror;


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
