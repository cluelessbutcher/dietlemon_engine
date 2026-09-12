#pragma once

#include "defines.h"

typedef union vec2_u {
    float elements[2];
    struct {
        union {
            float x, r, s, u;
        };
        union {
            float y, g, t, v;
        };
    };
} vec2;

typedef struct vec3_u {  
  float elements[3];
  struct {
    union {
      float x, r, s, u;
    };
    union {
      float y, g, t, v;
    };
    union {
      float z, b, p, w;
    };
  };
} vec3;

typedef union vec4_u {
	float elements[4];
    union {
        struct {
            union {
                float x, r, s;
            };
            union {
                float y, g, t;
            };
            union {
                float z, b, p;
            };
            union {
                float w, a, q;
            };
        };
    };
} vec4;

typedef vec4 quat;

typedef union mat4_u {
  float data[16];
} mat4;
