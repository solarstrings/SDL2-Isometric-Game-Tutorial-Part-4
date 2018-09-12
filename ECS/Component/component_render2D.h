#ifndef __COMPONENT_RENDER2D_H_
#define __COMPONENT_RENDER2D_H_
#include "../../texture.h"

typedef struct componentRender2DT
{
    textureT *texture;      //pointer to the texture
    int layer;              //which layer to render the component on
}componentRender2DT;

componentRender2DT *componentNewRender2D();
void componentFreeRender2D(componentRender2DT *render2Dcomponent);
void componentRender2DSetTextureAndClipRect(componentRender2DT *render2DComponent,Uint32 entity,textureT *texture,SDL_Rect *clipRect);
void componentRender2DSetClipRect(componentRender2DT *render2DComponent,Uint32 entity,SDL_Rect *clipRect);
void componentRender2DSetLayer(componentRender2DT *render2DComponent,Uint32 entity,int layer);

#endif // __COMPONENT_RENDER_H_
