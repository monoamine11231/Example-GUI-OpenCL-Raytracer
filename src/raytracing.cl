#include "src/types.cl"             /* All used types */
#include "src/primitives.cl"        /* Intersection functions */



#define MAX_DEPTH 5
/* To avoid self shadow, we add the normal * EPSILON to the intersection */
#define INVERSE_SQUARE_LIGHT 1.0f





__kernel void raytracer(__global rray* rays, __global rray* output,
                        __global rsphere* spheres,
                        __global rplane* planes, __global rlight* lights,
                        uchar spheres_num, uchar planes_num, uchar light_num,
                        uint total_size ) {

    uint id = get_global_id(0);
    if (id >= total_size) {
        return;
    }
    rray ray = rays[id];

    float f = 1.0f;
    
    while (ray.depth < MAX_DEPTH) {
        float3 intersection;
        float3 normal;
        rmaterial material;
        uint intersect = findIntersection(&ray, spheres, planes, lights, spheres_num,
                                          planes_num, light_num, &intersection, &normal,
                                          &material);

        if (!intersect) {   
            break;
        }

        if (intersect == 2) {
            break;
        }
        float3 new_dir = normalize(ray.dir - 2*dot(normal, ray.dir)*normal);
        output[id].rgb += f* material.rgb * material.ambient;

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
                                                    &light_material);

            float light_distance = distance(light.origin, intersection);
            float obj_distance   = distance(light_intersection, intersection);
            if (light_intersect != 0 && obj_distance < light_distance) {
                continue;
            }
            float d = light_distance;
            float3 light_rgb = light.rgb*light.intensity*INVERSE_SQUARE_LIGHT*1/(d*d);

            /* v points from the intersection to the ray origin */
            float3 v = normalize(ray.origin - intersection);
            /* h is the bisector of v and reflected ray */
            float3 h = normalize(v+shadow_ray.dir);

            /* Specular component */
            float3 spec_f = pow(max(0.0f, dot(normal, h)), (float)material.shininess);
            output[id].rgb += f*material.specular*light_rgb*spec_f; 

            /* Diffuse component */
            float3 diff_f = max(0.0f, dot(normal, shadow_ray.dir));
            output[id].rgb += f*material.diffuse*light_rgb*diff_f;
        }

        /* WHY DID I HAVE TO FIND MYSELF THAT THE DOT PRODUCT MUST BE SIGN FLIPPED */
        float fr = material.fresnel+(1-material.fresnel)*pow(-dot(normal,ray.dir),5.0f);

        /* Previously dark pixel on 800x600 */
        if (id == 305742) {   
        }
        f*= material.reflectivity+(1.0f-material.reflectivity)*fr;

        ray.dir = new_dir;
        ray.origin = intersection;
        ray.depth++;


    }

    output[id].rgb = clamp(output[id].rgb, 0.0f, 1.0f);
}
