/*
 *  The isometric render world system is special since it has to draw entities in order according to their Y
 *  position (height) in Cartesian coordinates and also drawn them correctly behind and in-front of the isometric
 *  tiles.
 *
 *  To do this, we will first create a sorted list of on-screen entities based on their height in Cartesian
 *  coordinates, while also calculating the row on the map which they are standing on.
 *  Secondly while drawing the map, we will keep track of which row on the map that is being drawn, and draw the
 *  entities that are standing on that row. The sorted list of entities will function like a stack, where we take
 *  the top entity of the stack and move our way down. This will simply be an integer that is iterating through
 *  the sorted list. Since the list is in the correct order we only have to take out the top entities that stand
 *  on the row that's being drawn.
 *
 */
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include "system.h"
#include "system_render_iso_world.h"
#include "system_control_entity.h"
#include "../../logger.h"
#include "../../deltatimer.h"
#include "../Scene/scene.h"
#include "../Component/component.h"
#include "../../texture.h"
#include "../../IsoEngine/isoEngine.h"
#include "../../renderer.h"

//define a mask for the render isometric system. It requires a position and a render2D component.
//it works on SET1 components, so we mark that as well in the define name
#define SYSTEM_RENDER_ISO_MASK_SET1 (COMPONENT_SET1_POSITION | COMPONENT_SET1_RENDER2D)
#define NUM_INITIAL_ONSCREEN_ENTITIES_PER_LAYER   10

//local global variable for system failure
static int systemFailedToInitialize = 1;

//local global variable to used flag when to allocate more memory for the on-screen entities
//the variable points to the layer that should be reallocated
static int allocateMoreOnScreenEntities = -1;

//local global pointers to the data
static componentPositionT *posComponents = NULL;
static componentRender2DT *render2DComponents = NULL;
static isoEngineT *isoEngine = NULL;
static sceneT *scn = NULL;

static Uint32 playerEntityIndex = -1;

//local struct for an on-screen entity
typedef struct entityOnScreenPosT
{
    int isoX,isoY;          //entity isometric x,y position
    int cartesianYPos;      //entity Cartesian y position
    int row;                //which row the entity is standing on
    Uint32 entityID;        //entity ID to draw
}entityOnScreenPosT;

//Local struct to store entities that will be drawn on the screen
typedef struct entitiesOnScreenT
{
    entityOnScreenPosT *entityList;     //list with entities on the screen
    Uint32 maxEntities;                 //current max entities on screen
    Uint32 numEntities;                 //number entities on screen
    Uint32 currentEntityToDraw;         //current index in the sortedIndexList we want to draw
}entitiesOnScreenT;

//local global pointer to entities on screen
static entitiesOnScreenT *entitiesOnScreen = NULL;

//function prototypes
static void systemRenderIsometricObject(int entity);
static void insertionSortOnScreenEntities(entitiesOnScreenT *entities,int layer,entityOnScreenPosT *entity);
static int binarySearchFindOnScreenEntityInsertIndex(entitiesOnScreenT *entities,int layer,entityOnScreenPosT *entity);

static void updateComponentPointers()
{
    if(scn == NULL){
        return;
    }
    //ERROR handling is done in the scene.c file for the components. If a realloc fail, the system will shut down there.

    //get the render2D component
    render2DComponents = (componentRender2DT*)ecsSceneGetComponent(scn,COMPONENT_SET1_RENDER2D);

    // Get the position component
    posComponents = (componentPositionT*)ecsSceneGetComponent(scn,COMPONENT_SET1_POSITION);
}

