
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <SDL2/SDL.h>
#include "../../IsoEngine/isoEngine.h"
#include "../../deltatimer.h"
#include "../../logger.h"
#include "../Entity/entity.h"
#include "scene.h"

//function prototypes, allowing the functions to be used before they are defined.
static void freeComponentsFromScene(sceneT *scene);
static void freeSystemsFromScene(sceneT *scene);
static int allocateMoreMemoryForComponents(sceneT *scene);
static void moveEntityIndexInComponentsAfterEntityRemove(sceneT* scene, Uint32 entityID);

sceneT *ecsSceneCreateNewScene(char *name)
{
    char msg[512];
    Uint32 i=0;
    //create the entity manager
    sceneT *scene = malloc(sizeof(struct sceneT));

    //if memory allocation for the scene failed
    if(scene == NULL){
        //log the error and return NULL.
        writeToLog("Error in ecsCreateNewScene() - Could not allocate memory for a new scene!","runlog.txt");
        return NULL;
    }
    //if no name was passed to the function
    if(name == NULL){
        //Set the scene name to "newmap"
        snprintf(scene->name,SCENE_NAME_LENGTH,"Newmap");
    }
    //otherwise
    else{
        //Set the scene name
        snprintf(scene->name,SCENE_NAME_LENGTH,"%s",name);
    }

    //allocate memory for the entities
    scene->entities = malloc(sizeof(entityT)*NUM_INITIAL_ENTITIES);

    //if memory allocation failed
    if(scene->entities == NULL)
    {
        //free the entity manager
        free(scene);

        //log the error
        sprintf(msg,"Error in ecsCreateNewScene() - Could not allocate memory for: %d, entities!",NUM_INITIAL_ENTITIES);
        writeToLog(msg,"runlog.txt");

        //return NULL
        return NULL;
    }
    //initialize the values
    scene->numEntities = 0;
    scene->maxEntities = NUM_INITIAL_ENTITIES;
    scene->memallocFailed = 0;
    scene->deleteScene = 0;
    scene->sceneHasInputKeyboardComponent = 0;
    scene->sceneHasInputMouseComponent = 0;
    scene->sceneHasInputSystem = 0;
    scene->exitScene = 0;
    scene->consumeLessCPU = 0;
    scene->isoEngine = NULL;
    scene->componentPointersRealloced = 0;

    //loop through all entities
    for(i = 0;i<scene->maxEntities;++i)
    {
        //initialize its components to NONE.
        scene->entities[i].componentSet1 = COMPONENT_NONE;
        //scene->entities[i].componentSet2 = COMPONENT_NONE;
        scene->entities[i].id = 0;
    }

    //allocate memory for the components
    scene->components = malloc(sizeof(componentT)*COMPONENT_TYPE_COUNT);

    //If memory allocation for the components failed
    if(scene->components == NULL){

        //free the entity list
        free(scene->entities);

        //free the entity manager
        free(scene);

        //log the error
        sprintf(msg,"Error in ecsCreateNewScene() - Could not allocate memory for components!");
        writeToLog(msg,"runlog.txt");

        //return NULL
        return NULL;
    }
    //set number of components to 0
    scene->numComponents = 0;
    //set max components to the component type count
    scene->maxComponents = COMPONENT_TYPE_COUNT;

    //set component type to NONE for all allocated components
    for(i=0;i<scene->maxComponents;++i){
        scene->components[i].type = COMPONENT_NONE;
        scene->components[i].data = NULL;
    }

    //allocate memory for systems
    scene->systems = malloc(sizeof(struct systemT)*NUM_INITIAL_SYSTEMS);

    //if the memory allocation for the systems failed
    if(scene->systems == NULL)
    {
        //free the components
        free(scene->components);

        //free the entity list
        free(scene->entities);

        //free the entity manager
        free(scene);

        //log the error
        sprintf(msg,"Error in ecsCreateNewScene() - Could not allocate memory for systems!");
        writeToLog(msg,"runlog.txt");

        //return NULL
        return NULL;
    }

    //initialize the system variables
    scene->numSystems = 0;
    scene->maxSystems = NUM_INITIAL_SYSTEMS;
    scene->systemInitFailed = 0;

    //initialize all systems to none
    for(i=0;i<NUM_INITIAL_SYSTEMS;++i){
        scene->systems[i].type = SYSTEM_NONE;
        scene->systems[i].init = NULL;
        scene->systems[i].updateEntity = NULL;
        scene->systems[i].update = NULL;
    }

    //return the entity manager
    return scene;
}

