#ifndef __TYPES_CL
#define __TYPES_CL

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

struct __rcamera {
    rray        pos_dir;

    float       fov;
    float       focal_length;
} __attribute__((packed));

typedef struct __rcamera rcamera;

#endif