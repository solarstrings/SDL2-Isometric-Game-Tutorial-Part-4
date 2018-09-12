#ifndef __COMPONENT_COLLISION_H_
#define __COMPONENT_COLLISION_H_
#include <SDL2/SDL.h>
#include "../../IsoEngine/isoEngine.h"

typedef enum collisionTypeE
{
    COLLISIONTYPE_DEACTIVATED       = 0,
    COLLISIONTYPE_WORLD             = 1,
    COLLISIONTYPE_ENTITY            = 2,
    COLLISIONTYPE_WORLD_AND_ENTITY  = 3,
}collisionTypeE;

typedef struct componentCollisionT
{
    collisionTypeE collisionType;   //which collisions to apply to the entity
    SDL_Rect rect;                  //collision rectangle
    SDL_Rect worldRect;             //collision rectangle in world coordinates
}componentCollisionT;

componentCollisionT *componentNewCollision();
void componentFreeCollision(componentCollisionT *collisionComponent);
void componentCollisionSetCollisionType(componentCollisionT *collisionComponent,Uint32 entity,collisionTypeE collisionType);
void componentCollisionSetCollisionRectange(componentCollisionT *collisionComponent,Uint32 entity,SDL_Rect *collisionRect);
#endif // __COMPONENT_COLLISION_H_