int ecsSceneAddComponentToScene(sceneT *scene,componentTypeE componentType)
{
    Uint32 i=0;
    char msg[512];
    char componentName[200];

    //get the component name (stored in the componentName variable)
    ecsGetComponentName(componentType,componentName);

    //if the passed pointer to the scene is NULL
    if(scene == NULL){
        //log the error and return NULL.
        writeToLog("Error in ecsSceneAddComponentToScene() - sceneT *scene is NULL!","runlog.txt");
        return -1;
    }

    //if the components in the scene are not allocated
    if(scene->components == NULL)
    {
        //log the error and return NULL.
        writeToLog("Error in ecsSceneAddComponentToScene() - scene->components is NULL!","runlog.txt");
        return -1;
    }

    //check if the component type already added to the scene
    for(i=0;i<scene->numComponents;++i){
        //if so
        if(scene->components[i].type == componentType)
        {
            //log the error and return 0.
            sprintf(msg,"Warning: in ecsSceneAddComponentToScene() - ComponentType: %s, already added to the scene!",componentName);
            writeToLog(msg,"runlog.txt");
            return 0;
        }
    }

    //// POSITION COMPONENT
    if(componentType == COMPONENT_SET1_POSITION){
        //set the component type
        scene->components[scene->numComponents].type = componentType;

        //set the component data
        scene->components[scene->numComponents].data = componentNewPosition();

        //if the data is NULL; return -1. An error was written to the log file int componentNewPosition()
        if(scene->components[scene->numComponents].data == NULL){
            //flag that memory allocation has failed
            scene->memallocFailed=1;
            return -1;
        }
        //increase number of components
        scene->numComponents++;
        return 1;
    }
    //// VELOCITY COMPONENT
    else if(componentType == COMPONENT_SET1_VELOCITY){
        //set the component type
        scene->components[scene->numComponents].type = componentType;

        //set the component data
        scene->components[scene->numComponents].data = componentNewVelocity();

        //if the data is NULL; return -1. An error was written to the log file int componentNewPosition()
        if(scene->components[scene->numComponents].data == NULL){
            //flag that memory allocation has failed
            scene->memallocFailed=1;

            return -1;
        }
        //increase number of components
        scene->numComponents++;
        return 1;
    }
    //// KEYBOARD COMPONENT
    else if(componentType == COMPONENT_SET1_KEYBOARD){
        scene->components[scene->numComponents].type = componentType;
        scene->components[scene->numComponents].data = componentNewInputKeyboard();
        if(scene->components[scene->numComponents].data == NULL){
            scene->memallocFailed=1;
            return -1;
        }
        //increase number of components
        scene->numComponents++;
        scene->sceneHasInputKeyboardComponent = 1;
        return 1;
    }
    //// MOUSE COMPONENT
    else if(componentType == COMPONENT_SET1_MOUSE){
        scene->components[scene->numComponents].type = componentType;
        scene->components[scene->numComponents].data = componentNewInputMouse();
        if(scene->components[scene->numComponents].data == NULL){
            scene->memallocFailed=1;
            return -1;
        }
        //increase number of components
        scene->numComponents++;
        scene->sceneHasInputMouseComponent = 1;
        return 1;
    }
    //// RENDER2D COMPONENT
    else if(componentType == COMPONENT_SET1_RENDER2D){
        //set the component type
        scene->components[scene->numComponents].type = componentType;

        //set the component data
        scene->components[scene->numComponents].data = componentNewRender2D();

        //if the data is NULL; return -1. An error was written to the log file int componentNewRender2D()
        if(scene->components[scene->numComponents].data == NULL){
            //flag that memory allocation has failed
            scene->memallocFailed=1;

            return -1;
        }
        //increase number of components
        scene->numComponents++;
        return 1;
    }
    //// NAMETAG COMPONENT
    else if(componentType == COMPONENT_SET1_NAMETAG){
        //set the component type
        scene->components[scene->numComponents].type = componentType;

        //set the component data
        scene->components[scene->numComponents].data = componentNewNameTag();

        //if the data is NULL; return -1. An error was written to the log file int componentNewNameTag()
        if(scene->components[scene->numComponents].data == NULL){
            //flag that memory allocation has failed
            scene->memallocFailed=1;

            return -1;
        }
        //increase number of components
        scene->numComponents++;
        return 1;
    }
    //// NAMETAG COMPONENT
    else if(componentType == COMPONENT_SET1_COLLISION){
        //set the component type
        scene->components[scene->numComponents].type = componentType;

        //set the component data
        scene->components[scene->numComponents].data = componentNewCollision();

        //if the data is NULL; return -1. An error was written to the log file int componentNewNameTag()
        if(scene->components[scene->numComponents].data == NULL){
            //flag that memory allocation has failed
            scene->memallocFailed=1;

            return -1;
        }
        //increase number of components
        scene->numComponents++;
        return 1;
    }
    //// COMPONENT NOT IMPLEMENTED
    else
    {
        //log that the component is not implemented yet
        sprintf(msg,"\nWarning: in ecsSceneAddComponentToScene() - ComponentType:%s with bit position: %d, has not been implemented, component NOT added!\n",componentName,ecsGetComponentBit(componentType));
        writeToLog(msg,"runlog.txt");

        //also log it to the console
        fprintf(stderr,"%s",msg);
    }
    //return
    return -2;
}
static void freeSystemsFromScene(sceneT *scene)
{
    Uint32 i=0;
    if(scene==NULL){
        writeToLog("Warning in Function freeSystemsFromScene() - sceneT* scene is NULL!","runlog.txt");
        return;
    }
    if(scene->systems == NULL){
        return;
    }
    //loop through the systems
    for(i=0;i<scene->numSystems;++i)
    {
        //free the system
        scene->systems[i].free();
    }
}
static void freeComponentsFromScene(sceneT *scene)
{
    Uint32 i=0;
    char msg[512];
    char componentName[200];
    if(scene==NULL){
        writeToLog("Warning in Function freeComponentsFromScene() - sceneT* scene is NULL!","runlog.txt");
        return;
    }
    if(scene->components == NULL){
        writeToLog("Warning in Function freeComponentsFromScene() - scene->components NULL!","runlog.txt");
        return;
    }
    //Loop through all components & free them
    for(i=0;i<scene->numComponents;++i)
    {
        //// POSITION COMPONENT
        if(scene->components[i].type == COMPONENT_SET1_POSITION){
            componentFreePosition((componentPositionT*)scene->components[i].data);
        }
        //// VELOCITY COMPONENT
        else if(scene->components[i].type == COMPONENT_SET1_VELOCITY){
            componentFreeVelocity((componentVelocityT*)scene->components[i].data);
        }
        //// INPUT KEYBOARD COMPONENT
        else if(scene->components[i].type == COMPONENT_SET1_KEYBOARD){
            componentFreeInputKeyboard((componentInputKeyboardT*)scene->components[i].data,scene->maxEntities);
        }
        //// INPUT MOUSE COMPONENT
        else if(scene->components[i].type == COMPONENT_SET1_MOUSE){
            componentFreeInputMouse((componentInputMouseT*)scene->components[i].data,scene->maxEntities);
        }
        //// RENDER 2D COMPONENT
        else if(scene->components[i].type == COMPONENT_SET1_RENDER2D){
            componentFreeRender2D((componentRender2DT*)scene->components[i].data);
        }
        //// NAME TAG COMPONENT
        else if(scene->components[i].type == COMPONENT_SET1_NAMETAG){
            componentFreeNameTag((componentNameTagT*)scene->components[i].data,scene->maxEntities);
        }
        //// COLLISION COMPONENT
        else if(scene->components[i].type == COMPONENT_SET1_COLLISION){
            componentFreeCollision((componentCollisionT*)scene->components[i].data);
        }
        //// UNHANDLED COMPONENT
        else{
            //get the component name
            ecsGetComponentName(scene->components[i].type,componentName);
            //log the error
            sprintf(msg,"\nError in Function freeComponentsFromScene() - ComponentType:%s with bit:%d is missing. Add free for it!\n",componentName,ecsGetComponentBit(scene->components[i].type));
            writeToLog(msg,"runlog.txt");
            //log error to console as well.
            fprintf(stderr,"%s",msg);
        }
    }
}

