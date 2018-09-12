#ifndef __COMPONENT_ANIMATION_H_
#define __COMPONENT_ANIMATION_H_
#include "../../texture.h"

#define COMPONENT_ANIMATION_NUM_INITIAL_ANIMATIONS  10

//forward declaration of sceneT, allows us to use the sceneT struct without causing a cross-referencing header error
typedef struct sceneT sceneT;

typedef struct animationFrameT
{
    SDL_Rect clipRect;          //clip rectangle for the animation frame
    int frameTimeMilliSeconds;  //how long this frame should be displayed
}animationFrameT;

typedef struct animationT
{
    textureT *texture;          //pointer to the texture
    char *name;                 //name of the animation
    animationFrameT *frames;    //animation frames
    int numFrames;              //how many frames there are in the animation
    int currentFrame;           //current frame to draw
}animationT;

typedef struct componentAnimationT
{
    animationT *animations;
    int numAnimations;
    int maxAnimations;
}componentAnimationT;

componentAnimationT *componentNewAnimation();
void componentFreeAnimation(componentAnimationT *componentAnimation,int maxEntities);
int componentAnimationCreateNewAnimation(componentAnimationT *componentAnimation,int entity,textureT *texture,char *animationName);
int componentAnimationAddAnimationFrames(componentAnimationT *componentAnimation,int entity,int animationIndex,int width,int height,int numFrames,int frameTimeMilliSeconds);
void componentAnimationSetAnimationFrameTime(componentAnimationT *componentAnimation,int entity,int animationIndex,int frame,int frameTimeMilliseconds);
int componentAnimationGetAnimationIndexByName(componentAnimationT *componentAnimation,int entity,char *animationName);

#endif // __COMPONENT_ANIMATION_H_