int systemRenderIsoMetricWorldInit(void *scene)
{
    int i=0,j=0;
    char msg[512];
    systemFailedToInitialize=0;
    componentNameTagT *nameTagComponents = NULL;
    playerEntityIndex = -1;
    writeToLog("Initializing Isometric World Render System...","systemlog.txt");
    //if the passed entity manager is NULL
    if(scene == NULL){
        //log it as an error
        writeToLog("Error in Function systemRenderIsoInit() - Render system failed to initialize: *scene is NULL!","systemlog.txt");

        //mark that the system failed to initialize
        systemFailedToInitialize = 1;
        //exit the function
        return 0;
    }
    //typecast the void *scene to a sceneT* pointer
    scn = (sceneT*)scene;

    //if the scene has the isometric engine attached to it
    if(scn->isoEngine != NULL){
        //set the local global isoEngine pointer to point to the isometric engine.
        isoEngine = scn->isoEngine;
    }
    //if the scene does not have the isometric engine
    else{
        //log it as an error
        writeToLog("Error in Function systemRenderIsoInit() - Render isometric world system failed to initialize: The scene does not have the isoEngine!","systemlog.txt");
        systemFailedToInitialize = 1;
        return 0;
    }

    //if the isometric map is NULL
    if(isoEngine->isoMap == NULL){
        //log it as an error
        writeToLog("Error in Function systemRenderIsoInit() - Render isometric world system failed to initialize: isoEngine->isoMap is NULL!","systemlog.txt");
        systemFailedToInitialize = 1;
        //return
        return 0;
    }

    //allocate memory for entities on screen struct
    entitiesOnScreen = malloc(sizeof(struct entitiesOnScreenT)*(isoEngine->isoMap->numLayers));
    if(entitiesOnScreen == NULL)
    {
        //log it as an error
        writeToLog("Error in Function systemRenderIsoInit() - Could not allocate memory for 'entitiesOnScreen' which is used for sorting the entities draw order.","systemlog.txt");
        systemFailedToInitialize = 1;
        //return out of the function
        return 0;
    }

    //allocate memory for the entities on the layers
    for(i=0;i<isoEngine->isoMap->numLayers;++i){
        entitiesOnScreen[i].entityList = malloc(sizeof(struct entityOnScreenPosT)*NUM_INITIAL_ONSCREEN_ENTITIES_PER_LAYER);
        if(entitiesOnScreen[i].entityList == NULL){
            //log it as an error
            sprintf(msg,"Error in Function systemRenderIsoInit() - Could not allocate memory for layer %d :entitiesOnScreen[%d]->entityList, which stores the on-screen entities.",i,i);
            writeToLog(msg,"systemlog.txt");
            systemFailedToInitialize = 1;
            //return out of the function
            return 0;
        }
        entitiesOnScreen[i].numEntities=0;
        entitiesOnScreen[i].maxEntities = NUM_INITIAL_ONSCREEN_ENTITIES_PER_LAYER;
        entitiesOnScreen[i].currentEntityToDraw = 0;

        for(j=0;j<NUM_INITIAL_ONSCREEN_ENTITIES_PER_LAYER;++j)
        {
            entitiesOnScreen[i].entityList[j].entityID = 0;
            entitiesOnScreen[i].entityList[j].cartesianYPos = 0;
            entitiesOnScreen[i].entityList[j].isoX = 0;
            entitiesOnScreen[i].entityList[j].isoY = 0;
            entitiesOnScreen[i].entityList[j].row = -1;
        }
    }

    //get the render2D component
    render2DComponents = (componentRender2DT*)ecsSceneGetComponent(scn,COMPONENT_SET1_RENDER2D);

    //if the scene does not have a render2D component
    if(render2DComponents == NULL){
        //log it as an error
        writeToLog("Error in Function systemRenderIsoInit() - Render isometric world  system failed to initialize: Scene does not have a COMPONENT_SET1_RENDER2D","systemlog.txt");
        systemFailedToInitialize = 1;
        return 0;
    }

    // Get the position component
    posComponents = (componentPositionT*)ecsSceneGetComponent(scn,COMPONENT_SET1_POSITION);

    ////if the scene does not have a position component
    if(posComponents == NULL){
        //log it as an error
        writeToLog("Error in Function systemRenderIsoInit() - Render isometric world  system failed to initialize: Position component data is not allocated!","systemlog.txt");
        systemFailedToInitialize = 1;
        return 0;
    }

    nameTagComponents = (componentNameTagT*)ecsSceneGetComponent(scn,COMPONENT_SET1_NAMETAG);
    if(nameTagComponents != NULL){

        //find the index of the player entity
        for(i=0;i<(Sint32)scn->numEntities;++i)
        {
            //make sure that the name is not a NULL pointer
            if(nameTagComponents[i].name!=NULL)
            {
                //if the name tag of the component is player1
                if(strcmp(nameTagComponents[i].name,"player1")==0)
                {
                    //set the isometric entity index
                    playerEntityIndex = i;
                }
            }
        }
    }

    //log that the isometric world render system was successfully initialized
    writeToLog("Initializing Isometric World Render System... DONE!","systemlog.txt");

    //return 1, successfully initialized the system
    return 1;
}