void ecsSceneFreeScene(sceneT *scene)
{
    //if the scene has allocated memory
    if(scene != NULL)
    {
        //free the entities
        if(scene->entities!=NULL){
            free(scene->entities);
        }
        //free the components
        if(scene->components!=NULL){
            //free each individual component inside the components pointer
            freeComponentsFromScene(scene);
            //free the components pointer
            free(scene->components);
        }
        //free the systems
        if(scene->systems!=NULL)
        {
            //free memory allocated by the systems
            freeSystemsFromScene(scene);
            //free the systems pointer
            free(scene->systems);
        }
        //free the isometric engine
        if(scene->isoEngine!=NULL)
        {
            //the isoEngineFreeIsoEngine function also frees the scene->isoEngine pointer
            isoEngineFreeIsoEngine(scene->isoEngine);
        }

        //free the scene
        free(scene);
    }
}

Uint32 ecsSceneAddEntityToScene(sceneT* scene, Uint32 componentSet1) //,Uint32 componentSet2)
{
    entityT *newEntityList = NULL;
    if(scene == NULL)
    {
        writeToLog("Error in Function ecsSceneAddEntity() - sceneT* scene is NULL!","runlog.txt");
        return -1;
    }
    //if we're on the last entity, and no reallocation errors has occurred
    if(scene->numEntities >= scene->maxEntities && scene->memallocFailed == 0)
    {
        //try add another 1000 entities to the list
        scene->maxEntities+=1000;
        newEntityList = realloc(scene->entities,sizeof(struct entityT)*scene->maxEntities);

        //if the new entity list could not be created
        if(newEntityList==NULL)
        {
            //roll back max number of entities
            scene->maxEntities-=1000;
            writeToLog("Error in Function ecsSceneAddEntityToScene() - Failed to re-allocate memory for new entities!","runlog.txt");
        }
        //if list could be reallocated
        else{

            //allocate more memory for the components as well
            if(allocateMoreMemoryForComponents(scene) == 0)
            {
                //if memory re-allocation failed

                //free the new entity list. It us useless without components mapped to it.
                free(newEntityList);

                //TRY TO FREE SOME MEMORY HERE IF POSSIBLE, THEN HAVE A RE-TRY.

                //set the realloc Failed flag, stopping the engine from allocating more memory.
                scene->memallocFailed = 1;

                //roll-back number of entities.
                scene->maxEntities-=1000;

                //return -1
                return -1;
            }

            //point the entity list to the new one.
            scene->entities = newEntityList;

            //clear the components for the new entity
            scene->entities[scene->numEntities].componentSet1 = 0;
            //scene->entities[scene->numEntities].componentSet2 = 0;    //<- add when the number of components is more than 32

            //add the components to the entity
            scene->entities[scene->numEntities].componentSet1 = componentSet1;
            //scene->entities[scene->numEntities].componentSet2 = componentSet2;    //<- add when the number of components is more than 32

            //set the new entity
            scene->entities[scene->numEntities].id = scene->numEntities;
            scene->numEntities++;
            //return the entity index
            return scene->numEntities-1;
        }
    }
    else{
        //set the new entity
        scene->entities[scene->numEntities].componentSet1 = componentSet1;
        //scene->entities[scene->numEntities].componentSet2 = componentSet2;
        scene->entities[scene->numEntities].id = scene->numEntities;
        //increase number of entities
        scene->numEntities++;
        //return the entity index
        return scene->numEntities-1;
    }
    //if something went wrong
    return -1;
}

