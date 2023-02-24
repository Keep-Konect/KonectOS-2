#include <kot-ui++/component.h>

namespace Ui {
    void ButtonDraw(Button_t* Button){
        memset32(Button->Cpnt->GetFramebuffer()->Buffer, Button->CurrentColor, Button->Cpnt->GetFramebuffer()->Size);
    }

    void ButtonUpdate(Component* Cpnt){
        Button_t* Button = (Button_t*)Cpnt->ExternalData;
        if(Cpnt->IsFramebufferUpdate){
            // Draw
            Button->Style.G.Currentwidth = Cpnt->GetStyle()->Currentwidth;
            Button->Style.G.Currentheight = Cpnt->GetStyle()->Currentheight;

            ButtonDraw(Button);
            Cpnt->IsFramebufferUpdate = false;
        }else if(Button->IsDrawUpdate){
            ButtonDraw(Button);
            Button->IsDrawUpdate = false;
        }
        Cpnt->AbsolutePosition = {.x = (int64_t)(Cpnt->Parent->AbsolutePosition.x + Cpnt->Style->Position.x + Cpnt->Style->Margin.Left), .y = (int64_t)(Cpnt->Parent->AbsolutePosition.y + Cpnt->Style->Position.y + Cpnt->Style->Margin.Top)};
        
        BlitFramebuffer(Cpnt->Parent->GetFramebuffer(), Cpnt->GetFramebuffer(), (int64_t)(Cpnt->Style->Position.x + Cpnt->Style->Margin.Left), (int64_t)(Cpnt->Style->Position.y + Cpnt->Style->Margin.Top));
        SetGraphicEventbuffer(Cpnt->UiCtx->EventBuffer, (uint64_t)Cpnt, Button->Style.G.Currentwidth, Button->Style.G.Currentheight, Cpnt->AbsolutePosition.x, Cpnt->AbsolutePosition.y);
    }

    void ButtonMouseEvent(class Component* Cpnt, bool IsHover, int64_t RelativePositionX, int64_t RelativePositionY, int64_t PositionX, int64_t PositionY, int64_t ZValue, uint64_t Status){
        if(IsHover){
            Button_t* Button = (Button_t*)Cpnt->ExternalData;
            if(Cpnt->UiCtx->FocusCpnt != Cpnt){
                if(Cpnt->UiCtx->FocusCpnt->MouseEvent){
                    Cpnt->UiCtx->FocusCpnt->MouseEvent(Cpnt->UiCtx->FocusCpnt, false, RelativePositionX, RelativePositionY, PositionX, PositionY, ZValue, Status);
                }
            }
            Cpnt->UiCtx->FocusCpnt = Button->Cpnt;

            if(Status & MOUSE_CLICK_LEFT){
                Button->CurrentColor = Button->Style.ClickColor;
                Button->Style.OnMouseEvent(Button, ButtonEventTypeLeftClick);
                Button->IsDrawUpdate = true;
            }else if(Status & MOUSE_CLICK_RIGHT){
                Button->CurrentColor = Button->Style.ClickColor;
                Button->Style.OnMouseEvent(Button, ButtonEventTypeRightClick);
                Button->IsDrawUpdate = true;
            }else if(Status & MOUSE_CLICK_MIDDLE){
                Button->Style.OnMouseEvent(Button, ButtonEventTypeMiddleClick);
                Button->CurrentColor = Button->Style.HoverColor;
                Button->IsDrawUpdate = true;
            }else{
                Button->Style.OnMouseEvent(Button, ButtonEventTypeHover);
                Button->CurrentColor = Button->Style.HoverColor;
                Button->IsDrawUpdate = true;               
            }
        }else{
            Button_t* Button = (Button_t*)Cpnt->ExternalData;
            Button->CurrentColor = Button->Style.BackgroundColor;
            Button->IsDrawUpdate = true;
            Button->Style.OnMouseEvent(Button, ButtonEventTypeUnfocus);   
        }
    }

    Button_t* Button(ButtonStyle_t Style, Component* ParentCpnt){
        Button_t* Button = (Button_t*)malloc(sizeof(Button_t));
        memcpy(&Button->Style, &Style, sizeof(ButtonStyle_t));
        Button->CurrentColor = Button->Style.BackgroundColor;
        Button->Cpnt = new Component(Style.G, ButtonUpdate, ButtonMouseEvent, (uintptr_t)Button, ParentCpnt, true, true);
        return Button;
    }

}