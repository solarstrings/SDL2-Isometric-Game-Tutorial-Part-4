#include <stdio.h>
#include "component_collision.h"
#include "../../logger.h"
#include "../Scene/scene.h"

componentCollisionT *componentNewCollision()
{
    Uint32 i=0;
    //allocate memory for new collision components
    componentCollisionT *newCollisionComponent = malloc(sizeof(struct componentCollisionT)*NUM_INITIAL_ENTITIES);

    //if memory allocation failed
    if(newCollisionComponent == NULL)
    {
        writeToLog("Error in function componentNewCollision() - Could not allocate memory for collision components","runlog.txt");
        return NULL;
    }
    //loop through all entities
    for(i=0;i<NUM_INITIAL_ENTITIES;++i)
    {
        //set collision type to deactivated
        newCollisionComponent[i].collisionType = COLLISIONTYPE_DEACTIVATED;
        //create a default collision rectangle
        setupRect(&newCollisionComponent[i].rect,0,0,5,5);
    }
    //return the pointer to the collision components
    return newCollisionComponent;
}

void componentCollisionSetCollisionType(componentCollisionT *collisionComponent,Uint32 entity,collisionTypeE collisionType)
{
    if(collisionComponent != NULL)
    {
        collisionComponent[entity].collisionType = collisionType;
    }
    else
    {
        writeToLog("Error in function componentCollisionSetCollisionType() - Parameter 'componentCollisionT *collisionComponent' is NULL","runlog.txt");
    }
}

void componentCollisionSetCollisionRectange(componentCollisionT *collisionComponent,Uint32 entity,SDL_Rect *collisionRect)
{
    //make sure that the pointers are not NULL
    if(collisionComponent == NULL)
    {
        writeToLog("Error in function componentCollisionSetCollisionRectange() - Parameter 'componentCollisionT *collisionComponent' is NULL","runlog.txt");
        return;
    }
    else if(collisionRect == NULL)
    {
        writeToLog("Error in function componentCollisionSetCollisionRectange() - Parameter 'SDL_Rect *collisionRect' is NULL","runlog.txt");
        return;
    }
    //dereference the collision rectangle pointer and set the collision rectangle value
    collisionComponent[entity].rect = *collisionRect;
}

void componentFreeCollision(componentCollisionT *collisionComponent)
{
    if(collisionComponent!=NULL)
    {
        free(collisionComponent);
    }
}
