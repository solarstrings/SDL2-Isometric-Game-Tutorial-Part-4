#ifndef __COMPONENT_INPUT_KEYBOARD_H_
#define __COMPONENT_INPUT_KEYBOARD_H_
#include <SDL2/SDL.h>

#define COMPONENT_INPUTKEYBOARD_STATE_RELEASED  0
#define COMPONENT_INPUTKEYBOARD_STATE_PRESSED   1
#define COMPONENT_INPUTKEYBOARD_ACTION_NAMELENGTH 15

//forward declaration of sceneT, allows us to use the sceneT struct without causing a cross-referencing header error
typedef struct sceneT sceneT;

typedef struct inputKeyboardActionT
{
    char *name;            //name of the action
    SDL_Scancode scanCode; //what key to map the action to
    char state;            //current state of the key
    char oldState;         //last key state
}inputKeyboardActionT;

typedef struct componentInputKeyboardT
{
    int numActions;                 //number of mapped actions
    int maxActions;                 //current max number of actions
    inputKeyboardActionT *actions;  //keyboard actions
    char active;                    //if the keyboard component is active
}componentInputKeyboardT;

componentInputKeyboardT *componentNewInputKeyboard();
void componentFreeInputKeyboard(componentInputKeyboardT *componentInputKeyboard,int numEntities);
void componentInputKeyboardAddAction(componentInputKeyboardT *componentInputKeyboard,int entity,char *name,SDL_Scancode scanCode);
int componentInputKeyboardGetActionIndex(componentInputKeyboardT *componentInputKeyboard,Uint32 entity,char *actionName);
void componentInputKeyboardSetActiveState(componentInputKeyboardT *componentInputKeyboard,Uint32 entity,int value);


#endif // __COMPONENT_INPUT_KEYBOARD_H_