//each time the entities increase, so must the components.
static int allocateMoreMemoryForComponents(sceneT *scene)
{
    Uint32 i=0,j=0;
    char msg[512];
    char componentName[200];
    componentPositionT *newComponentPosition = NULL;
    componentVelocityT *newComponentVelocity = NULL;
    componentInputKeyboardT *newComponentInputKeyboard = NULL;
    componentInputMouseT *newComponentInputMouse = NULL;
    componentRender2DT *newComponentRender2D = NULL;
    componentNameTagT *newComponentNameTag = NULL;
    componentCollisionT *newComponentCollision = NULL;

    //flag that the component pointers has been reallocated
    scene->componentPointersRealloced = 1;

    for(i=0;i<scene->numComponents;++i)
    {
        //// POSITION COMPONENT
        if(scene->components[i].type == COMPONENT_SET1_POSITION)
        {
            newComponentPosition = realloc((componentPositionT*)scene->components[i].data,sizeof(struct componentPositionT)*scene->maxEntities);
            if(newComponentPosition == NULL){
                //write the error to the logfile
                writeToLog("Error in Function allocateMoreMemoryForComponents() - Could not allocate more memory for new position components! ","runlog.txt");
                //mark that memory allocation failed
                scene->memallocFailed = 1;
                return 0;
            }
            //initialize the component for the new entities
            for(j=scene->numEntities;j<scene->maxEntities;++j){
                newComponentPosition[j].x = 0;
                newComponentPosition[j].y = 0;
                newComponentPosition[j].xOffset = 0;
                newComponentPosition[j].yOffset = 0;
            }
            //point the pointer to the new data
            scene->components[i].data = newComponentPosition;
        }
        //// VELOCITY COMPONENT
        else if(scene->components[i].type == COMPONENT_SET1_VELOCITY)
        {
            newComponentVelocity = realloc((componentVelocityT*)scene->components[i].data,sizeof(struct componentVelocityT)*scene->maxEntities);
            if(newComponentVelocity == NULL){
                //write the error to the logfile
                writeToLog("Error in Function allocateMoreMemoryForComponents() - Could not allocate more memory for velocity components! ","runlog.txt");
                //mark that memory allocation failed
                scene->memallocFailed = 1;
                return 0;
            }
            //initialize the component for the new entities
            for(j=scene->numEntities;j<scene->maxEntities;++j){
                newComponentVelocity[j].x = 0;
                newComponentVelocity[j].y = 0;
                newComponentVelocity[j].friction = 1;
                newComponentVelocity[j].maxVelocity = 1000;
            }
            //point the data pointer to the new data
            scene->components[i].data = newComponentVelocity;
        }
        //// KEYBOARD COMPONENT
        else if(scene->components[i].type == COMPONENT_SET1_KEYBOARD)
        {
            newComponentInputKeyboard = realloc((componentInputKeyboardT*)scene->components[i].data,sizeof(struct componentInputKeyboardT)*scene->maxEntities);
            if(newComponentInputKeyboard == NULL){
                //write the error to the logfile
                writeToLog("Error in Function allocateMoreMemoryForComponents() - Could not allocate more memory for input keyboard components! ","runlog.txt");
                //mark that memory allocation failed
                scene->memallocFailed = 1;
                return 0;
            }

            //initialize the component for the new entities
            for(j=scene->numEntities;j<scene->maxEntities;++j){
                newComponentInputKeyboard[j].actions = NULL;
                newComponentInputKeyboard[j].active = 0;
                newComponentInputKeyboard[j].maxActions = 0;
                newComponentInputKeyboard[j].numActions = 0;
            }
            //point the data pointer to the new data
            scene->components[i].data = newComponentInputKeyboard;
        }
        //// MOUSE COMPONENT
        else if(scene->components[i].type == COMPONENT_SET1_MOUSE)
        {
            newComponentInputMouse = realloc((componentInputMouseT*)scene->components[i].data,sizeof(struct componentInputMouseT)*scene->maxEntities);
            if(newComponentInputMouse == NULL){
                //write the error to the logfile
                writeToLog("Error in Function allocateMoreMemoryForComponents() - Could not allocate more memory for input mouse components! ","runlog.txt");
                //mark that memory allocation failed
                scene->memallocFailed = 1;
                return 0;
            }
            //initialize the component for the new entities
            for(j=scene->numEntities;j<scene->maxEntities;++j){
                newComponentInputMouse[j].actions = NULL;
                newComponentInputMouse[j].active = 0;
                newComponentInputMouse[j].maxActions = 0;
                newComponentInputMouse[j].numActions = 0;
            }
            //point the data pointer to the new data
            scene->components[i].data = newComponentInputMouse;
        }
        //// RENDER 2D COMPONENT
        else if(scene->components[i].type == COMPONENT_SET1_RENDER2D)
        {
            newComponentRender2D = realloc((componentRender2DT*)scene->components[i].data,sizeof(struct componentRender2DT)*scene->maxEntities);
            if(newComponentRender2D == NULL){
                //write the error to the logfile
                writeToLog("Error in Function allocateMoreMemoryForComponents() - Could not allocate more memory for render2D components! ","runlog.txt");
                //mark that memory allocation failed
                scene->memallocFailed = 1;
                return 0;
            }
            //initialize the component for the new entities
            for(j=scene->numEntities;j<scene->maxEntities;++j){
                newComponentRender2D[j].texture = NULL;
                newComponentRender2D[j].layer = -1;
            }
            //point the data pointer to the new data
            scene->components[i].data = newComponentRender2D;
        }
        //// NAMETAG COMPONENT
        else if(scene->components[i].type == COMPONENT_SET1_NAMETAG)
        {
            newComponentNameTag = realloc((componentNameTagT*)scene->components[i].data,sizeof(struct componentNameTagT)*scene->maxEntities);
            if(newComponentNameTag == NULL){
                //write the error to the logfile
                writeToLog("Error in Function allocateMoreMemoryForComponents() - Could not allocate more memory for name tag components! ","runlog.txt");
                //mark that memory allocation failed
                scene->memallocFailed = 1;
                return 0;
            }
            //initialize the component for the new entities
            for(j=scene->numEntities;j<scene->maxEntities;++j){
                newComponentNameTag[j].name = NULL;
            }
            //point the data pointer to the new data
            scene->components[i].data = newComponentNameTag;
        }
        //// COLLISION COMPONENT
        else if(scene->components[i].type == COMPONENT_SET1_COLLISION)
        {
            newComponentCollision = realloc((componentCollisionT*)scene->components[i].data,sizeof(struct componentCollisionT)*scene->maxEntities);
            if(newComponentCollision == NULL){
                //write the error to the logfile
                writeToLog("Error in Function allocateMoreMemoryForComponents() - Could not allocate more memory for collision components! ","runlog.txt");
                //mark that memory allocation failed
                scene->memallocFailed = 1;
                return 0;
            }
            //initialize the component for the new entities
            for(j=scene->numEntities;j<scene->maxEntities;++j){
                newComponentCollision[j].collisionType = COLLISIONTYPE_DEACTIVATED;
                setupRect(&newComponentCollision[j].rect,0,0,5,5);
            }
            //point the data pointer to the new data
            scene->components[i].data = newComponentCollision;
        }
        //If the component has not yet been added to the function
        else{
            //log the error to file
            ecsGetComponentName(scene->components[i].type,componentName);
            sprintf(msg,"\nError in Function allocateMoreMemoryForComponents() ComponentType:%s with bit:%d has not been added here for more memory allocation!!\n",componentName,ecsGetComponentBit(scene->components[i].type));
            writeToLog(msg,"runlog.txt");
            //also write the error to console
            fprintf(stderr,"%s",msg);
            //flag that memory allocation has failed
            scene->memallocFailed = 1;
            return 0;
        }
    }
    return 1;
}

