#pragma once

enum eWidgetAction
{
    ACTION_NONE = 0,
    ACTION_VC_SHOOT,
    ACTION_TARGET,
    ACTION_JUMP,
    ACTION_CROUCH,
    ACTION_SPRINT,
    ACTION_DPAD,
    ACTION_PREV_WEAPON,
    ACTION_NEXT_WEAPON,
    ACTION_MACRO1,
    ACTION_MACRO2,
    ACTION_TOGGLE_HUD,
    ACTION_WALK,
    ACTION_CLEAR_AIM
};

enum eWidgetType
{
    WTYPE_DEFAULT = 0,      // Blocks touch, Click to activate
    WTYPE_PASSTHROUGH,      // Allows touch behind, Click to activate
    WTYPE_SLIDE,            // Blocks touch, Slide to activate
    WTYPE_SLIDE_PASS        // Allows touch behind, Slide to activate
};

struct CustomWidget
{
    bool enabled;
    int action; // eWidgetAction
    int type;   // eWidgetType
    float posX;
    float posY;
    float size;
    float macroDelay; // For Macro 2
    float areaW;      // For DPAD
    float areaH;      // For DPAD
};

#define MAX_CUSTOM_WIDGETS 10

struct PCControlSettings
{
    bool showMenu;
    bool enableCameraPatch;
    bool disableNativeCrouch;
    bool disableNativeJump;
    bool enableAnalogPatch;
    bool enableSprintDoubleTapBoost;
    float camSensX;
    float camSensY;
    float aimSensX;
    float aimSensY;
    float smoothness;
    float deathListFontSize;
    float deathListPosX;
    float deathListPosY;
    float deathListSpacing;
    float customWidgetOpacity;

    CustomWidget widgets[MAX_CUSTOM_WIDGETS];

    int selectedWidget; // 0: None, 1..MAX: widgets
    bool disableLookBehind;
    bool disablePinchZoom;
    bool hideCustomWidgets;

    float menuPosX;
    float menuPosY;
    float menuWidth;
    float menuHeight;
    float menuFontScale;
    
    float menuAccentR;
    float menuAccentG;
    float menuAccentB;
    
    float menuBgOpacity;
    float menuRounding;
};

extern PCControlSettings g_pcSettings;

void InitPCControlSettings();
void SavePCControlSettings();
void TogglePCControlMenu();
bool IsPCControlMenuVisible();
bool IsAnyMenuOpen();
