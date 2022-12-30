struct __rmaterial {
    float3              rgb;

    float               ambient;
    float               diffuse;
    float               specular;
    uint                shininess;
    
    uint                transperent;
    float               fresnel;
    float               reflectivity;
} __attribute__((packed));

struct __rsphere {
    float3              origin;
    float               radius;

    struct __rmaterial  material;
} __attribute__((packed));

struct __rplane {
    float3              normal;
    float3              point_in_plane;

    struct __rmaterial  material;
} __attribute__((packed));

/* Light objects are spheres */
struct __rlight {
    float3              origin;
    float               radius;
    float               intensity;

    float3              rgb;   
} __attribute__((packed));

typedef struct __rmaterial  rmaterial;
typedef struct __rsphere    rsphere;
typedef struct __rplane     rplane;
typedef struct __rlight     rlight;


struct __rray {
    float3   origin;
    float3   dir;
    
    float3   rgb;

    int      depth;
} __attribute__((packed));

typedef struct __rray rray;


#define MAX_DEPTH 5
/* To avoid self shadow, we add the normal * EPSILON to the intersection */
#define EPSILON 0.1f
#define INVERSE_SQUARE_LIGHT 1.0f

bool intersect_sphere(rray *ray, float3* sphere_origin, float sphere_radius, float* t) {

    float3  v = ray->origin-(*sphere_origin);
    float   a = dot(ray->dir, ray->dir);
    float   b = dot(2*v, ray->dir);
    float   c = dot(v,v)-sphere_radius*sphere_radius;

    float   t2;

    float   D = b*b-4*a*c;

    /* No intersection */
    if (D < 0) {
        return false;
    }
    D = sqrt(D);

    /* If the closest intersection is behind the camera, replace it with the
        farthest */
    t2 = ((-b-D)/(2*a) < 0) ? (-b+D)/(2*a) : (-b-D)/(2*a);
    if (t2 <= 0) {
        return false;
    }
    *t = t2;
    return true;
}

bool intersect_plane(rray *ray, float3* plane_normal, float3* point_in_plane, float* t) {
    float       t2;
    float       b;

    b = dot(ray->dir, *plane_normal);
    
    /* No intersection */
    if (b == 0) {
        return false;
    }

    
    t2 = dot(((*point_in_plane)-ray->origin), *plane_normal)/b;
    if (t2 <= 0) {
        return false;
    }
    *t = t2;
    return true;
}


/*  RETURN 0: NO INTERSECTION
    RETURN 1: INTERSECTION SOLID OBJECT
    RETURN 2: INTERSECTION LIGHT OBJECT */
uint findIntersection(rray *ray,
                      __global rsphere* spheres, __global rplane* planes,
                      __global rlight* lights, uchar spheres_num, uchar planes_num,
                      uchar light_num, float3* intersection, 
                      float3* normal, rmaterial* material) {
                        
    rmaterial           transfer_material;

    bool hit_light          = false;
    /* Takes account the intensity */
    float3 light_color;

    bool did_intersect      = false;

    float t                 = INFINITY;
    float3 target_normal;
    float3 interpoint;

    /* Find closest intersection with spheres */
    for (uchar i = 0; i < spheres_num; i++) {
        rsphere sphere = spheres[i];
        float _t;
        bool _intersect = intersect_sphere(ray, &sphere.origin, sphere.radius, &_t);
        if (!_intersect || _t >= t) {
            continue;
        }

        t = _t;
        interpoint = ray->origin+ray->dir*t;
        target_normal = normalize(interpoint-sphere.origin);
        /* To avoid self shadow */
        interpoint += target_normal*EPSILON;

        transfer_material = sphere.material;
        did_intersect = true;
        hit_light = false;
    }

    /* Find closest intersection with planes */
    for (uchar i = 0; i < planes_num; i++) {
        rplane      plane = planes[i];

        float _t;
        bool _intersect = intersect_plane(ray, &plane.normal, &plane.point_in_plane,&_t);
        if (!_intersect || _t >= t) {
            continue;
        }
        
        t = _t;
        interpoint = ray->origin+ray->dir*t;

        target_normal = plane.normal;
        interpoint += target_normal*EPSILON;
        
        transfer_material = plane.material;

        did_intersect = true;
        hit_light = false;
    }

    /*
    for (uchar i = 0; i < light_num; i++) {
        rlight light = lights[i];

        float _t;
        bool _intersect = intersect_sphere(ray, &light.origin, light.radius, &_t);
        if (!_intersect || _t >= t) {
            continue;
        }

        t = _t;     

        interpoint = ray->origin+ray->dir*t;
        target_normal = normalize(interpoint-light.origin);


        interpoint += target_normal*EPSILON;


        light_color = light.rgb*light.intensity*INVERSE_SQUARE_LIGHT*1/(light.radius*light.radius);
        did_intersect = true;
        hit_light = true;
    }
    */

    if (!did_intersect) { return 0; }

    *intersection   = interpoint;
    *normal         = target_normal;
    *material       = transfer_material;
    if (hit_light) {
        material->transperent   = false;
        material->fresnel       = 0.0f;
        material->ambient       = 1.0f;
        material->specular      = 0.0f;
        material->diffuse       = 0.0f;
        material->shininess     = 0.0f;
        /* High intensity */
        material->rgb           = light_color;
        return 2;
    }
    return 1;
}



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
