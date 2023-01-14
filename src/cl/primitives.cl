#ifndef __PRIMITIVES_CL
#define __PRIMITIVES_CL

/* To avoid self shadow, we add the normal * EPSILON to the intersection */
#define EPSILON 0.001f
#define INVERSE_SQUARE_LIGHT M_1_PI_F

#define PRINT_VEC(v) printf("%f %f %f\n", v.x, v.y, v.z)


typedef struct {
    uint x;
} xorshift32_state;

/* Pseudo random generator for soft shadow sampling */
float xorshift32(xorshift32_state *state) {
    uint x = state->x;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    state->x = x;

    return ((float)x)/2147483648.0f*2.0f;
}

float3 reflect(float3 *incident, float3 *normal) {
    float cosI = -dot(*normal, *incident);
    return *incident + 2*cosI * (*normal);
}

float3 refract(float n1, float n2, float3* incident, float3* normal)
{
    float n = n1/n2;
    float cosI = -dot(*normal, *incident);
    float sinT2 = n*n*(1.0f-cosI*cosI);

    if (sinT2 > 1.0f) {
        return (float3){NAN, NAN, NAN};
    }

    float cosT = sqrt(1.0f - sinT2);
    return n*(*incident)+(n*cosI-cosT)*(*normal);
}

float reflectance(float n1, float n2, float3* incident, float3* normal) {
    float n = n1/n2;
    float cosI = -dot(*normal, *incident);
    float sinT2 = n*n*(1.0f-cosI*cosI);
    if (sinT2 > 1.0f) { return 1.0f; }

    float cosT = sqrt(1.0f-sinT2);
    float rorth = (n1*cosI-n2*cosT)/(n1*cosI+n2*cosT);
    float rPar = (n2*cosI-n1*cosT)/(n2*cosI+n1*cosT);
    return (rorth*rorth+rPar*rPar) / 2.0f;
}

float compute_schlick(float n1, float n2, float3* incident, float3 *normal) {
    float r0 = (n1-n2)/(n1+n2);
    r0*=r0;
    float cosX = -dot(*normal, *incident);
    if (n1 > n2) {
        float n = n1/n2;
        float sinT2 = n*n*(1.0f-cosX*cosX);
        if (sinT2 > 1.0f) { return 1.0f; }

        cosX = sqrt(1.0f-sinT2);
    }

    float x = 1.0f-cosX;
    return r0+(1.0f-r0)*x*x*x*x*x;
}

int euclidean_modulo(int a, int b) {
  int m = a % b;
  if (m < 0) {
    m = (b < 0) ? m - b : m + b;
  }
  return m;
}

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

float3 plane_texture_pixel(rplane *plane, float3 *interpoint, image2d_array_t im_arr) {
    float3 vecs[3];
    vecs[0] = (float3){1.0f, 0.0f, 0.0f};
    vecs[1] = (float3){0.0f, 1.0f, 0.0f};
    vecs[2] = (float3){0.0f, 0.0f, 1.0f};

    float3 basis[2];

    /* Calculate the basis for the plane */
    for (int i = 0; i < 3; i++) {
        float3 cr = cross(vecs[i], plane->normal);
        if (dot((float3){1.0f,1.0f,1.0f}, cr) == 0.0f) {
            continue;
        }

        basis[0] = cr;
        basis[1] = cross(plane->normal, cr);
        break;
    }

    float ui = dot(basis[0], *interpoint)*plane->material.texture_scale;
    float vi = dot(basis[1], *interpoint)*plane->material.texture_scale;

    int2 im_dim = get_image_dim(im_arr);
    
    /* Data used to fetch the pixel from the texture */
    /* euclidean_modulo to guarantee no negative values on coordinates */
    int4 pixel_fetch = (int4){
        euclidean_modulo((int)ui, im_dim[0]),
        euclidean_modulo((int)vi, im_dim[1]),
        plane->material.texture_id, 0 
    };

    int4    pixeli = read_imagei(im_arr, pixel_fetch);
    /* Cast to normalized float manually */
    float3  pixelf = (float3){
        (float)pixeli.x/255.0f,
        (float)pixeli.y/255.0f,
        (float)pixeli.z/255.0f
    };

    return pixelf;
}


bool findLightIntersection(rray *ray,
                        __global rlight *lights,
                        uint light_num,
                        float3 *color) {
    bool did_intersect = false;
    float t = INFINITY;
    float3 color_;

    for (uchar i = 0; i < light_num; i++) {
        rlight light = lights[i];

        float _t;
        bool _intersect = intersect_sphere(ray, &light.origin, light.radius, &_t);
        if (!_intersect || _t >= t) {
            continue;
        }

        t = _t;     
        float3 interpoint = ray->origin+ray->dir*t;
        float d = distance(ray->origin, interpoint);

        color_ = light.rgb*light.intensity*INVERSE_SQUARE_LIGHT*(1/d*d);
        did_intersect = true;
    }

    *color = color_;
    return did_intersect;
}

/*  RETURN 0: NO INTERSECTION
    RETURN 1: INTERSECTION SOLID OBJECT */
bool findSolidIntersection(rray *ray,
                      __global rsphere* spheres, __global rplane* planes,
                      uchar spheres_num, uchar planes_num,
                      float3* intersection, float3* normal, rmaterial* material,
                      read_only image2d_array_t im_arr) {
                        
    rmaterial           transfer_material;

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

        transfer_material = plane.material;

        /* If there's a texture attached on the plane */
        if (plane.material.texture_id >= 0) {


            transfer_material.rgb = plane_texture_pixel(&plane, &interpoint, im_arr);
        }

        interpoint += target_normal*EPSILON;

        did_intersect = true;
    }



    if (!did_intersect) { return 0; }

    *intersection   = interpoint;
    *normal         = target_normal;
    *material       = transfer_material;

    return 1;
}

bool testShadowPath(float3 *to, float3 *from, __global rsphere *spheres,
                    __global rplane *planes, uint spheres_num, uint planes_num) {

    rray ray;
    ray.origin = *from;
    ray.dir = normalize((*to)-(*from));

    float t                 = distance(*to, *from);

    /* Find closest intersection with spheres */
    for (uchar i = 0; i < spheres_num; i++) {
        rsphere sphere = spheres[i];
        if (sphere.material.transperent) {continue;}
        
        float _t;
        bool _intersect = intersect_sphere(&ray, &sphere.origin, sphere.radius, &_t);
        if (!_intersect || _t >= t) {
            continue;
        }

        return true;
    }

    /* Find closest intersection with planes */
    for (uchar i = 0; i < planes_num; i++) {
        rplane      plane = planes[i];

        float _t;
        bool _intersect = intersect_plane(&ray, &plane.normal, &plane.point_in_plane,&_t);
        if (!_intersect || _t >= t) {
            continue;
        }
        
        t = _t;

        return true;
    }

    return false;
}

#endif