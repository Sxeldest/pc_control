#include "settings.h"
#include <mod/config.h>
#include <mod/amlmod.h>
#include <stdio.h>

PCControlSettings g_pcSettings = {
    false, // showMenu
    true,  // enableCameraPatch
    false, // disableNativeCrouch
    false, // disableNativeJump
    true,  // enableAnalogPatch
    true,  // enableSprintDoubleTapBoost
    40.0f,
    35.0f,
    30.0f,
    25.0f,
    7.0f,
    1.0f,
    1500.0f,
    350.0f,
    5.0f,
    1.0f,
    {},    // widgets (all false/zero)
    0,     // selectedWidget
    false, // disableLookBehind
    false, // disablePinchZoom
    false, // hideCustomWidgets

    100.0f, // menuPosX
    100.0f, // menuPosY
    650.0f, // menuWidth
    550.0f, // menuHeight
    0.85f,  // menuFontScale
    
    0.26f,  // accent R
    0.59f,  // accent G
    0.98f,  // accent B
    
    0.95f,  // bg opacity
    8.0f    // rounding
};

static const char* kSettingsSection = "PCControl";

static ConfigEntry* s_enableCameraPatch = nullptr;
static ConfigEntry* s_disableNativeCrouch = nullptr;
static ConfigEntry* s_disableNativeJump = nullptr;
static ConfigEntry* s_analogPatch = nullptr;
static ConfigEntry* s_sprintDoubleTapBoost = nullptr;
static ConfigEntry* s_camSensX = nullptr;
static ConfigEntry* s_camSensY = nullptr;
static ConfigEntry* s_aimSensX = nullptr;
static ConfigEntry* s_aimSensY = nullptr;
static ConfigEntry* s_smoothness = nullptr;
static ConfigEntry* s_deathListFontSize = nullptr;
static ConfigEntry* s_deathListPosX = nullptr;
static ConfigEntry* s_deathListPosY = nullptr;
static ConfigEntry* s_deathListSpacing = nullptr;
static ConfigEntry* s_customWidgetOpacity = nullptr;

static ConfigEntry* s_widgetEnabled[MAX_CUSTOM_WIDGETS];
static ConfigEntry* s_widgetAction[MAX_CUSTOM_WIDGETS];
static ConfigEntry* s_widgetType[MAX_CUSTOM_WIDGETS];
static ConfigEntry* s_widgetPosX[MAX_CUSTOM_WIDGETS];
static ConfigEntry* s_widgetPosY[MAX_CUSTOM_WIDGETS];
static ConfigEntry* s_widgetSize[MAX_CUSTOM_WIDGETS];
static ConfigEntry* s_widgetMacroDelay[MAX_CUSTOM_WIDGETS];
static ConfigEntry* s_widgetAreaW[MAX_CUSTOM_WIDGETS];
static ConfigEntry* s_widgetAreaH[MAX_CUSTOM_WIDGETS];

static ConfigEntry* s_disableLookBehind = nullptr;
static ConfigEntry* s_disablePinchZoom = nullptr;
static ConfigEntry* s_hideCustomWidgets = nullptr;

static int* s_nativeMenuOpened = nullptr;
static bool* s_nativeUserPause = nullptr;

static ConfigEntry* s_menuPosX = nullptr;
static ConfigEntry* s_menuPosY = nullptr;
static ConfigEntry* s_menuWidth = nullptr;
static ConfigEntry* s_menuHeight = nullptr;
static ConfigEntry* s_menuFontScale = nullptr;

static ConfigEntry* s_menuAccentR = nullptr;
static ConfigEntry* s_menuAccentG = nullptr;
static ConfigEntry* s_menuAccentB = nullptr;
static ConfigEntry* s_menuBgOpacity = nullptr;
static ConfigEntry* s_menuRounding = nullptr;

