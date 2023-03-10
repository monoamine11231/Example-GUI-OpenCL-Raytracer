#include <stdio.h>
#include <CL/opencl.h>
#include "cpu_obj.h"


int main() {
    /* Red sphere and blue sphere*/
    rsphere spheres[4];
    spheres[0].origin                   =   (cl_float3){.x = 4.5f, .y = 0.5f, .z = -1.0f};
    spheres[0].radius                   =   0.5f;
    spheres[0].material                 =   plastic;
    spheres[0].material.rgb             =   (cl_float3){.x = 1.0f, .y = 0.0f, .z = 0.0f};
    spheres[0].material.texture_id      =   -1;


    spheres[1].origin                   =   (cl_float3){.x = -1.0f,.y = 1.0f, .z = 4.5f};
    spheres[1].radius                   =   0.8f;
    spheres[1].material                 =   plastic;
    spheres[1].material.rgb             =   (cl_float3){.x = 0.0f, .y = 0.0f, .z = 1.0f};
    spheres[1].material.texture_id      =   -1;

    spheres[2].origin                   =   (cl_float3){.x = 0.8f, .y = 0.8f, .z = 1.5f};
    spheres[2].radius                   =   0.8f;
    spheres[2].material                 =   glass;
    spheres[2].material.texture_id      =   -1;

    spheres[3].origin                   =   (cl_float3){.x = -0.6f, .y = 0.8f, .z = -1.0f};
    spheres[3].radius                   =   0.8f;
    spheres[3].material                 =   glass;
    spheres[3].material.rgb             =   (cl_float3){.x = 0.0f, .y = 1.0f, .z = 0.0f};
    spheres[3].material.ambient         =   0.05f;
    spheres[3].material.texture_id      =   -1;


    rplane planes[2];
    planes[0].point_in_plane            =   (cl_float3){.x = 0.0f, .y = 0.0f, .z = 0.0f};
    planes[0].normal                    =   (cl_float3){.x = 0.0f, .y = 1.0f, .z = 0.0f};
    planes[0].material                  =   stone;
    planes[0].material.rgb              =   (cl_float3){.x = 0.0f, .y = 0.0f, .z = 0.0f};
    planes[0].material.texture_scale    =   100.0f;
    planes[0].material.texture_id       =   2;

    planes[1].point_in_plane            =   (cl_float3){.x = 0.0f, .y = 0.0f, .z = 7.0f};
    planes[1].normal                    =   (cl_float3){.x = 0.0f, .y = 0.0f,.z = -1.0f};
    planes[1].material                  =   mirror;
    planes[1].material.ambient          =   0.3;
    planes[1].material.shininess        =   150;
    planes[1].material.specular         =   0.4f;
    planes[1].material.rgb              =   (cl_float3){.x = 0.3f, .y = 0.3f, .z = 0.3f};
    planes[1].material.texture_id       =   -1;



    /* Light yellow light source*/
    rlight lights[3];
    lights[0].origin                    =   (cl_float3){.x = -2.0f, .y = 3.0f, .z = 2.0f};
    lights[0].intensity                 =   8.0f;
    lights[0].radius                    =   0.1f;
    lights[0].rgb                       =   (cl_float3){.x = 0.0f, .y = 1.0f, .z = 0.0f};

    lights[1].origin                    =   (cl_float3){.x = 2.0f, .y = 1.5f, .z = 0.2f};
    lights[1].intensity                 =   50.3f;
    lights[1].radius                    =   0.1f;
    lights[1].rgb                       =   (cl_float3){.x = 1.0f, .y = 1.0f, .z = 1.0f};

    lights[2].origin                    =   (cl_float3){.x = 1.0f, .y = 4.0f, .z = 3.0f};
    lights[2].intensity                 =   20.5f;
    lights[2].radius                    =   0.1f;
    lights[2].rgb                       =   (cl_float3){.x = 0.0f, .y = 0.0f, .z = 1.0f};

    int a = dump_robj("scenes/render.map", &spheres[0], 4, &planes[0], 2, &lights[0], 3);
    if (!a) {
        printf("Unable to create scene file\n");
    }

    return 0;
}