void ecsSceneRemoveEntityFromScene(sceneT* scene, Uint32 entityID)
{
    Uint32 tmpID;
    //if the passed entity manager is NULL
    if(scene == NULL){
        //log it as an error
        writeToLog("Error in Function ecsEntityManagerRemoveEntityFromScene() - sceneT *scene is NULL!","runlog.txt");
        //exit the function
        return;
    }

    //Failsafe, if there are no entities, simply return
    if(scene->numEntities<=0){
        return;
    }

    //if there is only one entity, or if the entity to remove is the last one
    if(scene->numEntities==1 || entityID == scene->numEntities){
        //printf("entity is the last one, or the only one... No entity index component moving is needed!");
        //Decrease number of entities with 1
        //The entity will be overwritten the next time a new entity is created.
        scene->numEntities--;
    }
    //if there are multiple entities
    else{
        //copy the entity ID from the entity to be removed
        tmpID = scene->entities[entityID].id;

        //move the entity last in the array to the slot of the entity that was removed.
        scene->entities[entityID] = scene->entities[scene->numEntities-1];

        //change its entity ID to its new slot position.
        scene->entities[entityID].id = tmpID;

        //Do the same in all components as well.
        moveEntityIndexInComponentsAfterEntityRemove(scene,entityID);

        //decrease number of entities.
        scene->numEntities--;
    }
}