static float ClampSetting(float value, float min, float max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void InitPCControlSettings()
{
    cfg->Init();

    s_enableCameraPatch = cfg->Bind("EnableCameraPatch", true, kSettingsSection);
    s_disableNativeCrouch = cfg->Bind("DisableNativeCrouch", false, kSettingsSection);
    s_disableNativeJump = cfg->Bind("DisableNativeJump", false, kSettingsSection);
    s_analogPatch = cfg->Bind("AnalogPatch", true, kSettingsSection);
    s_sprintDoubleTapBoost = cfg->Bind("SprintDoubleTapBoost", true, kSettingsSection);
    s_camSensX = cfg->Bind("CamSensX", 40.0f, kSettingsSection);
    s_camSensY = cfg->Bind("CamSensY", 35.0f, kSettingsSection);
    s_aimSensX = cfg->Bind("AimSensX", 30.0f, kSettingsSection);
    s_aimSensY = cfg->Bind("AimSensY", 25.0f, kSettingsSection);
    s_smoothness = cfg->Bind("Smoothness", 7.0f, kSettingsSection);
    s_deathListFontSize = cfg->Bind("DeathListFontSize", 1.0f, kSettingsSection);
    s_deathListPosX = cfg->Bind("DeathListPosX", 1500.0f, kSettingsSection);
    s_deathListPosY = cfg->Bind("DeathListPosY", 350.0f, kSettingsSection);
    s_deathListSpacing = cfg->Bind("DeathListSpacing", 5.0f, kSettingsSection);
    s_customWidgetOpacity = cfg->Bind("CustomWidgetOpacity", 1.0f, kSettingsSection);

    for (int i = 0; i < MAX_CUSTOM_WIDGETS; ++i)
    {
        char buf[64];
        sprintf(buf, "Widget%d_Enabled", i);
        s_widgetEnabled[i] = cfg->Bind(buf, false, kSettingsSection);
        sprintf(buf, "Widget%d_Action", i);
        s_widgetAction[i] = cfg->Bind(buf, 0, kSettingsSection);
        sprintf(buf, "Widget%d_Type", i);
        s_widgetType[i] = cfg->Bind(buf, 0, kSettingsSection);
        sprintf(buf, "Widget%d_PosX", i);
        s_widgetPosX[i] = cfg->Bind(buf, 500.0f, kSettingsSection);
        sprintf(buf, "Widget%d_PosY", i);
        s_widgetPosY[i] = cfg->Bind(buf, 500.0f, kSettingsSection);
        sprintf(buf, "Widget%d_Size", i);
        s_widgetSize[i] = cfg->Bind(buf, 100.0f, kSettingsSection);
        sprintf(buf, "Widget%d_MacroDelay", i);
        s_widgetMacroDelay[i] = cfg->Bind(buf, 0.5f, kSettingsSection);
        sprintf(buf, "Widget%d_AreaW", i);
        s_widgetAreaW[i] = cfg->Bind(buf, 600.0f, kSettingsSection);
        sprintf(buf, "Widget%d_AreaH", i);
        s_widgetAreaH[i] = cfg->Bind(buf, 600.0f, kSettingsSection);

        g_pcSettings.widgets[i].enabled = s_widgetEnabled[i]->GetBool();
        g_pcSettings.widgets[i].action = s_widgetAction[i]->GetInt();
        g_pcSettings.widgets[i].type = s_widgetType[i]->GetInt();
        g_pcSettings.widgets[i].posX = s_widgetPosX[i]->GetFloat();
        g_pcSettings.widgets[i].posY = s_widgetPosY[i]->GetFloat();
        g_pcSettings.widgets[i].size = s_widgetSize[i]->GetFloat();
        g_pcSettings.widgets[i].macroDelay = s_widgetMacroDelay[i]->GetFloat();
        g_pcSettings.widgets[i].areaW = s_widgetAreaW[i]->GetFloat();
        g_pcSettings.widgets[i].areaH = s_widgetAreaH[i]->GetFloat();
    }

    s_disableLookBehind = cfg->Bind("DisableLookBehind", false, kSettingsSection);
    s_disablePinchZoom = cfg->Bind("DisablePinchZoom", false, kSettingsSection);
    s_hideCustomWidgets = cfg->Bind("HideCustomWidgets", false, kSettingsSection);

    s_menuPosX = cfg->Bind("MenuPosX", 100.0f, kSettingsSection);
    s_menuPosY = cfg->Bind("MenuPosY", 100.0f, kSettingsSection);
    s_menuWidth = cfg->Bind("MenuWidth", 650.0f, kSettingsSection);
    s_menuHeight = cfg->Bind("MenuHeight", 550.0f, kSettingsSection);
    s_menuFontScale = cfg->Bind("MenuFontScale", 0.85f, kSettingsSection);
    
    s_menuAccentR = cfg->Bind("MenuAccentR", 0.26f, kSettingsSection);
    s_menuAccentG = cfg->Bind("MenuAccentG", 0.59f, kSettingsSection);
    s_menuAccentB = cfg->Bind("MenuAccentB", 0.98f, kSettingsSection);
    
    s_menuBgOpacity = cfg->Bind("MenuBgOpacity", 0.95f, kSettingsSection);
    s_menuRounding = cfg->Bind("MenuRounding", 8.0f, kSettingsSection);

    g_pcSettings.enableCameraPatch = s_enableCameraPatch->GetBool();
    g_pcSettings.disableNativeCrouch = s_disableNativeCrouch->GetBool();
    g_pcSettings.disableNativeJump = s_disableNativeJump->GetBool();
    g_pcSettings.enableAnalogPatch = s_analogPatch->GetBool();
    g_pcSettings.enableSprintDoubleTapBoost = s_sprintDoubleTapBoost->GetBool();
    g_pcSettings.camSensX = ClampSetting(s_camSensX->GetFloat(), 1.0f, 100.0f);
    g_pcSettings.camSensY = ClampSetting(s_camSensY->GetFloat(), 1.0f, 100.0f);
    g_pcSettings.aimSensX = ClampSetting(s_aimSensX->GetFloat(), 1.0f, 100.0f);
    g_pcSettings.aimSensY = ClampSetting(s_aimSensY->GetFloat(), 1.0f, 100.0f);
    g_pcSettings.smoothness = ClampSetting(s_smoothness->GetFloat(), 1.0f, 20.0f);
    g_pcSettings.deathListFontSize = ClampSetting(s_deathListFontSize->GetFloat(), 0.1f, 3.0f);
    g_pcSettings.deathListPosX = ClampSetting(s_deathListPosX->GetFloat(), 0.0f, 3000.0f);
    g_pcSettings.deathListPosY = ClampSetting(s_deathListPosY->GetFloat(), 0.0f, 2000.0f);
    g_pcSettings.deathListSpacing = ClampSetting(s_deathListSpacing->GetFloat(), 0.0f, 100.0f);
    g_pcSettings.customWidgetOpacity = ClampSetting(s_customWidgetOpacity->GetFloat(), 0.0f, 1.0f);

    g_pcSettings.disableLookBehind = s_disableLookBehind->GetBool();
    g_pcSettings.disablePinchZoom = s_disablePinchZoom->GetBool();
    g_pcSettings.hideCustomWidgets = s_hideCustomWidgets->GetBool();

    uintptr_t gtasa = aml->GetLib("libGTASA.so");
    if (gtasa)
    {
        s_nativeMenuOpened = (int*)(gtasa + 0x6E0098);
        s_nativeUserPause = (bool*)aml->GetSym(aml->GetLibHandle("libGTASA.so"), "_ZN6CTimer11m_UserPauseE");
    }

    g_pcSettings.menuPosX = s_menuPosX->GetFloat();
    g_pcSettings.menuPosY = s_menuPosY->GetFloat();
    g_pcSettings.menuWidth = s_menuWidth->GetFloat();
    g_pcSettings.menuHeight = s_menuHeight->GetFloat();
    g_pcSettings.menuFontScale = s_menuFontScale->GetFloat();
    
    g_pcSettings.menuAccentR = s_menuAccentR->GetFloat();
    g_pcSettings.menuAccentG = s_menuAccentG->GetFloat();
    g_pcSettings.menuAccentB = s_menuAccentB->GetFloat();
    
    g_pcSettings.menuBgOpacity = s_menuBgOpacity->GetFloat();
    g_pcSettings.menuRounding = s_menuRounding->GetFloat();
    
    g_pcSettings.showMenu = false;

    SavePCControlSettings();
}

void SavePCControlSettings()
{
    if (!s_enableCameraPatch) return;

    // Clamp values
    g_pcSettings.camSensX = ClampSetting(g_pcSettings.camSensX, 1.0f, 100.0f);
    g_pcSettings.camSensY = ClampSetting(g_pcSettings.camSensY, 1.0f, 100.0f);
    g_pcSettings.aimSensX = ClampSetting(g_pcSettings.aimSensX, 1.0f, 100.0f);
    g_pcSettings.aimSensY = ClampSetting(g_pcSettings.aimSensY, 1.0f, 100.0f);
    g_pcSettings.smoothness = ClampSetting(g_pcSettings.smoothness, 1.0f, 20.0f);
    g_pcSettings.deathListFontSize = ClampSetting(g_pcSettings.deathListFontSize, 0.1f, 3.0f);
    g_pcSettings.deathListPosX = ClampSetting(g_pcSettings.deathListPosX, 0.0f, 3000.0f);
    g_pcSettings.deathListPosY = ClampSetting(g_pcSettings.deathListPosY, 0.0f, 2000.0f);
    g_pcSettings.deathListSpacing = ClampSetting(g_pcSettings.deathListSpacing, 0.0f, 100.0f);
    g_pcSettings.customWidgetOpacity = ClampSetting(g_pcSettings.customWidgetOpacity, 0.0f, 1.0f);

    for (int i = 0; i < MAX_CUSTOM_WIDGETS; ++i)
    {
        g_pcSettings.widgets[i].posX = ClampSetting(g_pcSettings.widgets[i].posX, 0.0f, 3000.0f);
        g_pcSettings.widgets[i].posY = ClampSetting(g_pcSettings.widgets[i].posY, 0.0f, 2000.0f);
        g_pcSettings.widgets[i].size = ClampSetting(g_pcSettings.widgets[i].size, 10.0f, 1000.0f);
        g_pcSettings.widgets[i].macroDelay = ClampSetting(g_pcSettings.widgets[i].macroDelay, 0.0f, 5.0f);
        g_pcSettings.widgets[i].areaW = ClampSetting(g_pcSettings.widgets[i].areaW, 50.0f, 2000.0f);
        g_pcSettings.widgets[i].areaH = ClampSetting(g_pcSettings.widgets[i].areaH, 50.0f, 2000.0f);

        s_widgetEnabled[i]->SetBool(g_pcSettings.widgets[i].enabled);
        s_widgetAction[i]->SetInt(g_pcSettings.widgets[i].action);
        s_widgetType[i]->SetInt(g_pcSettings.widgets[i].type);
        s_widgetPosX[i]->SetFloat(g_pcSettings.widgets[i].posX);
        s_widgetPosY[i]->SetFloat(g_pcSettings.widgets[i].posY);
        s_widgetSize[i]->SetFloat(g_pcSettings.widgets[i].size);
        s_widgetMacroDelay[i]->SetFloat(g_pcSettings.widgets[i].macroDelay);
        s_widgetAreaW[i]->SetFloat(g_pcSettings.widgets[i].areaW);
        s_widgetAreaH[i]->SetFloat(g_pcSettings.widgets[i].areaH);
    }

    s_enableCameraPatch->SetBool(g_pcSettings.enableCameraPatch);
    s_disableNativeCrouch->SetBool(g_pcSettings.disableNativeCrouch);
    s_disableNativeJump->SetBool(g_pcSettings.disableNativeJump);
    s_analogPatch->SetBool(g_pcSettings.enableAnalogPatch);
    s_sprintDoubleTapBoost->SetBool(g_pcSettings.enableSprintDoubleTapBoost);
    s_camSensX->SetFloat(g_pcSettings.camSensX);
    s_camSensY->SetFloat(g_pcSettings.camSensY);
    s_aimSensX->SetFloat(g_pcSettings.aimSensX);
    s_aimSensY->SetFloat(g_pcSettings.aimSensY);
    s_smoothness->SetFloat(g_pcSettings.smoothness);
    s_deathListFontSize->SetFloat(g_pcSettings.deathListFontSize);
    s_deathListPosX->SetFloat(g_pcSettings.deathListPosX);
    s_deathListPosY->SetFloat(g_pcSettings.deathListPosY);
    s_deathListSpacing->SetFloat(g_pcSettings.deathListSpacing);
    s_customWidgetOpacity->SetFloat(g_pcSettings.customWidgetOpacity);

    s_disableLookBehind->SetBool(g_pcSettings.disableLookBehind);
    s_disablePinchZoom->SetBool(g_pcSettings.disablePinchZoom);
    s_hideCustomWidgets->SetBool(g_pcSettings.hideCustomWidgets);

    s_menuPosX->SetFloat(g_pcSettings.menuPosX);
    s_menuPosY->SetFloat(g_pcSettings.menuPosY);
    s_menuWidth->SetFloat(g_pcSettings.menuWidth);
    s_menuHeight->SetFloat(g_pcSettings.menuHeight);
    s_menuFontScale->SetFloat(g_pcSettings.menuFontScale);
    
    s_menuAccentR->SetFloat(g_pcSettings.menuAccentR);
    s_menuAccentG->SetFloat(g_pcSettings.menuAccentG);
    s_menuAccentB->SetFloat(g_pcSettings.menuAccentB);
    
    s_menuBgOpacity->SetFloat(g_pcSettings.menuBgOpacity);
    s_menuRounding->SetFloat(g_pcSettings.menuRounding);

    cfg->Save();
}

void TogglePCControlMenu()
{
    g_pcSettings.showMenu = !g_pcSettings.showMenu;
}

bool IsPCControlMenuVisible()
{
    return g_pcSettings.showMenu;
}

bool IsAnyMenuOpen()
{
    if (g_pcSettings.showMenu) return true;
    if (s_nativeMenuOpened && *s_nativeMenuOpened != 0) return true;
    if (s_nativeUserPause && *s_nativeUserPause) return true;
    return false;
}
