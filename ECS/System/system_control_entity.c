#include <stdio.h>
#include <string.h>
#include "system.h"
#include "system_control_iso_world.h"
#include "../../logger.h"
#include "../Scene/scene.h"
#include "../Component/component.h"
#include "../../IsoEngine/isoEngine.h"

//define a mask for the control isometric system. It requires the keyboard component
//it works on SET1 components, so we mark that as well in the define
#define SYSTEM_CONTROL_ENTITY_MASK_SET1 (COMPONENT_SET1_KEYBOARD | COMPONENT_SET1_NAMETAG | COMPONENT_SET1_VELOCITY)

//function prototypes
static void mapKeyboardControl(sceneT *scene,int *key,char *action);
static void mapMouseControl(sceneT *scene,int *mouseAction,char *action);
static void setKeysAndMouseControls(sceneT *scene);

//local global variable for system failure
static int systemFailedToInitialize = 1;

//local global pointer to the components data
sceneT *scn = NULL;
static componentInputKeyboardT *keyboardInputComponents = NULL;
static componentInputMouseT *mouseInputComponents = NULL;
static componentNameTagT *nameTagComponents = NULL;
static componentVelocityT *velocityComponents = NULL;
static componentRender2DT *renderComponents = NULL;

//list of the common keys
static int keyMoveUp = -1;
static int keyMoveDown = -1;
static int keyMoveLeft = -1;
static int keyMoveRight = -1;
static int mouseWheel = -1;
static int mouseLeftClick = -1;

//we assume that controllable entity ID:s will be less than: 2,147,483,647 (max signed int32 value)
static Sint32 selectedEntityToControl = -1;
static Sint32 playerEntityID = -1;

static void updateComponentPointers()
{
    if(scn == NULL){
        return;
    }
    //ERROR handling is done in the scene.c file for the components. If a realloc fail, the system will shut down there.

    //get the pointer to the keyboard input components
    keyboardInputComponents = (componentInputKeyboardT*)ecsSceneGetComponent(scn,COMPONENT_SET1_KEYBOARD);
    //get the mouse input component
    mouseInputComponents = (componentInputMouseT*)ecsSceneGetComponent(scn,COMPONENT_SET1_MOUSE);
    //get the velocity components
    velocityComponents = (componentVelocityT*)ecsSceneGetComponent(scn,COMPONENT_SET1_VELOCITY);
    //get the pointer to the name tag components
    nameTagComponents = (componentNameTagT*)ecsSceneGetComponent(scn,COMPONENT_SET1_NAMETAG);
    //get the pointer to the render2D components
    renderComponents = (componentRender2DT*)ecsSceneGetComponent(scn,COMPONENT_SET1_RENDER2D);
}

int systemControlEntityInit(void *scene)
{
    systemFailedToInitialize=0;

    writeToLog("Initializing Entity Control System...","runlog.txt");
    //if the passed entity manager is NULL
    if(scene == NULL){
        //log it as an error
        writeToLog("Error in Function systemControlEntityInit() - Entity Control system failed to initialize: *scene is NULL!","runlog.txt");

        //mark that the system failed to initialize
        systemFailedToInitialize = 1;
        //exit the function
        return 0;
    }
    //typecast the void *scene to a sceneT* pointer
    scn = (sceneT*)scene;
    //get the pointer to the keyboard input components
    keyboardInputComponents = (componentInputKeyboardT*)ecsSceneGetComponent(scn,COMPONENT_SET1_KEYBOARD);
    //if the scene does not have a kayboard
    if(keyboardInputComponents == NULL){
        //log it as an error
        writeToLog("Error in Function systemControlEntityInit() - Entity Control system failed to initialize: Scene does not have 'Keyboard input' component!","runlog.txt");
        systemFailedToInitialize = 1;
        return 0;
    }

    //get the mouse input component
    mouseInputComponents = (componentInputMouseT*)ecsSceneGetComponent(scn,COMPONENT_SET1_MOUSE);
    //if the scene does not have the mouse component
    if(mouseInputComponents == NULL){
        //log it as an error
        writeToLog("Error in Function systemControlEntityInit() - Entity World Control system failed to initialize: Scene does not have 'Mouse input' component!","runlog.txt");
        systemFailedToInitialize = 1;
        return 0;
    }

    //get the velocity components
    velocityComponents = (componentVelocityT*)ecsSceneGetComponent(scn,COMPONENT_SET1_VELOCITY);
    //if the scene does not have velocity components
    if(velocityComponents == NULL){
        //log it as an error
        writeToLog("Error in Function systemControlEntityInit() - Entity World Control system failed to initialize: Scene does not have 'velocity' component!","runlog.txt");
        systemFailedToInitialize = 1;
        return 0;
    }
    //get the pointer to the name tag components
    nameTagComponents = (componentNameTagT*)ecsSceneGetComponent(scn,COMPONENT_SET1_NAMETAG);
    if(nameTagComponents == NULL){
        //log it as an error
        writeToLog("Error in Function systemControlEntityInit() - Entity Control system failed to initialize: Scene does not have a 'name tag' component!","runlog.txt");
        systemFailedToInitialize = 1;
        return 0;
    }
    //get the pointer to the render2D components
    renderComponents = (componentRender2DT*)ecsSceneGetComponent(scn,COMPONENT_SET1_RENDER2D);
    if(renderComponents == NULL){
        //log it as an error
        writeToLog("Error in Function systemControlEntityInit() - Entity Control system failed to initialize: Scene does not have 'render2D' component!","runlog.txt");
        systemFailedToInitialize = 1;
        return 0;
    }
    //get the player ID (if it exist)
    playerEntityID = componentNameTagGetEntityIDFromEntityByName(nameTagComponents,"player1",scn->numEntities);

    //log that the isomeric world control system was successfully initialized
    writeToLog("Initializing Entity Control System... DONE!","runlog.txt");

    //return 1, successfully initialized the system
    return 1;
}