static void growEntitySortList(int layer)
{
    Uint32 i=0;
    char msg[512];
    entityOnScreenPosT *newEntityList = NULL;
    if(layer > scn->isoEngine->isoMap->numLayers){
        return;
    }
    if(entitiesOnScreen[layer].numEntities>=entitiesOnScreen[layer].maxEntities)
    {
        entitiesOnScreen[layer].maxEntities+=1000;
        newEntityList = realloc(&entitiesOnScreen[layer].entityList,sizeof(struct entityOnScreenPosT)*entitiesOnScreen[layer].maxEntities);

        if(newEntityList == NULL){
            //log it as an error
            sprintf(msg,"Error in Function insertionSortOnScreenEntities() - Could not reallocate more memory for sorting entities on layer %d!",layer);
            writeToLog(msg,"systemlog.txt");

            //flag that the system has failed to initialize properly, thus causing the game to stop rendering.
            systemFailedToInitialize = 1;

            //roll back number of max entities
            entitiesOnScreen[layer].maxEntities-=1000;
            //return out of the function
            return;
        }

        for(i=entitiesOnScreen[layer].numEntities;i<entitiesOnScreen[layer].maxEntities;++i)
        {
            entitiesOnScreen[layer].entityList[i].entityID = 0;
            entitiesOnScreen[layer].entityList[i].cartesianYPos = 0;
            entitiesOnScreen[layer].entityList[i].isoX = 0;
            entitiesOnScreen[layer].entityList[i].isoY = 0;
            entitiesOnScreen[layer].entityList[i].row = -1;
        }
        entitiesOnScreen[layer].entityList = newEntityList;
    }
}

static void systemRenderIsometricObject(int entity)
{
    point2DT point,tmpPoint;
    textureT *texture = render2DComponents[entity].texture;

    point.x = (posComponents[entity].x*isoEngine->zoomLevel)+ isoEngine->scrollX;
    point.y = (posComponents[entity].y*isoEngine->zoomLevel)+ isoEngine->scrollY;
    isoEngineConvert2dToIso(&point);
    point.x += posComponents[entity].xOffset*isoEngine->zoomLevel;
    point.y += posComponents[entity].yOffset*isoEngine->zoomLevel;

    //if the object is within the screen area
    if(point.x+texture->width*isoEngine->zoomLevel > 0 && point.x<WINDOW_WIDTH && point.y+texture->height*isoEngine->zoomLevel > 0 && point.y<WINDOW_HEIGHT){
        //find the tile position
        tmpPoint.y = posComponents[entity].x/(isoEngine->isoMap->tileSize*0.5);
        tmpPoint.x = posComponents[entity].y/(isoEngine->isoMap->tileSize*0.5);
        //convert the tmp point to cartesian coordinates
        isoEngineConvertIsoPoint2DToCartesian(isoEngine,&tmpPoint,&tmpPoint);
        //draw it on screen
        textureRenderXYClipScale(texture,point.x,point.y,&texture->cliprect,isoEngine->zoomLevel);
    }
}

