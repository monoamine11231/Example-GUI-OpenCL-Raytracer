struct __rmaterial {
    float3              rgb;

    float               ambient;
    float               diffuse;
    float               specular;
    uint                shininess;
    
    uint                transperent;
    float               fresnel;
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
#define EPSILON 0.003f
#define INVERSE_SQUARE_LIGHT (M_1_PI_F/4)

bool intersect_sphere(rray *ray, float3* sphere_origin, float sphere_radius, float* t) {

    float3  v = ray->origin-(*sphere_origin);
    float   a = dot(ray->dir, ray->dir);
    float   b = dot(2*v, ray->dir);
    float   c = dot(v,v)-sphere_radius*sphere_radius;

    float   t2;

    float   D = b*b-4*a*c;

    /* No intersection */
    if (D < 0 || (-b+D)/(2*a) < 0) {
        return false;
    }
    D = sqrt(D);

    /* If the closest intersection is behind the camera, replace it with the
        farthest */
    t2 = ((-b-D)/(2*a) < 0) ? (-b+D)/(2*a) : (-b-D)/(2*a);

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
    if (t2 < 0) {
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

    /* Find closest intersection with spheres */
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
        /* To avoid self shadow */
        interpoint += target_normal*EPSILON;

        light_color = light.rgb*light.intensity*INVERSE_SQUARE_LIGHT*1/(light.radius*light.radius);
        did_intersect = true;
        hit_light = true;
    }

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
                        uchar spheres_num, uchar planes_num, uchar light_num  ) {

    uint id = get_global_id(0);
    rray ray = rays[id];
    
    while (ray.depth < MAX_DEPTH) {
        float3 intersection;
        float3 normal;
        rmaterial material;
        bool intersect = findIntersection(&ray, spheres, planes, lights, spheres_num,
                                          planes_num, light_num, &intersection, &normal,
                                          &material);


        if (!intersect) { return; }

        output[id].rgb += material.rgb * material.ambient;

        float3 new_dir = ray.dir - 2*dot(normal, ray.dir)*normal;
        ray.dir = new_dir;
        ray.origin = intersection;
        ray.depth++;

    }

    output[id].rgb = clamp(output[id].rgb, 0.0f, 1.0f);
}