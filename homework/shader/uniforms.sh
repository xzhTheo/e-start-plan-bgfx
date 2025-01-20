/*
 * Copyright 2016 Dario Manesku. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

uniform vec4 u_params[12];
#define u_mtx0          u_params[0]
#define u_mtx1          u_params[1]
#define u_mtx2          u_params[2]
#define u_mtx3          u_params[3]
#define blinn_phong     u_params[4].x
#define pbr             u_params[4].y
#define pbr_ibl         u_params[4].z
#define u_bgType        u_params[4].w
#define normalMap       u_params[5].x
#define shadmap       u_params[5].y
#define pcf             u_params[5].z
#define u_unused        u_params[5].w
#define u_doDiffuse     u_params[6].x
#define u_doSpecular    u_params[6].y
#define u_doDiffuseIbl  u_params[6].z
#define u_doSpecularIbl u_params[6].w
#define u_camPos        u_params[7].xyz
#define u_unused7       u_params[7].w
#define doPrefilterMap  u_params[8].x
#define doBrdfLut       u_params[8].y
#define u_rgbSpec       u_params[9]
#define lightPos        u_params[10].xyz
#define u_unused10      u_params[10].w
#define u_lightCol      u_params[11].xyz
#define u_unused11      u_params[11].w
