/*
 * Copyright 2013 Dario Manesku. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#ifndef CAMERA_H_HEADER_GUARD
#define CAMERA_H_HEADER_GUARD

#include "entry/entry.h"

#define CAMERA_KEY_FORWARD   UINT8_C(0x01)
#define CAMERA_KEY_BACKWARD  UINT8_C(0x02)
#define CAMERA_KEY_LEFT      UINT8_C(0x04)
#define CAMERA_KEY_RIGHT     UINT8_C(0x08)
#define CAMERA_KEY_UP        UINT8_C(0x10)
#define CAMERA_KEY_DOWN      UINT8_C(0x20)

///
void cameraCreate();

///
void cameraDestroy();

///
void cameraSetPosition(const bx::Vec3& _pos);

///
void cameraSetHorizontalAngle(float _horizontalAngle);

///
void cameraSetVerticalAngle(float _verticalAngle);

///
void cameraSetKeyState(uint8_t _key, bool _down);

void mtxLookAt_wasd(float* _result, const bx::Vec3& _eye, const bx::Vec3& _at, const bx::Vec3& _up, bx::Handness::Enum _handness);
///
void cameraGetViewMtx(float* _viewMtx);
///
void cameraSetDollyState(float _dz , float _deltaTime);

///
void cameraSetOrbitState(float _dx, float _dy, float _deltaTim);

///
bx::Vec3 cameraGetPosition();
///
void cameraGetenvViewMtx(float* _mtx);
///
bx::Vec3 cameraGetAt();

///
void cameraUpdate(float _deltaTime);

#endif // CAMERA_H_HEADER_GUARD
