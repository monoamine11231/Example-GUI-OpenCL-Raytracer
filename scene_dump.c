#include <stdio.h>
#include <CL/opencl.h>
#include "cpu_obj.h"


int main() {
    /* Red sphere and blue sphere*/
    rsphere spheres[2];
    spheres[0].origin       =   (cl_float3){.x = 1.5f, .y = 1.0f, .z = 3.0f};
    spheres[0].radius       =   1.0f;
    spheres[0].rgb          =   (cl_float3){.x = 1.0f, .y = 0.0f, .z = 0.0f};

    spheres[1].origin       =   (cl_float3){.x = -3.0f, .y = 2.0f, .z = 4.5f};
    spheres[1].radius       =   2.0f;
    spheres[1].rgb          =   (cl_float3){.x = 0.0f, .y = 0.0f, .z = 1.0f};

    rplane planes[2];
    planes[0].point_in_plane=   (cl_float3){.x = 0.0f, .y = 0.0f, .z = 0.0f};
    planes[0].normal        =   (cl_float3){.x = 0.0f, .y = 1.0f, .z = 0.0f};
    planes[0].rgb           =   (cl_float3){.x = 0.5f, .y = 0.5f, .z = 0.5f};

    planes[1].point_in_plane=   (cl_float3){.x = 0.0f, .y = 0.0f, .z = 7.0f};
    planes[1].normal        =   (cl_float3){.x = 0.0f, .y = 0.0f, .z = -1.0f};
    planes[1].rgb           =   (cl_float3){.x = 1.0f, .y = 1.0f, .z = 1.0f};

    /* Light yellow light source*/
    rlight light1;
    light1.origin           =   (cl_float3){.x = 1.0f, .y = 2.5f, .z = 5.5f};
    light1.intensity        =   1.0f;
    light1.radius           =   0.3f;
    light1.rgb              =   (cl_float3){.x = 1.0f, .y = 1.0f, .z = 0.4f};

    int a = dump_robj("scenes/render.map", &spheres[0], 2, &planes[0], 2, &light1, 1);
    if (!a) {
        printf("Unable to create scene file\n");
    }

    return 0;
}