void systemControlEntity()
{
    int controlledEntityIsPlayer1 = 0;
    SDL_Rect tmpRect;

    //if the system has failed to initialize
    if(systemFailedToInitialize==1 || selectedEntityToControl==-1){
        //return out of the function
        return;
    }
    //if the component pointers have been reallocated in the scene
    if(scn->componentPointersRealloced == 1){
        //update the local global component pointers
        updateComponentPointers();
    }

    //if the entity being controlled is the player
    if(selectedEntityToControl == playerEntityID)
    {
        //flag that this is so
        controlledEntityIsPlayer1 = 1;
    }


    //if the mouse wheel is scrolling up
    if(mouseWheel !=-1 && mouseInputComponents[selectedEntityToControl].actions[mouseWheel].state == COMPONENT_INPUTMOUSE_STATE_MOUSEWHEEL_UP){
        //do something
    }
    //if the mouse wheel is scrolling down
    if(mouseWheel !=-1 && mouseInputComponents[selectedEntityToControl].actions[mouseWheel].state == COMPONENT_INPUTMOUSE_STATE_MOUSEWHEEL_DOWN){
        //do something
    }

    //if the left mouse button has just been pressed
    if(mouseLeftClick !=-1 && mouseInputComponents[selectedEntityToControl].actions[mouseLeftClick].state == COMPONENT_INPUTMOUSE_STATE_RELEASED
    && mouseInputComponents[selectedEntityToControl].actions[mouseLeftClick].oldState == COMPONENT_INPUTMOUSE_STATE_PRESSED){
        //do something
    }

    ///KEYBOARD CONTROLS

    //if action keys: right & down is pressed
    if(keyMoveRight !=-1 && keyboardInputComponents[selectedEntityToControl].actions[keyMoveRight].state == COMPONENT_INPUTKEYBOARD_STATE_PRESSED
    && keyMoveDown !=-1 && keyboardInputComponents[selectedEntityToControl].actions[keyMoveDown].state == COMPONENT_INPUTKEYBOARD_STATE_PRESSED)
    {
        velocityComponents[selectedEntityToControl].x = 160;
        if(controlledEntityIsPlayer1){
            //set the clip rectangle (facing right down)
            setupRect(&tmpRect,280,0,70,102);
            componentRender2DSetClipRect(renderComponents,playerEntityID,&tmpRect);
        }
    }
    //if action keys: right & up is pressed
    else if(keyMoveRight !=-1 && keyboardInputComponents[selectedEntityToControl].actions[keyMoveRight].state == COMPONENT_INPUTKEYBOARD_STATE_PRESSED
    && keyMoveUp !=-1 && keyboardInputComponents[selectedEntityToControl].actions[keyMoveUp].state == COMPONENT_INPUTKEYBOARD_STATE_PRESSED)
    {
        velocityComponents[selectedEntityToControl].y = -160;
        if(controlledEntityIsPlayer1){
            //set the clip rectangle (facing right up)
            setupRect(&tmpRect,140,0,70,102);
            componentRender2DSetClipRect(renderComponents,playerEntityID,&tmpRect);
        }
    }
    //if action keys: left & up is pressed
    else if(keyMoveLeft !=-1 && keyboardInputComponents[selectedEntityToControl].actions[keyMoveLeft].state == COMPONENT_INPUTKEYBOARD_STATE_PRESSED
    && keyMoveUp !=-1 && keyboardInputComponents[selectedEntityToControl].actions[keyMoveUp].state == COMPONENT_INPUTKEYBOARD_STATE_PRESSED)
    {
        velocityComponents[selectedEntityToControl].x = -160;
        if(controlledEntityIsPlayer1){
            //set the clip rectangle (facing left up)
            setupRect(&tmpRect,0,0,70,102);
            componentRender2DSetClipRect(renderComponents,playerEntityID,&tmpRect);
        }
    }
    //if action keys: left & down is pressed
    else if(keyMoveLeft !=-1 && keyboardInputComponents[selectedEntityToControl].actions[keyMoveLeft].state == COMPONENT_INPUTKEYBOARD_STATE_PRESSED
    && keyMoveDown !=-1 && keyboardInputComponents[selectedEntityToControl].actions[keyMoveDown].state == COMPONENT_INPUTKEYBOARD_STATE_PRESSED)
    {
        velocityComponents[selectedEntityToControl].y = 160;
        if(controlledEntityIsPlayer1){
            //set the clip rectangle (facing left down)
            setupRect(&tmpRect,420,0,70,102);
            componentRender2DSetClipRect(renderComponents,playerEntityID,&tmpRect);
        }
    }
    //if the up key is pressed
    else if(keyMoveUp !=-1 && keyboardInputComponents[selectedEntityToControl].actions[keyMoveUp].state == COMPONENT_INPUTKEYBOARD_STATE_PRESSED)
    {
        velocityComponents[selectedEntityToControl].x = -160;
        velocityComponents[selectedEntityToControl].y = -160;
        if(controlledEntityIsPlayer1){
            //set the clip rectangle (facing up)
            setupRect(&tmpRect,70,0,70,102);
            componentRender2DSetClipRect(renderComponents,playerEntityID,&tmpRect);
        }
    }
    //if the down key is pressed
    else if(keyMoveDown !=-1 && keyboardInputComponents[selectedEntityToControl].actions[keyMoveDown].state == COMPONENT_INPUTKEYBOARD_STATE_PRESSED)
    {
        velocityComponents[selectedEntityToControl].x = 160;
        velocityComponents[selectedEntityToControl].y = 160;
        if(controlledEntityIsPlayer1){
            //set the clip rectangle (facing down)
            setupRect(&tmpRect,350,0,70,102);
            componentRender2DSetClipRect(renderComponents,playerEntityID,&tmpRect);
        }
    }
    //if the left key is pressed
    else if(keyMoveLeft !=-1 && keyboardInputComponents[selectedEntityToControl].actions[keyMoveLeft].state == COMPONENT_INPUTKEYBOARD_STATE_PRESSED)
    {
        velocityComponents[selectedEntityToControl].x = -80;
        velocityComponents[selectedEntityToControl].y = 80;
        if(controlledEntityIsPlayer1){
            //set the clip rectangle (facing left)
            setupRect(&tmpRect,490,0,70,102);
            componentRender2DSetClipRect(renderComponents,playerEntityID,&tmpRect);
        }
    }
    //if the right key is pressed
    else if(keyMoveRight !=-1 && keyboardInputComponents[selectedEntityToControl].actions[keyMoveRight].state == COMPONENT_INPUTKEYBOARD_STATE_PRESSED)
    {
        velocityComponents[selectedEntityToControl].x = 80;
        velocityComponents[selectedEntityToControl].y = -80;
        if(controlledEntityIsPlayer1){
            //set the clip rectangle (facing right)
            setupRect(&tmpRect,210,0,70,102);
            componentRender2DSetClipRect(renderComponents,playerEntityID,&tmpRect);
        }
    }

}

