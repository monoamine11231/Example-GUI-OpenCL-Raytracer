#include <stdio.h>
#include "cpu_obj.h"


const rmaterial plastic = { .rgb            = (cl_float3){.x = 1.0f, .y = 1.0f, .z = 1.0f},

                            .ambient        = 0.2f,
                            .diffuse        = 0.3f,
                            .specular       = 0.4f,
                            .shininess      = 50,

                            .transperent    = CL_FALSE,
                            .dielectric     = CL_FALSE,
                            .n              = 1.4f,
                            .reflectivity   = 0.0f };

const rmaterial mirror = {  .rgb            = (cl_float3){.x = 0.2f,.y = 0.2f,.z = 0.2f},
                            .ambient        = 0.3f,
                            .diffuse        = 0.0f,
                            .specular       = 0.6f,
                            .shininess      = 100.0f,

                            .transperent    = CL_FALSE,
                            .dielectric     = CL_TRUE,
                            .n              = 1.0f,
                            .reflectivity   = 1.0f, };

const rmaterial glass = {   .rgb            = (cl_float3){.x = 0.0f,.y = 0.0f,.z = 0.0f},
                            .ambient        = 0.1f,
                            .diffuse        = 0.0f,
                            .specular       = 0.0f,
                            .shininess      = 20.0f,
                            
                            .transperent    = CL_TRUE,
                            .dielectric     = CL_TRUE,
                            .n              = 1.52f,
                            .reflectivity   = 0.04f, };

int dump_robj(const char* filename, rsphere* rspheres, uint8_t rsphere_num,
                rplane* rplanes, uint8_t rplane_num, rlight* rlights,
                uint8_t rlight_num) {

    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        return 0;
    }

    /* First byte is the number of elements which is followed by the raw data
       of the structs array. The order is rsphere, rplane, rlight*/
    fwrite(&rsphere_num, 1, 1, fp);
    fwrite(rspheres, sizeof(rsphere), rsphere_num, fp);

    fwrite(&rplane_num, 1, 1, fp);
    fwrite(rplanes, sizeof(rplane), rplane_num, fp);

    fwrite(&rlight_num, 1, 1, fp);
    fwrite(rlights, sizeof(rlight), rlight_num, fp);

    fclose(fp);

    return 1;
}

void extract_robj(const char* filename, rsphere** rspheres, uint8_t* rsphere_num,
                    rplane** rplanes, uint8_t* rplane_num, rlight** rlights,
                    uint8_t* rlight_num) {

    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        return;
    }

    /* First byte is the number of elements which is followed by the raw data
       of the structs array. The order is rsphere, rplane, rlight*/

    fread(rsphere_num, 1, 1, fp);
    *rspheres = malloc((*rsphere_num) * sizeof(rsphere));
    fread(*rspheres, sizeof(rsphere), *rsphere_num, fp);

    fread(rplane_num, 1, 1, fp);
    *rplanes = malloc((*rplane_num) * sizeof(rplane));
    fread(*rplanes, sizeof(rplane), *rplane_num, fp);

    fread(rlight_num, 1, 1, fp);
    *rlights = malloc((*rlight_num) * sizeof(rlight));
    fread(*rlights, sizeof(rlight), *rlight_num, fp);

    fclose(fp);
}