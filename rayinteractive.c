#include <math.h>
#include <sys/time.h>
#include <CL/opencl.h>
#include <MiniFB.h>

#include "opencl_wrap.h"
#include "cpu_ray.h"
#include "cpu_obj.h"


#define TITLE "Interactive Raytracer"

#define WIDTH 800
#define HEIGHT 600

#define CAMERA_SPEED 0.05f;
#define MOVE_SPEED 0.1f


rcamera camera;
float X_ROT = M_PI_2;
float Y_ROT = M_PI_2;

/* Camera perspective values for ray generation */
cl_float3 im_corner, camera_origin, up, right;
cl_float  w_factor, h_factor;

cl_wrap wrap;



void camera_control(struct mfb_window *window, mfb_key key, mfb_key_mod mod,
                    bool isPressed) {

    switch (key)
    {
    case KB_KEY_UP:
        X_ROT -= CAMERA_SPEED;
        break;
    case KB_KEY_DOWN:
        X_ROT += CAMERA_SPEED;
        break;
    case KB_KEY_LEFT:
        Y_ROT -= CAMERA_SPEED;
        break;
    case KB_KEY_RIGHT:
        Y_ROT += CAMERA_SPEED;
        break;

    case KB_KEY_W:
        camera.pos_dir.origin.x += MOVE_SPEED*camera.pos_dir.dir.x;
        camera.pos_dir.origin.y += MOVE_SPEED*camera.pos_dir.dir.y;
        camera.pos_dir.origin.z += MOVE_SPEED*camera.pos_dir.dir.z;
        break;
    case KB_KEY_S:
        camera.pos_dir.origin.x -= MOVE_SPEED*camera.pos_dir.dir.x;
        camera.pos_dir.origin.y -= MOVE_SPEED*camera.pos_dir.dir.y;
        camera.pos_dir.origin.z -= MOVE_SPEED*camera.pos_dir.dir.z;
        break;
    case KB_KEY_A:
        camera.pos_dir.origin.x -= MOVE_SPEED*right.x;
        camera.pos_dir.origin.y -= MOVE_SPEED*right.y;
        camera.pos_dir.origin.z -= MOVE_SPEED*right.z;
        break;
    case KB_KEY_D:
        camera.pos_dir.origin.x += MOVE_SPEED*right.x;
        camera.pos_dir.origin.y += MOVE_SPEED*right.y;
        camera.pos_dir.origin.z += MOVE_SPEED*right.z;
        break;
    case KB_KEY_SPACE:
        camera.pos_dir.origin.x += MOVE_SPEED*up.x;
        camera.pos_dir.origin.y += MOVE_SPEED*up.y;
        camera.pos_dir.origin.z += MOVE_SPEED*up.z;
        break;
    case KB_KEY_LEFT_SHIFT:
        camera.pos_dir.origin.x -= MOVE_SPEED*up.x;
        camera.pos_dir.origin.y -= MOVE_SPEED*up.y;
        camera.pos_dir.origin.z -= MOVE_SPEED*up.z;
        break;

    default:
        break;
    }

    cl_float3 new_dir;
    // We replace z and y, because we use y as vertical axis
    new_dir.x = sin(X_ROT)*cos(Y_ROT);
    new_dir.z = sin(X_ROT)*sin(Y_ROT);
    new_dir.y = cos(X_ROT);
    
    /* Normalized already so do not use rlookat */
    camera.pos_dir.dir = new_dir;

    rgen_perspective(&camera, &im_corner, &camera_origin, &up, &right,
                     &w_factor, &h_factor, WIDTH, HEIGHT);

    /* Load the new generated perspective values*/
    cl_wrap_load_single_data(&wrap, 0, 0, &im_corner, sizeof(cl_float3));
    cl_wrap_load_single_data(&wrap, 0, 1, &camera_origin, sizeof(cl_float3));
    cl_wrap_load_single_data(&wrap, 0, 2, &up, sizeof(cl_float3));
    cl_wrap_load_single_data(&wrap, 0, 3, &right, sizeof(cl_float3));
    cl_wrap_load_single_data(&wrap, 0, 4, &w_factor, sizeof(cl_float));
    cl_wrap_load_single_data(&wrap, 0, 5, &h_factor, sizeof(cl_float));
}

