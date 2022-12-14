#include <math.h>
#include <float.h>
#include <stdbool.h>
#include "cpu_ray.h"


static cl_float3 normalize(cl_float3 vec) {
    float number = vec.x*vec.x+vec.y*vec.y+vec.z*vec.z;
    /* Quake III fast reversed square root algorithm */
    long i;
    float x2, y;
    const float thrhalfs = 1.5f;

    x2 = number * 0.5f;
    y = number;
    i = *(long*)&y;
    i = 0x5f3759df - (i >> 1);
    y = *(float*)&i;
    y = y * ( thrhalfs - ( x2 * y * y ) );

    cl_float3 res = vec;
    res.x *= y;
    res.y *= y;
    res.z *= y;

    return res;
}

static inline bool equal(cl_float3 a, cl_float3 b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

rcamera rinit_camera(cl_float3 camera_origin, cl_float3 camera_lookdir,
                     cl_float fov, cl_float focal_length) {
    rcamera camera;
    camera.pos_dir = (rray){.origin = camera_origin,
                            .dir = normalize(camera_lookdir),
                            .rgb = (cl_float3){.x=0.0f, .y=0.0f,.z=0.0f},
                            .depth = 0};
    camera.fov = fov;
    camera.focal_length = focal_length;

    return camera;
}

void rlookat(rcamera *camera, cl_float3 dir) {
    camera->pos_dir.dir = normalize(dir);
}

rray* rgen_rays(rcamera* camera, cl_int pwidth, cl_int pheight) {
    rray *rays;
    cl_float3 tmp_top, forward, left, up, image_center, image_lt_corner;
    cl_float half_radians_fov, aspect_ratio, fov_tan, image_width, image_height,
            w_factor, h_factor;
    bool is_180;


    rays                = malloc(sizeof(rray) * pwidth * pheight);
    tmp_top             = (cl_float3){.x = 0.0f, .y = 1.0f, .z = 0.0f};
    
    /* For comparison with floats and by taking in account the floating point */
    is_180              = camera->fov-180.0f <= FLT_EPSILON && camera->fov-180.0f >= 0;

    /* Leave if non-acceptable fov or if tmp top vector equals the lookat pos */
    if (is_180 || camera->fov <= FLT_EPSILON || equal(tmp_top, camera->pos_dir.dir)) {
        return NULL;
    }

    half_radians_fov    = (camera->fov / 360.0f) * M_PI;
    aspect_ratio        = (cl_float)pheight / (cl_float)pwidth;
    fov_tan             = tan(half_radians_fov);

    image_width         = fov_tan * camera->focal_length * 2;
    image_height        = aspect_ratio * image_width;

    w_factor            = image_width / pwidth;
    h_factor            = image_height/ pheight;

    /* It is normalized */
    forward             = camera->pos_dir.dir;

    /* Manual cross-product to construct the orthogonal unit vectors needed */
    left               = (cl_float3){
                            .x = tmp_top.y*forward.z-tmp_top.z*forward.y,
                            .y = tmp_top.z*forward.x-tmp_top.x*forward.z,
                            .z = tmp_top.x*forward.y-tmp_top.y*forward.x
                        };
    up                  = (cl_float3){
                            .x = forward.y*left.z-forward.z*left.y,
                            .y = forward.z*left.x-forward.x*left.z,
                            .z = forward.x*left.y-forward.y*left.x
                        };


    image_center        = (cl_float3){
                            .x = forward.x*camera->focal_length-camera->pos_dir.origin.x,
                            .y = forward.y*camera->focal_length-camera->pos_dir.origin.y,
                            .z = forward.z*camera->focal_length-camera->pos_dir.origin.z
                        };
    /* Vector pointing to the left top corner of the image */
    image_lt_corner     = (cl_float3){
                            .x = image_center.x+left.x*image_width/2+up.x*image_height/2,
                            .y = image_center.y+left.y*image_width/2+up.y*image_height/2,
                            .z = image_center.z+left.z*image_width/2+up.z*image_height/2
                        };

    /* For every pixel cast a ray given the vector pointing to the pixel */
    for (int w = 0; w < pwidth; w++) {
        for (int h = 0; h < pheight; h++) {
            rray ray;

            cl_float3 vec;
            vec.x = image_lt_corner.x-left.x*w_factor*w-up.x*h_factor*h;
            vec.y = image_lt_corner.y-left.y*w_factor*w-up.y*h_factor*h;
            vec.z = image_lt_corner.z-left.z*w_factor*w-up.z*h_factor*h;

            ray.depth = 0;
            ray.dir = normalize(vec);
            ray.origin = camera->pos_dir.origin;
            /* Black background on the scene */
            ray.rgb = (cl_float3){.x = 0.0f, .y = 0.0f, .z = 0.0f};

            /* Add the constructed ray to the linear list */
            rays[h*pwidth+w] = ray;
        }
    }

    return rays;
}