static void moveEntityIndexInComponentsAfterEntityRemove(sceneT* scene, Uint32 entityID)
{
    //this is called inside ecsSceneRemoveEntityFromScene()
    //where a healthy check of if(scene == NULL ) just was performed, so we skip it here.
    Uint32 i=0;
    char msg[512];
    char componentName[200];
    //pointers to the component types
    componentPositionT *componentPosition = NULL;
    componentVelocityT *componentVelocity = NULL;
    componentInputKeyboardT *componentInputKeyboard = NULL;
    componentInputMouseT *componentInputMouse = NULL;
    componentRender2DT *componentRender2D = NULL;
    componentNameTagT *componentNameTag = NULL;
    componentCollisionT *componentCollision = NULL;

    //loop through the components and move the component last in the array to the entity position that was removed
    for(i=0;i<scene->numComponents;++i)
    {
        if(scene->components[i].type == COMPONENT_SET1_POSITION){
            componentPosition = (componentPositionT*)scene->components[i].data;
            componentPosition[entityID] = componentPosition[scene->numEntities-1];
        }
        else if(scene->components[i].type == COMPONENT_SET1_VELOCITY){
            componentVelocity = (componentVelocityT*)scene->components[i].data;
            componentVelocity[entityID] = componentVelocity[scene->numEntities-1];
        }
        else if(scene->components[i].type == COMPONENT_SET1_KEYBOARD){
            componentInputKeyboard =(componentInputKeyboardT*)scene->components[i].data;
            componentInputKeyboard[entityID] = componentInputKeyboard[scene->numEntities-1];
        }
        else if(scene->components[i].type == COMPONENT_SET1_MOUSE){
            componentInputMouse =(componentInputMouseT*)scene->components[i].data;
            componentInputMouse[entityID] = componentInputMouse[scene->numEntities-1];
        }
        else if(scene->components[i].type == COMPONENT_SET1_RENDER2D){
            componentRender2D = (componentRender2DT*)scene->components[i].data;
            componentRender2D[entityID] = componentRender2D[scene->numEntities-1];
        }
        else if(scene->components[i].type == COMPONENT_SET1_NAMETAG){
            componentNameTag = (componentNameTagT*)scene->components[i].data;
            componentNameTag[entityID] = componentNameTag[scene->numEntities-1];
        }
        else if(scene->components[i].type == COMPONENT_SET1_COLLISION){
            componentCollision = (componentCollisionT*)scene->components[i].data;
            componentCollision[entityID] = componentCollision[scene->numEntities-1];
        }
        //if the component is not yet handled by the function
        else{
            ecsGetComponentName(scene->components[i].type,componentName);
            //log the error
            sprintf(msg,"\nError in Function moveEntityIndexInComponentsAfterEntityRemove() - ComponentType:%s with bit:%d is missing. Add it here!\n",componentName,ecsGetComponentBit(scene->components[i].type));
            writeToLog(msg,"runlog.txt");
            //log error to console as well.
            fprintf(stderr,"%s",msg);
        }
    }
}

Uint32 ecsSceneGetNumEntities(sceneT *scene)
{
    if(scene == NULL){
        return -1;
    }
    return scene->numEntities;
}

Uint32 ecsSceneGetComponentIndex(sceneT *scene,Uint32 componentFlag)
{
    Uint32 i=0;
    //loop through all the components in the scene
    for(i=0;i<scene->numComponents;++i)
    {
        //if the component is found
        if(scene->components[i].type & componentFlag)
        {
            //return its index
            return i;
        }
    }
    //if the component is not found, return -1
    return -1;
}

void *ecsSceneGetComponent(sceneT *scene,Uint32 componentFlag)
{
    Uint32 i=0;
    //loop through all the components in the scene
    for(i=0;i<scene->numComponents;++i)
    {
        //if the component is found
        if(scene->components[i].type & componentFlag)
        {
            //return its index
            return scene->components[i].data;
        }
    }
    //if the component is not found, return NULL
    return NULL;
}

