#pragma once

#include "defines.h"
#include "ecs/vtransform.h"

typedef struct _vrenderable{
  u64 meshHandle;
  u64 materialHandle;
  vtransform *transform;
}vrenderable;

/*!
 * @brief Registers a renderable component to the entity with the ID provided
 * @param entityID The ID of the entity to attach the renderable component to
 * @param meshHandle The handle rreturned by registering the mesh to the render system
 * @param materialHandle The handle returned by registering the material to the render system
 * @return TRUE if the component was able to be attached, FALSE if there was an existing renderable component or if the entity does not have a transform component
 */
b8 register_renderable(u64 entityID, u64 meshHandle, u64 materialHandle);