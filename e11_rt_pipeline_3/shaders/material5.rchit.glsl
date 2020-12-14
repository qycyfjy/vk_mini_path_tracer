// Copyright 2020 NVIDIA Corporation
// SPDX-License-Identifier: Apache-2.0
#version 460
#extension GL_GOOGLE_include_directive : require
#include "closestHitCommon.h"

void main()
{
  HitInfo hitInfo = getObjectHitInfo();

  if(mod(dot(hitInfo.objectPosition, vec3(1, 1, 1)), 0.5) >= 0.25)
  {
    pld.color        = vec3(0.7);
    pld.rayOrigin    = offsetPositionAlongNormal(hitInfo.worldPosition, hitInfo.worldNormal);
    pld.rayDirection = diffuseReflection(hitInfo.worldNormal, pld.rngState);
  }
  else
  {
    pld.color        = vec3(1.0);
    pld.rayOrigin    = offsetPositionAlongNormal(hitInfo.worldPosition, -hitInfo.worldNormal);
    pld.rayDirection = gl_WorldRayDirectionEXT;
  }
  pld.rayHitSky = false;
}