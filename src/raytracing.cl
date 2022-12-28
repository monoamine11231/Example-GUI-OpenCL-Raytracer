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


__kernel void raytracer(__global rray* rays, __global rray* output,
                        __global rsphere* spheres,
                        __global rplane* planes, __global rlight* lights,
                        uchar spheres_num, uchar planes_num, uchar light_num  ) {

    //printf("Plane 1: %f %f %f\n", planes[1].point_in_plane.x, planes[1].point_in_plane.y, planes[1].point_in_plane.z);
    rray ray = rays[get_global_id(0)];
    
    while (ray.depth < MAX_DEPTH) {
        rlight              light;
        rmaterial           material;

        bool hit_light      = false;
        /* Takes account the intensity */
        float3 light_color;

        bool intersection   = false;

        float t             = INFINITY;
        float3 normal;
        float3 interpoint;

        /* Find closest intersection with spheres */
        for (uchar i = 0; i < spheres_num; i++) {
            rsphere sphere = spheres[i];
            float3  v = ray.origin-sphere.origin;
            float   a = dot(ray.dir, ray.dir);
            float   b = dot(2*v, ray.dir);
            float   c = dot(v,v)-sphere.radius*sphere.radius;

            float   t2;

            float   D = b*b-4*a*c;

            /* No intersection */
            if (D < 0 || (-b+D)/(2*a) < 0) {
                continue;
            }

            /* If the closest intersection is behind the camera, replace it with the
               farthest */
            t2 = ((-b-D)/(2*a) < 0) ? (-b+D)/(2*a) : (-b-D)/(2*a);
            /* If closer than the previous intersections just replace it */
            if (t2 >= t) { continue; }

            t = t2;

            interpoint = ray.origin+ray.dir*t;
            normal = normalize(interpoint-sphere.origin);
            /* To avoid self shadow */
            interpoint += normal*EPSILON;

            material = sphere.material;
            intersection = true;
            hit_light = false;
        }

        /* Find closest intersection with planes */
        for (uchar i = 0; i < planes_num; i++) {
            rplane      plane = planes[i];
            float       t2;
            float       b;

            b = dot(ray.dir, plane.normal);
            
            /* No intersection */
            if (b == 0) {
                continue;
            }

            
            t2 = dot((plane.point_in_plane-ray.origin), plane.normal)/b;
            if (t2 < 0 || t2 >= t) { continue; }

            t = t2;
            interpoint = ray.origin+ray.dir*t;

            normal = plane.normal;
            interpoint += normal*EPSILON;
            
            material = plane.material;
            
            intersection = true;
            hit_light = false;
        }

        if (intersection) {
            if (hit_light) {
                output[get_global_id(0)].rgb += clamp(light_color, 0.0f, 1.0f);
            } else {
                output[get_global_id(0)].rgb += clamp(material.rgb * material.ambient * 5, 0.0f, 1.0f);
            }
        }
        return;
    }
}