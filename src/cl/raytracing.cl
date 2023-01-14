#include "src/cl/types.cl"             /* All used types */
#include "src/cl/primitives.cl"        /* Intersection functions */



/* N value for the air surrounding */
#define DEFAULT_N 1.0f

#define MAX_DEPTH 10
/* To avoid self shadow, we add the normal * EPSILON to the intersection */
#define INVERSE_SQUARE_LIGHT 1.0f





__kernel void raytracer(__global rray* rays,
                        __global rsphere* spheres,
                        __global rplane* planes, __global rlight* lights,
                        uchar spheres_num, uchar planes_num, uchar light_num,
                        uint total_size, read_only image2d_array_t im_arr ) {

    uint id = get_global_id(0);
    if (id >= total_size) {
        return;
    }

    rray    ray_stack[MAX_DEPTH];
    float   n_stack[MAX_DEPTH];
    float   f_stack[MAX_DEPTH];

    uint    stack_size = 1;

    ray_stack[0]    = rays[id];
    n_stack[0]      = DEFAULT_N;
    f_stack[0]      = 1.0f;
    
    while (stack_size > 0) {
        while (ray_stack[stack_size - 1].depth < MAX_DEPTH) {
            float3 intersection;
            float3 normal;
            rmaterial material;
            uint intersect = findIntersection(&ray_stack[stack_size-1], spheres, planes, 
                                            lights, spheres_num, planes_num, light_num,
                                            &intersection, &normal, &material, im_arr);

            if (!intersect) {   
                break;
            }

            ray_stack[stack_size-1].rgb += f_stack[stack_size-1]*\
                                                material.rgb * material.ambient;


                if (id == 3457 && material.reflectivity == 0.002f) {
                    PRINT_VEC(ray_stack[stack_size-1].rgb);
                }

            /* Calculate direct illumination on non light objects */
            for(uchar i = 0; i < light_num; i++) {
                rlight light = lights[i];

                rray shadow_ray;
                shadow_ray.origin = intersection;
                shadow_ray.dir = normalize(light.origin-intersection);
                shadow_ray.rgb = (float3){0.0f, 0.0f, 0.0f};

                float3 light_intersection, light_normal;
                rmaterial light_material;

                uint light_intersect = findIntersection(&shadow_ray, spheres, planes,
                                                        lights, spheres_num, planes_num,
                                                        light_num, &light_intersection,
                                                        &light_normal,
                                                        &light_material,
                                                        im_arr);

                float light_distance = distance(light.origin, intersection);
                float obj_distance   = distance(light_intersection, intersection);
                if (light_intersect != 0 && obj_distance < light_distance) {
                    continue;
                }
                float d = light_distance;
                float3 light_rgb =light.rgb*light.intensity*INVERSE_SQUARE_LIGHT*1/(d*d);

                /* v points from the intersection to the ray origin */
                float3 v = normalize(ray_stack[stack_size-1].origin - intersection);
                /* h is the bisector of v and reflected ray */
                float3 h = normalize(v+shadow_ray.dir);

                /* Specular component */
                float3 spec_f = pow(max(0.0f, dot(normal, h)),(float)material.shininess);
                ray_stack[stack_size-1].rgb += f_stack[stack_size-1]*\
                                                    material.specular*light_rgb*spec_f; 

                /* Diffuse component */
                float3 diff_f = max(0.0f, dot(normal, shadow_ray.dir));
                ray_stack[stack_size-1].rgb += f_stack[stack_size-1]*\
                                                    material.diffuse*light_rgb*diff_f;
            }

                if (id == 3457 && material.reflectivity == 0.002f) {
                    PRINT_VEC(ray_stack[stack_size-1].rgb);
                    printf("\n");
                }

            /* Save the incident ray before it gets updated */
            float3 incident = ray_stack[stack_size-1].dir;

            float n1 = n_stack[stack_size-1];
            float n2 = material.n;
            
            n2 = (n1 == DEFAULT_N) ? n2 : DEFAULT_N;

            float fr = compute_schlick(n1, n2, &incident, &normal);
            float reflect_amount=material.reflectivity+(1.0f-material.reflectivity)*fr;
            
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
