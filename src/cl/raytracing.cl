#include "src/cl/types.cl"             /* All used types */
#include "src/cl/primitives.cl"        /* Intersection functions */



/* N value for the air surrounding */
#define DEFAULT_N 1.0f

#define MAX_DEPTH 10
#define MAX_SOFT_SHADOWS 5



__kernel void raytracer(__global rray* rays,
                        __global rsphere* spheres,
                        __global rplane* planes, __global rlight* lights,
                        uchar spheres_num, uchar planes_num, uchar light_num,
                        uint total_size,
                        read_only image2d_array_t im_arr,
                        read_only image2d_array_t skybox) {

    uint id = get_global_id(0);
    if (id >= total_size) {
        return;
    }

    rray    ray_stack[MAX_DEPTH];
    float   n_stack[MAX_DEPTH];
    float   f_stack[MAX_DEPTH];

    xorshift32_state rand_state;
    rand_state.x = id;

    uint    stack_size = 1;

    ray_stack[0]    = rays[id];
    n_stack[0]      = DEFAULT_N;
    f_stack[0]      = 1.0f;
    
    while (stack_size > 0) {
        while (ray_stack[stack_size - 1].depth < MAX_DEPTH) {
            float3 intersection;
            float3 normal;
            rmaterial material;

            float3 light_color;
            if (findLightIntersection(&ray_stack[stack_size - 1],lights, light_num, &light_color)) {
                ray_stack[stack_size - 1].rgb += f_stack[stack_size-1]*light_color;
                break;
            }

            uint intersect = findSolidIntersection(&ray_stack[stack_size-1], spheres, planes, 
                                            spheres_num, planes_num,
                                            &intersection, &normal, &material, im_arr);

            /* Sample skybox texture if no intersection */
            if (!intersect) {
                int2 uv = map_to_cube(&ray_stack[stack_size-1].dir,
                                      get_image_dim(skybox).x/4);


                int4 pixel_fetch = (int4) {
                                    uv.x, get_image_dim(skybox).y-uv.y,
                                    0, 0};

                int4    pixeli = read_imagei(skybox, pixel_fetch);
                /* Cast to normalized float manually */
                float3  pixelf = (float3){
                    (float)pixeli.x/255.0f,
                    (float)pixeli.y/255.0f,
                    (float)pixeli.z/255.0f
                };

                ray_stack[stack_size-1].rgb += f_stack[stack_size-1]*pixelf;
                break;
                
            }

            ray_stack[stack_size-1].rgb += f_stack[stack_size-1]*\
                                                material.rgb * material.ambient;

            /* Calculate direct illumination on non light objects */
            for(uchar i = 0; i < light_num; i++) {
                rlight light = lights[i];

                /* Amount of soft shadows not blocked by objects */
                uint soft_shadows = 0;

                /* Main soft shadow through light center point */
                float3 shadow_dir = normalize(light.origin-intersection);

                for (uint j = 0; j < MAX_SOFT_SHADOWS; j++) {
                    /* Use xorshift pseudorandom generator to sample on the light
                       object's sphere (VERY SLOW) */
                    float theta = 2*M_PI*xorshift32(&rand_state);
                    float phi = M_PI*xorshift32(&rand_state);

                    float x = light.radius*sin(phi)*cos(theta);
                    float y = light.radius*sin(phi)*sin(theta);
                    float z = light.radius*cos(phi);

                    float3 sample = light.origin+(float3){x,y,z};

                    soft_shadows += !testShadowPath(&sample, &intersection, 
                                            spheres, planes, spheres_num, planes_num);
                }

                /* Soft shadow ratio */
                float ssr = soft_shadows/(float)MAX_SOFT_SHADOWS;

                float d = distance(light.origin, intersection);

                
                float3 light_rgb =light.rgb*light.intensity*INVERSE_SQUARE_LIGHT*1/(d*d);
                /* Apply the soft shadow ratio */
                light_rgb*=ssr;

                /* v points from the intersection to the ray origin */
                float3 v = normalize(ray_stack[stack_size-1].origin - intersection);
                /* h is the bisector of v and reflected ray */
                float3 h = normalize(v+shadow_dir);

                /* Specular component */
                float3 spec_f = pow(max(0.0f, dot(normal, h)),(float)material.shininess);
                ray_stack[stack_size-1].rgb += f_stack[stack_size-1]*\
                                                    material.specular*light_rgb*spec_f; 

                /* Diffuse component */
                float3 diff_f = max(0.0f, dot(normal, shadow_dir));
                ray_stack[stack_size-1].rgb += f_stack[stack_size-1]*\
                                                    material.diffuse*light_rgb*diff_f;
            }

            /* Save the incident ray before it gets updated */
            float3 incident = ray_stack[stack_size-1].dir;

            float n1 = n_stack[stack_size-1];
            float n2 = material.n;
            
            n2 = (n1 == DEFAULT_N) ? n2 : DEFAULT_N;

            float reflect_amount = material.reflectivity;
            if (material.dielectric) {
                float fr = compute_schlick(n1, n2, &incident, &normal);
                reflect_amount=material.reflectivity+(1.0f-material.reflectivity)*fr;
            }

            
            float old_f = f_stack[stack_size-1];
            f_stack[stack_size-1]*= reflect_amount;

            ray_stack[stack_size-1].dir = reflect(&ray_stack[stack_size-1].dir, &normal);

            ray_stack[stack_size-1].origin = intersection;
            ray_stack[stack_size-1].depth++;

            if (material.transperent && stack_size < MAX_DEPTH && reflect_amount < 1.0f) {
                
                ray_stack[stack_size]   = ray_stack[stack_size - 1];
                if (n1 < n2) {
                    ray_stack[stack_size].origin -= 2*EPSILON*normal;
                } else {
                    normal *= -1;
                }
                f_stack[stack_size]     = old_f*(1.0f-reflect_amount);
                ray_stack[stack_size].rgb = (float3){0.0f, 0.0f, 0.0f};

                n_stack[stack_size]     = n2;
                ray_stack[stack_size].dir = refract(n1, n2, &incident, &normal);
                if (isnan(ray_stack[stack_size].dir.x)) { continue; }



                stack_size++;
            }
        }

        /* Only one in stack - raytracing completed for this ray */
        if (stack_size == 1) { break; }

        /* If previous rays in stack - just add the current ray rgb and decrement
           the stack size */
    
        ray_stack[stack_size - 2].rgb += ray_stack[stack_size - 1].rgb;

        stack_size--;
    }

    rays[id].rgb = clamp(ray_stack[0].rgb, 0.0f, 1.0f);
}