int ecsSceneAddSystemToScene(sceneT *scene,systemTypeE systemType)
{
    systemT *newSystems = NULL;
    char systemName[200];
    char msg[512];

    if(scene == NULL){
        //log it as an error
        writeToLog("Error in Function ecsSceneAddSystemToScene() - sceneT *scene is NULL!","runlog.txt");
        //exit the function
        return 0;
    }
    //if we are on the last system
    if(scene->numSystems>=scene->maxSystems){
        //increase max systems
        scene->maxSystems+=5;

        //re-allocate memory for systems
        newSystems = realloc(scene->systems,sizeof(systemT)*scene->maxSystems);

        //if memory allocation failed
        if(newSystems == NULL){
            writeToLog("Error in Function ecsSceneAddSystemToScene() - Could not re-allocate memory for new systems!","runlog.txt");

            //undo number of systems
            scene->maxSystems-=5;

            //return 0
            return 0;
        }

        //point the systems pointer to the new allocated memory
        scene->systems = newSystems;
    }
    //// MOVE SYSTEM
    if(systemType == SYSTEM_MOVE)
    {
        scene->systems[scene->numSystems].type = SYSTEM_MOVE;
        scene->systems[scene->numSystems].init = systemMoveInit;
        scene->systems[scene->numSystems].updateEntity = systemMoveUpdateEntity;
        scene->systems[scene->numSystems].update = systemMoveUpdate;
        scene->systems[scene->numSystems].free = systemFreeMoveSystem;
        scene->numSystems++;
    }
    //// INPUT SYSTEM
    else if(systemType == SYSTEM_INPUT){
        scene->systems[scene->numSystems].type = SYSTEM_INPUT;
        scene->systems[scene->numSystems].init = systemInputInit;
        scene->systems[scene->numSystems].updateEntity = systemInputUpdateEntity;
        scene->systems[scene->numSystems].update = systemInputUpdate;
        scene->systems[scene->numSystems].free = systemFreeInputSystem;
        scene->numSystems++;
        scene->sceneHasInputSystem=1;
    }
    //// RENDER SYSTEM
    else if(systemType == SYSTEM_RENDER_ISOMETRIC_WORLD)
    {
        scene->systems[scene->numSystems].type = SYSTEM_RENDER_ISOMETRIC_WORLD;
        scene->systems[scene->numSystems].init = systemRenderIsoMetricWorldInit;
        scene->systems[scene->numSystems].update = systemRenderIsometricWorld;
        scene->systems[scene->numSystems].updateEntity = systemRenderIsometricWorldSortEntity;
        scene->systems[scene->numSystems].free = systemFreeRenderIsoWorldSystem;
        scene->numSystems++;
    }
    //// CONTROL ISOMETRIC WORLD SYSTEM
    else if(systemType == SYSTEM_CONTROL_ISOMETRIC_WORLD){
        scene->systems[scene->numSystems].type = SYSTEM_CONTROL_ISOMETRIC_WORLD;
        scene->systems[scene->numSystems].init = systemControlIsoMetricWorldInit;
        scene->systems[scene->numSystems].update = systemControlIsometricWorld;
        scene->systems[scene->numSystems].updateEntity = NULL;
        scene->systems[scene->numSystems].free = systemFreeControlIsoWorldSystem;
        scene->numSystems++;
    }
    //// CONTROL ENTITY SYSTEM
    else if(systemType == SYSTEM_CONTROL_ENTITY){
        scene->systems[scene->numSystems].type = SYSTEM_CONTROL_ENTITY;
        scene->systems[scene->numSystems].init = systemControlEntityInit;
        scene->systems[scene->numSystems].update = systemControlEntity;
        scene->systems[scene->numSystems].updateEntity = NULL;
        scene->systems[scene->numSystems].free = systemFreeControlEntitySystem;
        scene->numSystems++;
    }
    //// CONTROL ENTITY SYSTEM
    else if(systemType == SYSTEM_COLLISION){
        scene->systems[scene->numSystems].type = SYSTEM_COLLISION;
        scene->systems[scene->numSystems].init = systemCollisionInit;
        scene->systems[scene->numSystems].update = systemCollisionUpdate;
        scene->systems[scene->numSystems].updateEntity = systemCollisionUpdateEntity;
        scene->systems[scene->numSystems].free = systemCollisionFree;
        scene->numSystems++;
    }
    //// UNKNOWN SYSTEM
    else{
        //get the system name
        ecsGetSystemName(systemType,systemName);
        //log the warning to file
        sprintf(msg,"Warning in Function ecsSceneAddSystemToScene() - System:%s is not handled. Add it here!",systemName);
        writeToLog(msg,"runlog.txt");
        //also show warning in the console
        fprintf(stderr,"%s",msg);
    }
    return 1;
}

int ecsSceneInitSystemsInScene(sceneT *scene)
{
    Uint32 i = 0;
    //if the scene is NULL
    if(scene == NULL){
        //log it as an error
        writeToLog("Error in Function ecsInitAddedSystemsInScene() - sceneT *scene is NULL!","runlog.txt");
        //exit the function
        return 0;
    }
    //loop through all the systems and initialize them
    for(i=0;i<scene->numSystems;++i)
    {
        //Initialize the system
        if(scene->systems[i].init(scene) == 0)
        {
            scene->systemInitFailed = 1;
            //if the initialization failed, return 0
            return 0;
        }
    }
    //All systems were initialized successfully, return 1
    return 1;
}