int main() {
    cl_uint pwidth  = WIDTH;
    cl_uint pheight = HEIGHT;

    /* Initialize the default camera looking into +Z*/
    camera = rinit_camera(
        (cl_float3){.x = 0.8f, .y = 2.5f, .z = -8.0f},
        (cl_float3){.x = 0.0f, .y = 0.0f, .z = 1.0f},
        90.0f, 1.0f
    );

    /* Create the window */
    struct mfb_window *window = mfb_open_ex(TITLE, WIDTH, HEIGHT, 0);
    if (!window)
        return 0;

    mfb_set_keyboard_callback(window, camera_control);

    cl_uchar sphere_num, plane_num, light_num;
    rsphere *ext_spheres;
    rplane  *ext_planes;
    rlight  *ext_lights;

    extract_robj("scenes/render.map", &ext_spheres, &sphere_num, &ext_planes, &plane_num,
                    &ext_lights, &light_num);

    cl_wrap_init(&wrap, CL_DEVICE_TYPE_GPU,
                 "src/cl/raygen.cl", "raygen",
                 "src/cl/raytracing.cl", "raytracer", NULL);


    rgen_perspective(&camera, &im_corner, &camera_origin, &up, &right,
                     &w_factor, &h_factor, WIDTH, HEIGHT);    


    cl_uint pixels = WIDTH*HEIGHT;
    cl_uint ray_size = sizeof(rray)*pixels;

    cl_uint buffer_size = pixels*sizeof(cl_uint);
    cl_uint *buffer = malloc(buffer_size);

    cl_wrap_load_single_data(&wrap, 0, 0, &im_corner, sizeof(cl_float3));
    cl_wrap_load_single_data(&wrap, 0, 1, &camera_origin, sizeof(cl_float3));
    cl_wrap_load_single_data(&wrap, 0, 2, &up, sizeof(cl_float3));
    cl_wrap_load_single_data(&wrap, 0, 3, &right, sizeof(cl_float3));
    cl_wrap_load_single_data(&wrap, 0, 4, &w_factor, sizeof(cl_float));
    cl_wrap_load_single_data(&wrap, 0, 5, &h_factor, sizeof(cl_float));
    cl_wrap_load_single_data(&wrap, 0, 6, &pwidth, sizeof(cl_uint));
    cl_wrap_load_single_data(&wrap, 0, 7, &pheight, sizeof(cl_uint));

    cl_wrap_load_global_data(&wrap, 0, 8, NULL, ray_size, CL_MEM_READ_WRITE);
    
    cl_wrap_load_single_data(&wrap, 1, 0, &wrap.buffers[0][8], sizeof(cl_mem));
    cl_wrap_load_global_data(&wrap, 1, 1, ext_spheres, sizeof(rsphere)*sphere_num,
                             CL_MEM_READ_ONLY);
    cl_wrap_load_global_data(&wrap, 1, 2, ext_planes, sizeof(rplane)*plane_num,
                             CL_MEM_READ_ONLY);
    cl_wrap_load_global_data(&wrap, 1, 3, ext_lights, sizeof(rlight)*light_num,
                             CL_MEM_READ_ONLY);
    cl_wrap_load_single_data(&wrap, 1, 4, &sphere_num, sizeof(cl_uchar));
    cl_wrap_load_single_data(&wrap, 1, 5, &plane_num, sizeof(cl_uchar));
    cl_wrap_load_single_data(&wrap, 1, 6, &light_num, sizeof(cl_uchar));
    cl_wrap_load_single_data(&wrap, 1, 7, &pixels, sizeof(cl_uint));

    cl_wrap_load_images(&wrap, 1, 8,  CL_MEM_COPY_HOST_PTR, 4, 
                        "assets/cobblestone.png",
                        "assets/sand.png",
                        "assets/check.png",
                        "assets/grass.png");

    cl_wrap_load_images(&wrap, 1, 9,  CL_MEM_COPY_HOST_PTR, 1, 
                        "assets/bg/stormydays.png");

    cl_wrap_load_global_data(&wrap, 1, 10, NULL, buffer_size, CL_MEM_WRITE_ONLY);

    struct mfb_timer* timer = mfb_timer_create();

    while (mfb_wait_sync(window)) {
        int state;

        cl_wrap_output(&wrap, WIDTH*HEIGHT, 0, 0, 0, 0, NULL);
        /* Because we do not copy the ray memory buffer from the first kernel to the second,
        we just say to read the ray buffer which is stored on the first kernel 8:th arg */
        cl_wrap_output(&wrap, WIDTH*HEIGHT, buffer_size, 1, 1, 10, buffer);

        state = mfb_update_ex(window, buffer, WIDTH, HEIGHT);

        if (state < 0) {
            window = NULL;
            break;
        }
    }

    mfb_timer_destroy(timer);

    /* Release the OpenCL program */
    cl_wrap_release(&wrap);

    free(ext_spheres);
    free(ext_planes);
    free(ext_lights);
    free(buffer);
    return 0;
}