void systemControlEntitySetEntityToControlByNameTag(sceneT *scene,char *nameTag)
{
    char msg[512];
    Uint32 i=0;

    if(scene == NULL){
        //log it as an error
        writeToLog("Error in Function systemControlEntitySetEntityToControlByNameTag() - Parameter:'sceneT *scene' is NULL!","runlog.txt");
        return;
    }
    if(nameTag == NULL){
        //log it as an error
        writeToLog("Error in Function systemControlEntitySetEntityToControlByNameTag() - Parameter:'char *nameTag' is NULL!","runlog.txt");
        return;
    }
    //if the components have been reallocated in the scene
    if(scn->componentPointersRealloced == 1){
        //update the local global component pointers
        updateComponentPointers();
    }
    //loop through all entities
    for(i=0;i<scene->numEntities;++i){
        //if the entity with the name tag was found
        if(strcmp(nameTagComponents[i].name,nameTag)==0)
        {
            //set the index for the selected entity to i
            selectedEntityToControl = i;

            //set the keys and mouse controls
            setKeysAndMouseControls(scene);
            //return out of the function
            return;
        }
    }

    //If the entity was not found
    sprintf(msg,"Scene:%s, does not have an entity with the name tag:%s",scene->name,nameTag);
    //log it as an error
    writeToLog(msg,scene->name);
}

