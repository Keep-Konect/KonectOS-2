#ifndef _UIPP_COMPONENT_H_
#define _UIPP_COMPONENT_H_

#include <kot/types.h>
#include <kot/utils/vector.h>

#include <kot-graphics/orb.h>
#include <kot-graphics/font.h>
#include <kot-graphics/utils.h>


#include <kot-ui++/context.h>

namespace Ui {

    struct Layout {
        /* Space */
        enum ComponentHorizontalSpace {
            FILLHORIZONTAL = 0,
            BETWEENHORIZONTAL = 1,
            AROUNDHORIZONTAL = 2,
            LEFT    = 3,
            CENTER  = 4,
            RIGHT   = 5,
        };

        enum ComponentVerticalSpace {
            FILLVERTICAL = 0,
            BETWEENVERTICAL = 1,
            AROUNDVERTICAL = 2,
            TOP     = 3,
            MIDDLE  = 4,
            BOTTOM  = 5,
        };

        struct ComponentAlign_t {
            ComponentHorizontalSpace x;
            ComponentVerticalSpace y;
        };

    };

    enum ButtonEventType{
        ButtonEventTypeHover = 0,
        ButtonEventTypeLeftClick = 1,
        ButtonEventTypeRightClick = 2,
        ButtonEventTypeMiddleClick = 3,
        ButtonEventTypeUnfocus = 4,
    };

    typedef void (*MouseEventHandler)(class Component* Component, bool IsHover, int64_t RelativePositionX, int64_t RelativePositionY, int64_t PositionX, int64_t PositionY, int64_t ZValue, uint64_t Status);
    typedef void (*UpdateHandler)(class Component* Component);
    typedef void (*ButtonHandler)(struct Button_t* Button, ButtonEventType EventType);

    struct Margin{
        uint64_t Top;
        uint64_t Bottom;
        uint64_t Left;
        uint64_t Right;
    };

    struct ComponentGeneralStyle{
        uint64_t Maxwidth;
        uint64_t Minwidth;
        uint64_t Currentwidth;
        int64_t Width;

        uint64_t Maxheight;
        uint64_t Minheight;
        uint64_t Currentheight;
        int64_t Height;

        Margin Margin;

        bool AutoPosition;

        point_t Position;

        uint64_t ZIndex; // TODO

        uint64_t ExternalStyleType;
        uintptr_t ExternalStyleData;

        bool IsVisible;

        bool IsHorizontalOverflow;
        bool IsVerticalOverflow;
    };

    class Component {
        public:
            Component(ComponentGeneralStyle Style, UpdateHandler HandlerUpdate, MouseEventHandler HandlerMouseEvent, uintptr_t ExternalData, class Component* ParentCpnt, bool IsUpdateChild, bool IsOwnFb);

            /* Component Framebuffer */
            void CreateFramebuffer(uint32_t Width, uint32_t Height);
            void UpdateFramebuffer(uint32_t Width, uint32_t Height);

            framebuffer_t* GetFramebuffer();
            ComponentGeneralStyle* GetStyle();
            vector_t* GetChilds();
            uint32_t GetTotalWidthChilds();
            uint32_t GetTotalHeightChilds();

            void Update();


            MouseEventHandler MouseEvent;
            UpdateHandler UpdateFunction;
            uintptr_t ExternalData;
            Component* Parent;
            class UiContext* UiCtx;
            bool IsFramebufferUpdate;
            bool UpdateChild;
            bool IsRedraw;

            point_t DrawPosition;            
            point_t AbsolutePosition;            

            Component* VerticalOverflow;
            Component* HorizontalOverflow;

            ComponentGeneralStyle* Style;
            uint64_t Deep;

            framebuffer_t* Framebuffer;
            
            vector_t* Childs;

            // Component.cpp private function
            void AddChild(Component* Child);
    };

    /* components */