void systemRenderIsometricWorld()
{
    int i,j,layer;
    int x,y;
    int tile = 4;
    int controlledEntity;
    int oldRowY = -1;

    point2DT point,currentRow;
    point2DT entityPos,entitySize;

    //if the system has failed to initialize
    if(systemFailedToInitialize==1){
        //return out of the function
        return;
    }

    //if the component pointers have been reallocated in the scene
    if(scn->componentPointersRealloced == 1){
        //update the local global component pointers
        updateComponentPointers();
    }

    if(isoEngine->gameMode == GAME_MODE_OBJECT_FOCUS){
        controlledEntity = systemControlEntityGetControlledEntity();
        //if an entity is being controlled
        if(controlledEntity>=0)
        {
            entityPos.x = posComponents[controlledEntity].x;
            entityPos.y = posComponents[controlledEntity].y;
            entitySize.x = render2DComponents[controlledEntity].texture->cliprect.w;
            entitySize.y = render2DComponents[controlledEntity].texture->cliprect.h;
            isoEngineCenterMap(isoEngine,&entityPos,&entitySize);
        }
    }

    if(allocateMoreOnScreenEntities!=1){
        growEntitySortList(allocateMoreOnScreenEntities);
        allocateMoreOnScreenEntities=-1;
    }

    SDL_SetRenderDrawColor(getRenderer(),0x3b,0x3b,0x3b,0x00);
    SDL_RenderClear(getRenderer());

    //initialize the current row
    currentRow.y = 0;
    currentRow.x = 0;

    //calculate the start position on the map to pick tiles from
    int startX = -6/isoEngine->zoomLevel +(isoEngine->mapScroll2Dpos.x/isoEngine->zoomLevel/isoEngine->isoMap->tileSize)*2;
    int startY = -20/isoEngine->zoomLevel + abs((isoEngine->mapScroll2Dpos.y/isoEngine->zoomLevel/isoEngine->isoMap->tileSize))*2;

    //calculate number of tiles in width and height that fit on the screen
    int numTilesInWidth = 8+ ((WINDOW_WIDTH/isoEngine->isoMap->tileSize)/isoEngine->zoomLevel);
    //NOTE: INCREASE 26+ -> 28+ TO DRAW THE TREE ON SCREEN
    int numTilesInHeight = 28+ ((WINDOW_HEIGHT/isoEngine->isoMap->tileSize)/isoEngine->zoomLevel)*2;

    //precalculate zoomLevel * tileSize, which gives us 2 less multiplications for each tile in the loop
    float zoomLevelTileSizePreCalc = isoEngine->zoomLevel *isoEngine->isoMap->tileSize;

    //if the map has a tile-set assigned to it
    if(isoEngine->isoMap->tileSet != NULL){
        //loop through the layers of the map
        for(layer=0;layer<isoEngine->isoMap->numLayers;++layer){
            //loop through the height
            for(i=startY;i<startY+numTilesInHeight;++i){
                //loop through the width
                for(j=startX;j<startX+numTilesInWidth;++j){

                    //only draw when both x & y is equal, if they are not, skip to the next tile
                    if((j&1) != (i&1)){
                        continue;
                    }
                    //get the x & y tile coordinates for the tile on the map
                    x = (i+j)/2;
                    y = (i-j)/2;

                    //get the tile from the map
                    //the isoMapGetTile function also checks that the tile is within the map
                    tile = isoMapGetTile(isoEngine->isoMap,x,y,layer);

                //DRAW ENTITIES HERE
                    //get the x & y point
                    point.x = ((x*zoomLevelTileSizePreCalc));
                    point.y = ((y*zoomLevelTileSizePreCalc));

                    //get current row we are drawing on
                    isoEngineConvertIsoPoint2DToCartesian(isoEngine,&point,&currentRow);

                    //if the map is drawing on a new row
                    if(oldRowY!=(int)currentRow.y){
                        //update the old row value to a new
                        oldRowY = (int)currentRow.y;

                        if(entitiesOnScreen[layer].numEntities>0)
                        {
                            while(entitiesOnScreen[layer].entityList[entitiesOnScreen[layer].currentEntityToDraw].row == (int)currentRow.y/(isoEngine->isoMap->tileSize/2))
                            {
                                systemRenderIsometricObject(entitiesOnScreen[layer].entityList[entitiesOnScreen[layer].currentEntityToDraw].entityID);
                                entitiesOnScreen[layer].currentEntityToDraw++;
                            }
                        }

                    }

                    //if the tile is valid
                    if(tile>=0){

                        point.x = ((x*zoomLevelTileSizePreCalc) + isoEngine->scrollX);
                        point.y = ((y*zoomLevelTileSizePreCalc) + isoEngine->scrollY);

                        isoEngineConvert2dToIso(&point);

                        //only draw the tile if it's visible on screen
                        if(point.x + isoEngine->isoMap->tileSize*isoEngine->zoomLevel >=-128 && point.x < WINDOW_WIDTH
                        && point.y + isoEngine->isoMap->tileSize*isoEngine->zoomLevel >=-128 && point.y < WINDOW_HEIGHT)
                        {
                            textureRenderXYClipScale(isoEngine->isoMap->tileSet->tilesTex,(int)point.x,(int)point.y,
                                             &isoEngine->isoMap->tileSet->tileClipRects[tile],isoEngine->zoomLevel);
                        }
                    }
                }
            }
        }
    }

//    systemRenderIsometricObject(playerEntityIndex);

    isoEngineDrawIsoMouse(isoEngine);

    if(isoEngine->lastTileClicked!=-1){
        textureRenderXYClip(isoEngine->isoMap->tileSet->tilesTex,0,0,
                            &isoEngine->isoMap->tileSet->tileClipRects[isoEngine->lastTileClicked]);
    }
    SDL_RenderPresent(getRenderer());
}
//NOTE/ TODO: Make sure this works with layers!
void systemRenderIsometricWorldSortEntity(Uint32 entity)
{
    point2DT point,tmpPoint;
    entityOnScreenPosT newEntity;
    Uint32 i = 0,j = 0;
    char msg[512];
    entityOnScreenPosT *testNewEntitiesList=NULL;

    //if the system has failed to initialize
    if(systemFailedToInitialize==1){
        //return out of the function
        return;
    }

    //if it's the first entity
    if(entity == 0){
        //reset entity list
        for(i=0;i<(Uint32)isoEngine->isoMap->numLayers;++i){
            entitiesOnScreen[i].numEntities = 0;
            entitiesOnScreen[i].currentEntityToDraw = 0;
        }
    }

    //if the entity has the position and render2D component
    if(scn->entities[entity].componentSet1 & SYSTEM_RENDER_ISO_MASK_SET1)
    {
        //get the object position in the world
        point.x = posComponents[entity].x*isoEngine->zoomLevel + isoEngine->scrollX;
        point.y = posComponents[entity].y*isoEngine->zoomLevel + isoEngine->scrollY;

        //convert the position to isometric coordinates
        isoEngineConvert2dToIso(&point);

        //adjust the object position with its offset
        point.x += posComponents[entity].xOffset*isoEngine->zoomLevel;
        point.y += posComponents[entity].yOffset*isoEngine->zoomLevel;

        //if the object is within the screen area
        if(point.x+render2DComponents[entity].texture->width*isoEngine->zoomLevel > 0
        && point.x<WINDOW_WIDTH
        && point.y+render2DComponents[entity].texture->height*isoEngine->zoomLevel > 0
        && point.y<WINDOW_HEIGHT)
        {
        //Step 1: Store the entity height
            //Get the position of the entity
            tmpPoint.x = posComponents[entity].x*isoEngine->zoomLevel;
            tmpPoint.y = posComponents[entity].y*isoEngine->zoomLevel;

            //convert the point so we can track its height in the isometric world
            isoEngineConvertIsoPoint2DToCartesian(isoEngine,&tmpPoint,&tmpPoint);
            //store the entity y position
            newEntity.cartesianYPos = tmpPoint.y;
        //Step 2: Store the entity tile height
            //Get the tile position for the object
            tmpPoint.y = posComponents[entity].y/(isoEngine->isoMap->tileSize*0.5);   //*0.5 is the same as /2
            tmpPoint.x = posComponents[entity].x/(isoEngine->isoMap->tileSize*0.5);   //*0.5 is the same as /2
            //convert the point so we can track its row in the isometric world
            isoEngineConvertIsoPoint2DToCartesian(isoEngine,&tmpPoint,&tmpPoint);
            //Store the row for the object, make sure to multiply with the zoom level
            //so it will work when the game screen is zoomed in too
            newEntity.row = (int)tmpPoint.y*isoEngine->zoomLevel;
        //Step 3: Store the isometric position in the world
            newEntity.isoX = point.x;
            newEntity.isoY = point.y;
        //step 4: Store the entity ID
            newEntity.entityID = entity;
        //Step 5: Perform binary insert sort for the entity to the entities list
            if(render2DComponents[entity].layer>=0){
                //if max number of on-screen entities has been reached
                if(entitiesOnScreen[render2DComponents[entity].layer].numEntities >= entitiesOnScreen[render2DComponents[entity].layer].maxEntities)
                {
                    //flag that its time to allocate more memory for the on-screen entities
                    allocateMoreOnScreenEntities = render2DComponents[entity].layer;
                    return;
                }
                insertionSortOnScreenEntities(entitiesOnScreen,render2DComponents[entity].layer,&newEntity);
            }
        }
    }
}
void systemFreeRenderIsoWorldSystem()
{
    int i=0;
    //if memory has been allocated for the entities on screen
    if(entitiesOnScreen!=NULL)
    {
        //loop through each layer
        for(i=0;i<isoEngine->isoMap->numLayers;++i)
        {
            //free the allocated entities for each layer
            free(entitiesOnScreen[i].entityList);
        }
        //free the allocated memory for entities on screen
        free(entitiesOnScreen);
    }
}