void systemControlEntitySetEntityToControlByID(sceneT *scene,Uint32 entityID)
{
    if(scene == NULL){
        //log it as an error
        writeToLog("Error in Function systemControlEntitySetEntityToControlByID() - Parameter:'sceneT *scene' is NULL!","runlog.txt");
        return;
    }
    if(entityID > scene->numEntities){
        //log it as an error
        writeToLog("Error in Function systemControlEntitySetEntityToControlByID() - entityID is > than number of entities","runlog.txt");
        return;
    }
    //if the component pointers have been reallocated in the scene
    if(scene->componentPointersRealloced == 1){
        //update the local global component pointers
        updateComponentPointers();
    }

    //set the entity to control
    selectedEntityToControl = entityID;
    setKeysAndMouseControls(scene);
}

static void mapKeyboardControl(sceneT *scene,int *key,char *action)
{
    char msg[512];
    int writeErrorWithIndex = 0;

    componentInputKeyboardT *keyboardInputComp = (componentInputKeyboardT*)ecsSceneGetComponent(scene,COMPONENT_SET1_KEYBOARD);
    if(keyboardInputComp == NULL){
        sprintf(msg,"Error in Entity Control System, function mapKeyboardControl() - Scene %s does not have the COMPONENT_SET1_KEYBOARD component!",scene->name);
        writeToLog(msg,"runlog.txt");
    }

    componentNameTagT *nameTagComp = (componentNameTagT*)ecsSceneGetComponent(scene,COMPONENT_SET1_NAMETAG);
    if(nameTagComp == NULL){
        writeErrorWithIndex = 1;
    }


    *key = componentInputKeyboardGetActionIndex(keyboardInputComp,selectedEntityToControl,action);
    if(*key == -1)
    {
        //log it as a warning
        if(writeErrorWithIndex==0){
            sprintf(msg,"INFO - Entity Control System, function mapKeyboardControl() - Key action: '%s' is not mapped for the entity:%s",action,nameTagComp[selectedEntityToControl].name);
            writeToLog(msg,"runlog.txt");
        }
        else{
            sprintf(msg,"INFO - Entity Control System, function mapKeyboardControl() - Key action: '%s' is not mapped for the entity:%d",action,selectedEntityToControl);
            writeToLog(msg,"runlog.txt");
        }
    }
}
static void mapMouseControl(sceneT *scene,int *mouseAction,char *action)
{
    char msg[512];
    int writeErrorWithIndex = 0;
    componentInputMouseT *inputMouseComp = (componentInputMouseT*)ecsSceneGetComponent(scene,COMPONENT_SET1_MOUSE);
    if(inputMouseComp == NULL)
    {
        sprintf(msg,"Error in Entity Control System, function mapMouseControl() - Scene %s, does not have the COMPONENT_SET1_MOUSE component! Mouse actions not available!",scene->name);
        writeToLog(msg,"runlog.txt");
        return;
    }
    componentNameTagT *nameTagComp = (componentNameTagT*)ecsSceneGetComponent(scene,COMPONENT_SET1_NAMETAG);
    if(nameTagComp == NULL){
        writeErrorWithIndex = 1;
    }

    *mouseAction = componentInputMouseGetActionIndex(mouseInputComponents,selectedEntityToControl,action);
    if(*mouseAction == -1)
    {
        //log it as a warning
        if(writeErrorWithIndex==0){
            sprintf(msg,"INFO - Entity Control System, function mapMouseControl() - Mouse action: '%s' is not mapped for the entity:%s",action,nameTagComp[selectedEntityToControl].name);
            writeToLog(msg,"runlog.txt");
        }
        else{
            sprintf(msg,"INFO - Entity Control System, function mapMouseControl() - Mouse action: '%s' is not mapped for the entity:%d",action,selectedEntityToControl);
            writeToLog(msg,"runlog.txt");
        }
    }
}

static void setKeysAndMouseControls(sceneT *scene)
{
    //get the key indexes
    mapKeyboardControl(scene,&keyMoveUp,"up");
    mapKeyboardControl(scene,&keyMoveDown,"down");
    mapKeyboardControl(scene,&keyMoveLeft,"left");
    mapKeyboardControl(scene,&keyMoveRight,"right");

    //get the mouse actions
    mapMouseControl(scene,&mouseWheel,"mouseWheel");
    mapMouseControl(scene,&mouseLeftClick,"leftButton");
}

Uint32 systemControlEntityGetControlledEntity()
{
    return selectedEntityToControl;
}

void systemFreeControlEntitySystem()
{
    //control entity system is not allocating anything, so we leave it empty
}