    enum PictureboxType {
        _TGA = 0,
        _PNG = 1,
        _JPG = 2,
    };
    enum PictureboxFit{
        PICTUREFIT     = 0,
        PICTUREFILL    = 1,
        PICTURECENTER  = 2,
        PICTURESTRETCH = 3,
    };
    typedef struct {
        ComponentGeneralStyle G;
        PictureboxFit Fit;
    } PictureboxStyle_t;
    struct Picturebox_t{
        PictureboxType Type;
        PictureboxStyle_t Style;
        Component* Cpnt;
        uintptr_t Image;
        bool IsDrawUpdate;
        void UpdateSize(uint64_t Width, uint64_t Height);
        void UpdatePosition(point_t Position);
    };
    Picturebox_t* Picturebox(char* Path, PictureboxType Type, PictureboxStyle_t Style, class Component* ParentCpnt);

    typedef struct {
        ComponentGeneralStyle G;
        color_t BackgroundColor;
        color_t HoverColor;
        color_t ClickColor;
    } BoxStyle_t;
    struct Box_t{
        BoxStyle_t Style;
        Component* Cpnt;
        color_t CurrentColor;
        bool IsDrawUpdate;
        void UpdateSize(uint64_t Width, uint64_t Height);
        void UpdatePosition(point_t Position);
    };
    Box_t* Box(BoxStyle_t Style, class Component* ParentCpnt);

    typedef struct {
        ComponentGeneralStyle G;
        Layout::ComponentAlign_t Align;
    } FlexboxStyle_t;
    struct Flexbox_t{
        FlexboxStyle_t Style;
        Component* Cpnt;
        color_t CurrentColor;
        void UpdateSize(uint64_t Width, uint64_t Height);
        void UpdatePosition(point_t Position);
    };
    Flexbox_t* Flexbox(FlexboxStyle_t Style, class Component* ParentCpnt);

    typedef struct {
        ComponentGeneralStyle G;
        uint64_t CaseWidth;
        uint64_t CaseHeight;
    } GridboxStyle_t;
    struct Gridbox_t{
        GridboxStyle_t Style;
        Component* Cpnt;
        void UpdateSize(uint64_t Width, uint64_t Height);
        void UpdatePosition(point_t Position);
    };
    Gridbox_t* Gridbox(GridboxStyle_t Style, class Component* ParentCpnt);


    typedef struct {
        ComponentGeneralStyle G;
        color_t BackgroundColor;
        color_t HoverColor;
        color_t ClickColor;
        ButtonHandler OnMouseEvent;
        uintptr_t ExternalData;
    } ButtonStyle_t;
    struct Button_t{
        ButtonStyle_t Style;
        Component* Cpnt;
        color_t CurrentColor;
        bool IsDrawUpdate;
        void UpdateSize(uint64_t Width, uint64_t Height);
        void UpdatePosition(point_t Position);
    };
    Button_t* Button(ButtonStyle_t Style, class Component* ParentCpnt);

    typedef struct {
        ComponentGeneralStyle G;
        char* Text;
        char* FontPath;
        int16_t FontSize;
        color_t ForegroundColor;
    } LabelStyle_t;
    struct Label_t{
        LabelStyle_t Style;
        Component* Cpnt;
        color_t CurrentColor;
        bool IsDrawUpdate;
        kfont_t* Font;
        void UpdateText(char* Text);
        void UpdateSize(uint64_t Width, uint64_t Height);
        void UpdatePosition(point_t Position);
    };
    Label_t* Label(LabelStyle_t Style, class Component* ParentCpnt);

    typedef struct {
        uint32_t BackgroundColor;
        uint32_t ForegroundColor;
        bool IsVisible;
    } TitlebarStyle_t;
    struct Titlebar_t{
        TitlebarStyle_t Style;
        Component* Cpnt;
        Box_t* MainBox;
        Flexbox_t* TitleBox;
        Picturebox_t* Logo;
        Label_t* Title;
        Button_t* CloseBtn;
        Button_t* SizeBtn;
        Button_t* HideBtn;
        bool IsDrawUpdate;
        void UpdateSize(uint64_t Width, uint64_t Height);
        void UpdatePosition(point_t Position);
    };
    Titlebar_t* Titlebar(char* Title, char* Icon, TitlebarStyle_t Style, class Component* ParentCpnt);

}

#endif