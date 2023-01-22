#include <core/main.h>

using namespace std;

process_t Self;

vector_t* Background = NULL;
vector_t* Windows = NULL;
vector_t* Foreground = NULL;

vector_t* Monitors = NULL;

void RenderWindows(){
    // todo: multi threads monitor rendering
    for(uint64_t i = 0; i < Monitors->length; i++){
        monitorc* Monitor = (monitorc*)vector_get(Monitors, i);
        if(Monitor != NULL){
            Monitor->Update(Background, Windows, Foreground);
        }
    }
}

void UpdateAllEvents(){
    for(uint64_t i = 0; i < Monitors->length; i++){
        monitorc* Monitor = (monitorc*)vector_get(Monitors, i);
        if(Monitor != NULL){
            Monitor->UpdateEvents(Background, Windows, Foreground);
        }
    }
}

thread_t renderThread = NULL;

void threadRender(){
    RenderWindows();
    Sys_Execthread(renderThread, NULL, ExecutionTypeQueu, NULL);
    Sys_Close(KSUCCESS);
}

void InitialiseOrb(){
    Self = ShareProcessKey(Sys_GetProcess());

    Monitors = vector_create();

    Background = vector_create();
    Windows = vector_create();
    Foreground = vector_create();

    srv_system_callback_t* callback = Srv_System_GetFramebuffer(true);
    srv_system_framebuffer_t* bootframebuffer = (srv_system_framebuffer_t*)callback->Data;
    free(callback);

    size64_t fb_size = bootframebuffer->Pitch * bootframebuffer->Height;

    uint64_t virtualAddress = (uint64_t)MapPhysical((uintptr_t)bootframebuffer->Address, fb_size);

    monitorc* monitor0 = new monitorc(Self, (uintptr_t) virtualAddress, bootframebuffer->Width, bootframebuffer->Height, bootframebuffer->Pitch, bootframebuffer->Bpp, 0, 0);

    CursorMaxPosition.x = monitor0->GetWidth() - 1;
    CursorMaxPosition.y = monitor0->GetHeight() - 1;

    free(bootframebuffer);

    vector_push(Monitors, monitor0);
    
    windowc* LoadingScreen = new windowc(Window_Type_Background, NULL);
    LoadingScreen->Resize(Window_Max_Size, Window_Max_Size);
    LoadingScreen->SetVisible(true);
    
    loadBootGraphics(LoadingScreen->GetFramebuffer());
    
    InitializeCursor();

    Sys_Createthread(Self, (uintptr_t) &threadRender, PriviledgeDriver, NULL, &renderThread);
    Sys_Execthread(renderThread, NULL, ExecutionTypeQueu, NULL);
}

extern "C" int main() {
    InitialiseOrb();
    InitialiseServer();

    Printlog("[GRAPHICS/ORB] Service started");
    
    return KSUCCESS;
}