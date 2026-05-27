#pragma once

#include "defines.h"
#include "core/utils/vmath.h"

typedef struct _vtransform{
    vec3 position;
    vec3 rotation;
    vec3 scale;
    struct _vtransform *parent;
}vtransform;

/*!
 * @brief Pulls the transform component from the entity
 * @param entityID The ID of the entity to pull the transform component out of
 * @return Returns a pointer to the transorm, a null pointer if the entity doesn't have a transform component
 */
vtransform* get_transform_component(u64 entityID);

/*!
 * @brief Creates a transform that is filled with the provided parameters independent of the ecs system
 * @param posX The X component of the object's position
 * @param posY The Y component of the object's position
 * @param posZ The Z component of the object's position
 * @param rotX The rotation in degrees around the x axis
 * @param rotY The rotation in degrees around the y axis
 * @param rotZ The rotation in degrees around the z axis
 * @param scaX The amount of scaling in the x axis
 * @param scaY The amount of scaling in the y axis
 * @param scaZ The amount of scaling in the z axis
 * @param parent A pointer to the parent transform
 * @return A pass by reference vtransform
 */
vtransform create_transform(f32 posX, f32 posY, f32 posZ,
                            f32 rotX, f32 rotY, f32 rotZ,
                            f32 scaX, f32 scaY, f32 scaZ,
                            vtransform* parent);

/*!
 * @brief Registers a transform that already is created outside the ECS
 * @param entityID The ID of the entity to attach the empty transform to
 * @param trans A pass by reference transform, the data is copied to the ECS
 * @return TRUE if the component was able to be attached, FALSE if there was an existing transform component
 */
b8 register_existing_transform(u64 entityID, vtransform trans);

/*!
 * @brief Registers a transform that places the object at the origin, facing forward, scaled to 1.0
 * @param entityID The ID of the entity to attach the empty transform to
 * @param parent A pointer to the parent transform
 * @return TRUE if the component was able to be attached, FALSE if there was an existing transform component
 */
b8 register_empty_transform(u64 entityID, vtransform* parent);

/*!
 * @brief Registers a transform that is filled with the provided parameters
 * @param entityID The ID of the entity to attach the filled transform to
 * @param posX The X component of the object's position
 * @param posY The Y component of the object's position
 * @param posZ The Z component of the object's position
 * @param rotX The rotation in degrees around the x axis
 * @param rotY The rotation in degrees around the y axis
 * @param rotZ The rotation in degrees around the z axis
 * @param scaX The amount of scaling in the x axis
 * @param scaY The amount of scaling in the y axis
 * @param scaZ The amount of scaling in the z axis
 * @param parent A pointer to the parent transform
 * @return TRUE if the component was able to be attached, FALSE if there was an existing transform component
 */
b8 register_transform(u64 entityID, 
                      f32 posX, f32 posY, f32 posZ,
                      f32 rotX, f32 rotY, f32 rotZ,
                      f32 scaX, f32 scaY, f32 scaZ,
                      vtransform* parent);

/*!
 * @brief Sets the position of the provided transform component
 * @param transformComp A pointer that points to a transform component that's stored in the ECS
 * @param posX The X component of the object's position
 * @param posY The Y component of the object's position
 * @param posZ The Z component of the object's position
 */
void transform_set_position(vtransform *transformComp, f32 posX, f32 posY, f32 posZ);

/*!
 * @brief Registers a transform that is filled with the provided parameters
 * @param transformComp A pointer that points to a transform component that's stored in the ECS
 * @param rotX The rotation in degrees around the x axis
 * @param rotY The rotation in degrees around the y axis
 * @param rotZ The rotation in degrees around the z axis
 */
void transform_set_rotation(vtransform *transformComp, f32 rotX, f32 rotY, f32 rotZ);

/*!
 * @brief Registers a transform that is filled with the provided parameters
 * @param transformComp A pointer that points to a transform component that's stored in the ECS
 * @param scaX The amount of scaling in the x axis
 * @param scaY The amount of scaling in the y axis
 * @param scaZ The amount of scaling in the z axis
 */
void transform_set_scale(vtransform *transformComp, f32 scaX, f32 scaY, f32 scaZ);

/*!
 * @brief Sets the position of the provided transform component
 * @param transformComp A pointer that points to a transform component that's stored in the ECS
 * @param posX The amount to translate the position component along the x axis
 * @param posY The amount to translate the position component along the y axis
 * @param posZ The amount to translate the position component along the z axis
 */
void transform_move(vtransform *transformComp, f32 posX, f32 posY, f32 posZ);

/*!
 * @brief Sets the position of the provided transform component
 * @param transformComp A pointer that points to a transform component that's stored in the ECS
 * @param axis The amount to translate the position component along the x axis
 * @param rotationDegrees The amount in degrees to rotate around an axis
 */
void transform_rotate(vtransform *transformComp, vec3 axis, f32 rotationDegrees);

/*!
 * @brief Gets the model matrix of the current state of the transform component
 * @param transformComp The transform component to get the data from
 * @return A 4x4 matrix containing the model matrix made from the transform component
 */
mat4 transform_get_model_matrix(vtransform *transformComp);