#include <math.h>
#include <float.h>

#include <png.h>
#include "cpu_ray.h"


static cl_float3 normalize(cl_float3 vec) {
    float number = 1/sqrt(vec.x*vec.x+vec.y*vec.y+vec.z*vec.z);


    cl_float3 res;
    res.x = vec.x*number;
    res.y = vec.y*number;
    res.z = vec.z*number;
    
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


bool rgen_perspective(rcamera* camera, cl_float3* im_corner,
                      cl_float3* camera_origin,
                      cl_float3* up, cl_float3* right,
                      cl_float* w_factor, cl_float* h_factor,
                      cl_uint pwidth, cl_uint pheight) {

    cl_float3 tmp_top, forward, image_center;
    cl_float half_radians_fov, aspect_ratio, fov_tan, image_width, image_height;
    bool is_180;


    *camera_origin = camera->pos_dir.origin;

    tmp_top             = (cl_float3){.x = 0.0f, .y = 1.0f, .z = 0.0f};
    
    /* For comparison with floats and by taking in account the floating point */
    is_180              = camera->fov-180.0f <= FLT_EPSILON && camera->fov-180.0f >= 0;

    /* Leave if non-acceptable fov or if tmp top vector equals the lookat pos */
    if (is_180 || camera->fov <= FLT_EPSILON || equal(tmp_top, camera->pos_dir.dir)) {
        return false;
    }

    half_radians_fov    = (camera->fov / 360.0f) * M_PI;
    aspect_ratio        = (cl_float)pheight / (cl_float)pwidth;
    fov_tan             = tan(half_radians_fov);

    image_width         = fov_tan * camera->focal_length * 2;
    image_height        = aspect_ratio * image_width;

    *w_factor           = image_width / pwidth;
    *h_factor           = image_height / pheight;

    /* It is normalized */
    forward             = camera->pos_dir.dir;
    forward.x           *= -1.0f;
    forward.y           *= -1.0f;
    forward.z           *= -1.0f;

    /* Manual cross-product to construct the orthogonal unit vectors needed */
    *right              = (cl_float3){
                        .x = tmp_top.y*forward.z-tmp_top.z*forward.y,
                        .y = tmp_top.z*forward.x-tmp_top.x*forward.z,
                        .z = tmp_top.x*forward.y-tmp_top.y*forward.x
                        };
    *up                 = (cl_float3){
                        .x = forward.y*right->z-forward.z*right->y,
                        .y = forward.z*right->x-forward.x*right->z,
                        .z = forward.x*right->y-forward.y*right->x
                        };


    image_center        = (cl_float3){
                        .x = -forward.x*camera->focal_length,
                        .y = -forward.y*camera->focal_length,
                        .z = -forward.z*camera->focal_length
                        };

    /* Vector pointing to the left top corner of the image */
    *im_corner          = (cl_float3){
                        .x = image_center.x-right->x*image_width/2+up->x*image_height/2,
                        .y = image_center.y-right->y*image_width/2+up->y*image_height/2,
                        .z = image_center.z-right->z*image_width/2+up->z*image_height/2
                        };
}

int png_dump(const char* filename, cl_uint* buffer, cl_int pwidth, cl_int pheight) {
    FILE* fp;
    png_structp png_ptr     = NULL;
    png_infop info_ptr      = NULL;
    png_byte **row_pointers = NULL;

    fp = fopen(filename, "wb");
    if (!fp) {
        return 0;
    }

    png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        return 0;
    }

    info_ptr = png_create_info_struct (png_ptr);
    if (!info_ptr) {
        return 0;
    }

    /* PNG information header, set rgb mode and pixel depth of 8 bits */
    png_set_IHDR(png_ptr,
                 info_ptr,
                 pwidth,
                 pheight,
                 8,
                 PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    /* Dump the rays rgb values as bytes into the png buffer */
    row_pointers = png_malloc (png_ptr, pheight * sizeof (png_byte *));
    for (int r = 0; r < pheight; r++) {
        png_byte* row = png_malloc(png_ptr, pwidth * 3);
        row_pointers[r] = row;

        for (int c = 0; c < pwidth; c++) {
            *row++ = (uint8_t)((buffer[r*pwidth+c] >> 16) & 0xFF);
            *row++ = (uint8_t)((buffer[r*pwidth+c] >> 8) & 0xFF);
            *row++ = (uint8_t)(buffer[r*pwidth+c] & 0xFF);
        }
    }

    png_init_io(png_ptr, fp);
    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    for (int r = 0; r < pheight; r++) {
        png_free(png_ptr, row_pointers[r]);
    }

    png_free(png_ptr, row_pointers);

    fclose(fp);
    return 1;
}