static void insertionSortOnScreenEntities(entitiesOnScreenT *entities,int layer,entityOnScreenPosT *entity)
{
    int insertIndex =-1;

    //if there are no entities in the array
    if(entities[layer].numEntities==0){
        //insert the entity to index 0
        entities[layer].entityList[0] = *entity;
        //increase counter for number of entities in the array
        entities[layer].numEntities+=1;
        //return out of the function
        return;
    }

    //Get the index where the next entity should be inserted
    insertIndex = binarySearchFindOnScreenEntityInsertIndex(entities,layer,entity);
    if(insertIndex!=-1){

        //If it's the first index
        if(insertIndex==0){
            //move everything one step forward in the entities array
            memmove(entities[layer].entityList+1,entities[layer].entityList,sizeof(entityOnScreenPosT)*entities[layer].numEntities+1);
        }
        else{
            //move everything from the insertIndex forward
            memmove(entities[layer].entityList+insertIndex,entities[layer].entityList+insertIndex-1,sizeof(entityOnScreenPosT)*entities[layer].numEntities+1);
        }
        //write the entity the index in the array
        entities[layer].entityList[insertIndex] = *entity;
        entities[layer].numEntities++;
    }
}

//The binary search only works on a sorted entities
static int binarySearchFindOnScreenEntityInsertIndex(entitiesOnScreenT *entities,int layer,entityOnScreenPosT *entity)
{
    int first = 0;                      //initialize the first index to 0
    int last = entities[layer].numEntities-1; //initialize the last index to the number of entities in the entities list
    int search = (first+last)/2;        //initialize the first search position to the middle of the entities list

    //as long as the first index is less or equal to the last
    while (first <= last)
    {
        //check if we have found the position to insert the entity into
        if (
            //If the search index cartesianYPos is identical to the entity cartesianYPos
            entities[layer].entityList[search].cartesianYPos == entity->cartesianYPos
            //or if the search index cartesianYPos is less than the entity cartesianYPos
            //and the next search index in the entities is larger than the entity cartesianYPos
            || (entities[layer].entityList[search].cartesianYPos < entity->cartesianYPos
            && entities[layer].entityList[search+1].cartesianYPos > entity->cartesianYPos
            && entities[layer].numEntities>=1)
            //or if the search index -1 is less than the number and the search index is larger than the number
            || (entities[layer].entityList[search-1].cartesianYPos < entity->cartesianYPos
            && entities[layer].entityList[search].cartesianYPos > entity->cartesianYPos
            && entities[layer].numEntities>=1)
            //or if the search point has reached the first or the last index in the entities
            || search == first || search == last)
        {
            //if the search index cartesianYPos is less than the entity cartesianYPos
            if(entities[layer].entityList[search].cartesianYPos < entity->cartesianYPos){
                //special case for when first and last position are next to one another
                if(first+1 == last){
                    //check if the last index cartesianYPos is lower than the entity cartesianYPos
                    if(entities[layer].entityList[search+1].cartesianYPos < entity->cartesianYPos){
                        //if so, the index to insert the entity into is at the end
                        return search+2;
                    }
                }
                //return the search index +1
                return search+1;
            }
            //if not, return the search index
            else{
                return search;
            }

        }
        //if the entity cartesianYPos is less than the requested entity cartesianYPos
        else if (entities[layer].entityList[search].cartesianYPos < entity->cartesianYPos)
        {
            //set the first index to the search position +1
            first = search + 1;
        }
        //otherwise if the search number is larger than the requested number
        else
        {
            //set the last position to the search -1
            last = search - 1;
        }
        //move the the search position
        search = (first + last)/2;
    }
    return -1;
}
