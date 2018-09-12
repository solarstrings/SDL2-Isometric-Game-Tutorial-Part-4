#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "component_inputkeyboard.h"
#include "../../logger.h"
#include "../Scene/scene.h"

componentInputKeyboardT *componentNewInputKeyboard()
{
    int i=0;
    componentInputKeyboardT *newKeyboardComponent = malloc(sizeof(struct componentInputKeyboardT)*NUM_INITIAL_ENTITIES);
    if(newKeyboardComponent == NULL){
        writeToLog("Error in componentNewInputKeyboard() - Could not allocate memory for keyboard input component!","systemlog.txt");
        return NULL;
    }
    for(i=0;i<NUM_INITIAL_ENTITIES;++i){
        newKeyboardComponent[i].actions = NULL;                         //set actions to NULL
        newKeyboardComponent[i].numActions = 0;                         //number of actions
        newKeyboardComponent[i].maxActions = 0;                         //current max number of actions
        newKeyboardComponent[i].active = 0;                             //the component state is set to not active
    }

    return newKeyboardComponent;
}

void componentInputKeyboardAddAction(componentInputKeyboardT *componentInputKeyboard,int entity,char *name,SDL_Scancode scanCode)
{
    char msg[512];
    int actionIndex=-1;
    int len=0;
    int i=0;
    inputKeyboardActionT *newKeyboardAction = NULL;
    if(componentInputKeyboard == NULL){
        sprintf(msg,"Warning in componentInputKeyboardAddAction() - Parameter: 'componentInputKeyboardT *componentInputKeyboard' is NULL. Action:%s is not added!",name);
        writeToLog(msg,"systemlog.txt");
        return;
    }
    if(name == NULL){
        sprintf(msg,"Warning in componentInputKeyboardAddAction() - Parameter: 'char *name' is NULL. Action not added!");
        writeToLog(msg,"systemlog.txt");
        return;
    }

    //there are no actions
    if(componentInputKeyboard[entity].actions == NULL){
        componentInputKeyboard[entity].numActions = 0;
        componentInputKeyboard[entity].maxActions = 0;
        //allocate memory for five actions
        componentInputKeyboard[entity].maxActions+=5;

        componentInputKeyboard[entity].actions = malloc(sizeof(struct inputKeyboardActionT)*componentInputKeyboard[entity].maxActions);
        if(componentInputKeyboard[entity].actions == NULL){
            sprintf(msg,"Warning in componentInputKeyboardAddAction() - Could not allocate memory for a new Action. Action:%s was not added to entity:%d!",name,entity);
            writeToLog(msg,"systemlog.txt");
            return;
        }
        //initialize the new actions
        for(i=0;i<componentInputKeyboard[entity].maxActions;++i){
            componentInputKeyboard[entity].actions[i].name = NULL;
            componentInputKeyboard[entity].actions[i].oldState = COMPONENT_INPUTKEYBOARD_STATE_RELEASED;
            componentInputKeyboard[entity].actions[i].state = COMPONENT_INPUTKEYBOARD_STATE_RELEASED;
            componentInputKeyboard[entity].actions[i].scanCode = -1;
        }
    }
    //if more memory is needed
    else if(componentInputKeyboard[entity].numActions >= componentInputKeyboard[entity].maxActions)
    {
        //allocate memory for five more actions
        componentInputKeyboard[entity].maxActions+=5;
        newKeyboardAction = realloc(componentInputKeyboard[entity].actions,sizeof(struct inputKeyboardActionT)*componentInputKeyboard[entity].maxActions);
        if(newKeyboardAction == NULL){
            sprintf(msg,"Warning in componentInputKeyboardAddAction() - Could not re-allocate more memory for keyboard actions. Action:%s was not added!",name);
            writeToLog(msg,"systemlog.txt");
            componentInputKeyboard[entity].maxActions-=5;
            return;
        }
        //point the actions to the new memory
        componentInputKeyboard[entity].actions = newKeyboardAction;
    }
    //if the action already exist
    actionIndex = componentInputKeyboardGetActionIndex(componentInputKeyboard,entity,name);
    if(actionIndex!=-1){
        //change the key scan code for the action
        componentInputKeyboard[entity].actions[actionIndex].scanCode = scanCode;
        //return out of the function
        return;
    }

    //add the keyboard action
    //allocate memory for the name
    componentInputKeyboard[entity].actions[componentInputKeyboard[entity].numActions].name = malloc(sizeof(char)*len);
    if(componentInputKeyboard[entity].actions[componentInputKeyboard[entity].numActions].name)
    {
        writeToLog("Error in componentInputKeyboardAddAction(): Could not allocate memory for action name:%s",name);
    }
    else{
        //copy the name
        sprintf(componentInputKeyboard[entity].actions[componentInputKeyboard[entity].numActions].name,"%s",name);
    }

    strncpy(componentInputKeyboard[entity].actions[componentInputKeyboard[entity].numActions].name,name,len);
    componentInputKeyboard[entity].actions[componentInputKeyboard[entity].numActions].name[len+1] = '\0';
    //set the keyboard scan code
    componentInputKeyboard[entity].actions[componentInputKeyboard[entity].numActions].scanCode = scanCode;

    //set the key states
    componentInputKeyboard[entity].actions[componentInputKeyboard[entity].numActions].state = COMPONENT_INPUTKEYBOARD_STATE_RELEASED;
    componentInputKeyboard[entity].actions[componentInputKeyboard[entity].numActions].oldState = COMPONENT_INPUTKEYBOARD_STATE_RELEASED;

    //Update number of actions
    componentInputKeyboard[entity].numActions++;

}

int componentInputKeyboardGetActionIndex(componentInputKeyboardT *componentInputKeyboard,Uint32 entity,char *actionName)
{
    int i=0;
    //if the keyboard input component is valid
    if(componentInputKeyboard != NULL)
    {
        //loop through the input keyboard actions of the entity
        for(i=0;i<componentInputKeyboard[entity].numActions;++i)
        {
            //if the name matches
            if(strcmp(actionName,componentInputKeyboard[entity].actions[i].name)==0)
            {
                //return the index
                return i;
            }
        }
    }
    return -1;
}

void componentFreeInputKeyboard(componentInputKeyboardT *componentInputKeyboard,int maxEntities)
{
    int i=0;
    if(componentInputKeyboard !=NULL)
    {
        for(i=0;i<maxEntities;++i){
            if(componentInputKeyboard[i].actions !=NULL)
            {
                //free the actions
                free(componentInputKeyboard[i].actions);
            }
        }
        //free the input keyboard component
        free(componentInputKeyboard);
    }
}

void componentInputKeyboardSetActiveState(componentInputKeyboardT *componentInputKeyboard,Uint32 entity,int value)
{
    char msg[512];
    if(componentInputKeyboard !=NULL)
    {
        //if the value is 1 or 0
        if(value==1 || value == 0){
            componentInputKeyboard[entity].active = value;
        }
        else{
            sprintf(msg,"Warning in componentInputKeyboardSetActiveState() - Cannot set state for entity:%d, parameter:'int value' must be 1 or 0. Invalid passed value was:%d",entity,value);
            writeToLog(msg,"systemlog.txt");
        }
    }
}