void ecsSceneUpdateSystemsInScene(sceneT *scene)
{
    Uint32 i = 0;
    Uint32 j = 0;
    //No scene == NULL check here, this will be called each game loop
    //so we want it as fast as possible

    //run the systems that don't require working on an entity
    for(i=0;i<scene->numSystems;++i){
        //if the system has an update function
        if(scene->systems[i].update!=NULL){
            //update the system
            scene->systems[i].update(scene);
        }
    }

    //loop through all entities in the scene
    for(i=0;i<scene->numEntities;++i){
        //loop through all the systems in the scene
        for(j=0;j<scene->numSystems;++j){
            //if the system for updating entities exist
            if(scene->systems[j].updateEntity != NULL){
                //update the system, performing changes on the entities that match the required components
                scene->systems[j].updateEntity(i);
            }
        }
    }

    //if the component pointers were reallocated
    //the systems will now have updated the pointers
    if(scene->componentPointersRealloced == 1){
        //Reset the pointers reallocated flag to 0
        scene->componentPointersRealloced = 0;
    }
}

void ecsGetSystemName(systemTypeE systemType,char *name)
{
    if(systemType == SYSTEM_MOVE)
    {
        sprintf(name,"SYSTEM_MOVE");
    }
    else if(systemType == SYSTEM_INPUT){
        sprintf(name,"SYSTEM_INPUT");
    }
    else if(systemType == SYSTEM_RENDER_ISOMETRIC_WORLD)
    {
        sprintf(name,"SYSTEM_RENDER_ISOMETRIC_WORLD");
    }
    else if(systemType == SYSTEM_CONTROL_ISOMETRIC_WORLD)
    {
        sprintf(name,"SYSTEM_CONTROL_ISOMETRIC_WORLD");
    }
    else if(systemType == SYSTEM_CONTROL_ENTITY)
    {
        sprintf(name,"SYSTEM_CONTROL_ENTITY");
    }
    else{
        sprintf(name," ecsGetSystemName(id:%d) - System Not Named!",systemType);
    }
}

void ecsGetComponentName(componentTypeE componentType,char *name)
{
    if(componentType == COMPONENT_SET1_POSITION){
        sprintf(name,"COMPONENT_SET1_POSITION");
    }
    else if(componentType == COMPONENT_SET1_VELOCITY){
        sprintf(name,"COMPONENT_SET1_VELOCITY");
    }
    else if(componentType == COMPONENT_SET1_MOUSE){
        sprintf(name,"COMPONENT_SET1_MOUSE");
    }
    else if(componentType == COMPONENT_SET1_KEYBOARD){
        sprintf(name,"COMPONENT_SET1_KEYBOARD");
    }
    else if(componentType == COMPONENT_SET1_MOUSE){
        sprintf(name,"COMPONENT_SET1_MOUSE");
    }
    else if(componentType == COMPONENT_SET1_RENDER2D){
        sprintf(name,"COMPONENT_SET1_RENDER2D");
    }
    else if(componentType == COMPONENT_SET1_NAMETAG){
        sprintf(name,"COMPONENT_SET1_NAMETAG");
    }
    else if(componentType == COMPONENT_SET1_COLLISION){
        sprintf(name,"COMPONENT_SET1_COLLISION");
    }
    else{
        sprintf(name," getComponentName(bit:%d) - Component Not Named!",ecsGetComponentBit(componentType));
    }
}

int ecsGetComponentBit(componentTypeE value)
{
    char msg[512];
    char componentName[200];
    int position=1;
    Uint32 i=1;     // start on the first bit: 0000 0000 0000 0000 0000 0000 0000 000X

    //every next bit in the binary system will have the double value to the previous bit:
    //0000 0000 0000 0000 0000 0000 0000 0001 = 1   (position = 1)
    //0000 0000 0000 0000 0000 0000 0000 0010 = 2   (position = 2)
    //0000 0000 0000 0000 0000 0000 0000 0100 = 4   (position = 3)
    //0000 0000 0000 0000 0000 0000 0000 1000 = 8   (position = 4)
    //etc..

    //therefore we simply increase i with i*=2;

    while(1)
    {
        //if the component is found
        if(i==(Uint32)value){
            //return the current position
            return position;
        }

        //go to the next bit
        i*=2;

        //add the counter with 1
        position++;
        //if we've reached the last bit in the 32-bit integer)
        if(position>=32){

            //the component was not found, and is probably not correctly initialized
            //so give a warning about that
            ecsGetComponentName(value,componentName);
            sprintf(msg,"Warning in Function ecsGetComponentBit() - componentType:%s is not correctly initialized",componentName);
            writeToLog(msg,"runlog.txt");

            //break out from the while loop
            break;
        }
    }
    return -1;
}

void ecsSceneSetCPUDelay(sceneT *scene, int value)
{
    if(scene == NULL)
    {
        //write error to the log file and exit out of the function
        writeToLog("Error in Function ecsSceneSetCPUDelay() - parameter 'sceneT *scene' is NULL","runlog.txt");
        return;
    }
    if(value == 1){
        scene->consumeLessCPU = 1;
    }
    else if(value == 0){
        scene->consumeLessCPU = 0;
    }
}

