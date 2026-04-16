#pragma once
#include "defines.h"
#include "container/darray.h"

typedef struct _entity_component_system{
  darray *list;
  u32 entities[VELORA_MAX_OBJECTS];
}entity_component_system;

typedef enum _vcomponent_type{
  VELORA_COMPONENT_TRANSFORM,
  VELORA_COMPONENT_CAMERA,
}vcomponent_type;

typedef struct _vcomponent_list{
  vcomponent_type type;
  darray *data;
}vcomponent_list;

typedef struct _vcomponent{
  u64 entityID;
  u64 dataSize;
  void *data; 
}vcomponent;

/**
 * @brief Starting up the entity component system. This function must be called before any other
 * @return TRUE if ECS is initilized for the first time, FALSE if the ECS is already initilized
 */
b8 initilize_entity_component_system();

/**
 * @brief Clears out the ECS and deactivates it
 * @return FALSE if the ECS hasn't been initilized, TRUE otherwise
 */
b8 shutdown_entity_component_system();

/**
 * @brief Returns the lowest empty entity ID. If called twice without assigning a component to the entity, the same ID will be returned.
 * @return A u64 that hasn't been used by an entity previously or U64_MAX if there are no available entities
 */
u64 create_new_entity();

/**
 * @brief Registers a new component with the ECS. Only 1 component of it's type can be attached to each entity.
 * @param compType The type of component to register
 * @param entityId The ID of the entity to attach the component to
 * @param dataSize The size of the data contained in the data pointer
 * @param data The type of component to register
 * @return FALSE if ECS hasn't been initilized, a component of this type is already attached to the entity, or the entityID is beyond VELORA_MAX_OBJECTS. TRUE otherwise.
 */
b8 attach_component(vcomponent_type compType, u64 entityId, u64 dataSize, void *data);

/**
 * @brief Deletes a single component off an entity
 * @param type The type of component to delete
 * @param entityID The id of the entity to take the component off of
 * @return TRUE if the component existed and was attached to the entity, FALSE if otherwise
 */
b8 delete_component(vcomponent_type type, u64 entityID);

/**
 * @brief Gets a pointer to an array filled with the components of the type supplied. Do not free this pointer
 * @param type The type of components to get a list of
 * @param outList A double pointer that will point the dynamic array containing the components with the type requested
 * @return FALSE if no component of this type was ever registered with the ECS, TRUE otherwise
 */
b8 get_components(vcomponent_type type, darray** outList);

/**
 * @brief Goes through and deteles all components attached to an entity, this is very expensive and time consuming
 * @param entityID The id of the entity to remove from ECS
 * @return TRUE if any component was attached to this entity, FALSE otherwise
 */
b8 delete_entity(u64 entityID);

/**
 * @brief Retrieves pointer to the component data in the dynamic array. Do not free this memory.
 * @param entityID The id of the entity who's data we want to retrieve
 * @param compType The type of component to get from this entity
 * @return A pointer to the data in the component. Otherwise a null pointer if the entity doesn't contain a component of this type.
 */
void* get_component_data(u64 entityID, vcomponent_type compType);
