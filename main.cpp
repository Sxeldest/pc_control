#include <mod/config.h>
#include <mod/amlmod.h>
#include <mod/logger.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "ImGui/imgui.h"
#include "ImGui/RW/RenderWare.h"
#include "pccontrol/camera.h"
#include "pccontrol/command.h"
#include "pccontrol/menu.h"
#include "pccontrol/settings.h"
#include "pccontrol/deathlist.h"
#include "pccontrol/gui.h"
#include "pccontrol/timecyc.h"
#include "pccontrol/widgetcustom.h"

MYMODCFG(dexsocy.gtasa.pc.control, GTASA_PC_CONTROL, 1.1, Dexsociety)

uintptr_t addrLeftRight = 0x3FA1C8;
uintptr_t addrUpDown = 0x3FA248;
uintptr_t addrWidgetUpdate = 0x2C150C;
uintptr_t addrWidgetRegionLookUpdate = 0x2C0DA4;
uintptr_t addrIsDoubleTapped = 0x2B2068;
uintptr_t addrIsPinchZooming = 0x2B0DFC;
uintptr_t addrIsTouched = 0x2B0CBC;
uintptr_t addrIsReleased = 0x2B0D5C;
uintptr_t addrGetSprint = 0x3FBD60;
uintptr_t addrSprintJustDown = 0x3FBE14;
uintptr_t addrCycleWeaponLeft = 0x3FB1E8;
uintptr_t addrCycleWeaponRight = 0x3FB25C;
uintptr_t addrFindPlayerPed = 0x19847C;
uintptr_t addrClearWeaponTarget = 0x18B274;
uintptr_t addrSampOnInputEnd = 0x17CC88;
uintptr_t addrFontSizeMultiplier = 0x23751C;
uintptr_t addrBaseFontSize = 0x238EC0;
uintptr_t addrRenderText = 0x12AE34;
uintptr_t addrCalcTextSize = 0x12B090;
uintptr_t addrProcessWeaponSwitch = 0x4C58A8;
uintptr_t addrGetTaskUseGun = 0x4C0566;

DECL_HOOKi(GetPedWalkLeftRight, void* self);
DECL_HOOKi(GetPedWalkUpDown, void* self);
DECL_HOOK(float, WidgetUpdate, void* self);
DECL_HOOK(float*, WidgetRegionLookUpdate, void* self);
DECL_HOOKi(IsDoubleTapped, int widgetId, bool a2, int a3);
DECL_HOOKi(IsTouched, int widgetId, void* a2, int a3);
DECL_HOOKi(IsReleased, int widgetId, void* a2, int a3);
DECL_HOOKi(GetSprint, void* self, int sprintType);
DECL_HOOKi(SprintJustDown, void* self);
DECL_HOOKb(CycleWeaponLeftJustDown, void* self);
DECL_HOOKb(CycleWeaponRightJustDown, void* self);
DECL_HOOKv(CTimeCycle_Update, void* self);
DECL_HOOKv(SampOnInputEnd, void* env, void* thiz, void* text);
DECL_HOOK(unsigned int, RenderText, int a1, void* a2, void* a3, void* a4, int a5, float a6);
DECL_HOOK(int, CalcTextSize, float* a1, int a2, void* a3, float a4);
DECL_HOOK(int, IsPinchZooming, int a1, int a2, int a3);
DECL_HOOK(bool, InitRenderware);
DECL_HOOKv(Render2DStuff);
DECL_HOOKv(OnTouchEvent, int type, int fingerId, int x, int y);
int (*GetTaskUseGun)(void* self);
DECL_HOOK(int, ProcessWeaponSwitch, void* self, void* pad);
void* (*FindPlayerPed)(int);
void (*SetMoveState)(void* self, int state);

static bool g_crouchPrevState = false;
static bool g_jumpPrevState = false;
static bool g_sprintPrevState = false;
static bool g_prevWeaponPrevState = false;
static bool g_nextWeaponPrevState = false;
static int g_sprintDoubleTapBoost = 0;
static int g_sprintJustDownFrames = 0;

const int Z_SPRINT_DOUBLE_TAP_BOOST = 4;
const float Z_DEADZONE = 8.0f;
const float Z_AXIS_DEADZONE = 18.0f;
const float Z_AXIS_LOCK_RATIO = 0.35f;
const float Z_WALK_MAX = 80.0f;
const float Z_VISUAL_RUN = 127.0f;

static int g_cachedX = 0;
static int g_cachedY = 0;
static void* g_lastPed = nullptr;
void* pGameHandle = nullptr;
static uintptr_t g_gtasa = 0;
uintptr_t hSAMP = 0;
uintptr_t hSAMP_ORIG = 0;
RwReal* nearScreenZ = nullptr;
RwReal* recipNearClip = nullptr;
void (*SetScissorRect)(float*) = nullptr;
static bool g_imguiInitialized = false;
static uintptr_t* g_touchWidgets = nullptr;

struct CCam {
    char padding[14];
    uint16_t nMode;
    char padding1[132 - 16];
    float fAngleVertical;
    float fBetaSpeed;
    char padding2[8];
    float fAngleHorizontal;
    float fAlphaSpeed;
    char padding3[528 - 156];
};

struct CCamera {
    char padding[87];
    uint8_t nActiveCam;
    char padding2[172 - 88];
    uint32_t nControlMode;
    char padding3[368 - 176];
    CCam aCams[3];
};

static CCamera* pTheCamera = nullptr;

static bool IsCustomVCShootWidget(int widgetId) { return widgetId == 21; }
static bool IsCustomTargetWidget(int widgetId) { return widgetId == 1 || widgetId == 19 || widgetId == 20; }

static bool IsAimMode()
{
    return IsCameraInAimMode();
}

static void CleanAnalogAxes(float& x, float& y)
{
    float absX = fabsf(x);
    float absY = fabsf(y);
    if (absX < Z_AXIS_DEADZONE) x = 0.0f;
    if (absY < Z_AXIS_DEADZONE) y = 0.0f;
    absX = fabsf(x);
    absY = fabsf(y);
    if (absX > 0.0f && absY <= absX * Z_AXIS_LOCK_RATIO) y = 0.0f;
    if (absY > 0.0f && absX <= absY * Z_AXIS_LOCK_RATIO) x = 0.0f;
}

static float SnapVisualAngle(float x, float y, bool aimMode)
{
    if (aimMode)
    {
        if (fabsf(x) >= fabsf(y)) return (x >= 0.0f) ? 0.0f : 180.0f;
        return (y >= 0.0f) ? 90.0f : -90.0f;
    }
    float angle = atan2f(y, x) * 180.0f / 3.1415926535f;
    if (angle > -30.0f && angle <= 30.0f) return 0.0f;
    if (angle > 30.0f && angle <= 60.0f) return 45.0f;
    if (angle > 60.0f && angle <= 120.0f) return 90.0f;
    if (angle > 120.0f && angle <= 150.0f) return 135.0f;
    if (angle > 150.0f || angle <= -150.0f) return 180.0f;
    if (angle > -150.0f && angle <= -120.0f) return -135.0f;
    if (angle > -120.0f && angle <= -60.0f) return -90.0f;
    if (angle > -60.0f && angle <= -30.0f) return -45.0f;
    return 0.0f;
}

void CalculateWASD(void* self, int& outX, int& outY)
{
    if (!self) { outX = 0; outY = 0; return; }
    float rawX = (float)GetPedWalkLeftRight(self);
    float rawY = (float)GetPedWalkUpDown(self);
    CleanAnalogAxes(rawX, rawY);
    float mag = sqrtf(rawX * rawX + rawY * rawY);
    if (mag < Z_DEADZONE) { outX = 0; outY = 0; return; }
    outX = 0; outY = 0;
    bool aimMode = IsAimMode();
    if (aimMode)
    {
        if (fabsf(rawX) >= fabsf(rawY)) outX = (rawX >= 0.0f) ? 127 : -127;
        else outY = (rawY >= 0.0f) ? 127 : -127;
        return;
    }
    int speed = (mag <= Z_WALK_MAX) ? 64 : 127;
    int diagSpeed = (int)(speed * 0.7071f);
    float angle = atan2f(rawY, rawX) * 180.0f / 3.1415926535f;
    if (angle > -30.0f && angle <= 30.0f) outX = speed;
    else if (angle > 30.0f && angle <= 60.0f) { outX = diagSpeed; outY = diagSpeed; }
    else if (angle > 60.0f && angle <= 120.0f) outY = speed;
    else if (angle > 120.0f && angle <= 150.0f) { outX = -diagSpeed; outY = diagSpeed; }
    else if (angle > 150.0f || angle <= -150.0f) outX = -speed;
    else if (angle > -150.0f && angle <= -120.0f) { outX = -diagSpeed; outY = -diagSpeed; }
    else if (angle > -120.0f && angle <= -60.0f) outY = -speed;
    else if (angle > -60.0f && angle <= -30.0f) { outX = diagSpeed; outY = -diagSpeed; }
}

int HookOf_GetPedWalkLeftRight(void* self) {
    if (!self) return 0;

    float customX, customY;
    GetCustomAnalogValues(customX, customY);
    if (customX != 0.0f || customY != 0.0f) {
        if (IsActionTouched(ACTION_WALK)) { customX *= 0.45f; customY *= 0.45f; }
        g_cachedX = (int)customX;
        g_cachedY = (int)customY;
        g_lastPed = self;
        return g_cachedX;
    }

    if (!g_pcSettings.enableAnalogPatch) return GetPedWalkLeftRight(self);
    CalculateWASD(self, g_cachedX, g_cachedY);
    if (IsActionTouched(ACTION_WALK)) { g_cachedX = (int)((float)g_cachedX * 0.45f); g_cachedY = (int)((float)g_cachedY * 0.45f); }
    g_lastPed = self;
    return g_cachedX;
}

int HookOf_GetPedWalkUpDown(void* self) {
    if (!self) return 0;

    float customX, customY;
    GetCustomAnalogValues(customX, customY);
    if (customX != 0.0f || customY != 0.0f) {
        if (self == g_lastPed) {
             if (IsActionTouched(ACTION_WALK)) return (int)(customY * 0.45f);
             return g_cachedY;
        }
        if (IsActionTouched(ACTION_WALK)) { customX *= 0.45f; customY *= 0.45f; }
        g_cachedX = (int)customX;
        g_cachedY = (int)customY;
        g_lastPed = self;
        return g_cachedY;
    }

    if (!g_pcSettings.enableAnalogPatch) return GetPedWalkUpDown(self);
    if (self == g_lastPed) {
        if (IsActionTouched(ACTION_WALK)) return (int)((float)g_cachedY * 0.45f);
        return g_cachedY;
    }
    CalculateWASD(self, g_cachedX, g_cachedY);
    if (IsActionTouched(ACTION_WALK)) { g_cachedX = (int)((float)g_cachedX * 0.45f); g_cachedY = (int)((float)g_cachedY * 0.45f); }
    return g_cachedY;
}

float HookOf_WidgetUpdate(void* self)
{
    if (!self) return 0.0f;
    if (!g_pcSettings.enableAnalogPatch) return WidgetUpdate(self);
    float* fv = (float*)self;
    unsigned char* cv = (unsigned char*)self;
    float res = WidgetUpdate(self);
    if (cv[76] != 0)
    {
        float& knobX = fv[39]; float& knobY = fv[40];
        float centerX = fv[41]; float centerY = fv[42];
        float dx = knobX - centerX; float dy = knobY - centerY;
        CleanAnalogAxes(dx, dy);
        float mag = sqrtf(dx * dx + dy * dy);
        if (mag < Z_DEADZONE) { knobX = centerX; knobY = centerY; }
        else {
            bool aimMode = IsAimMode();
            float visualMag = aimMode ? Z_VISUAL_RUN : ((mag <= Z_WALK_MAX) ? 50.0f : Z_VISUAL_RUN);
            float snappedAngle = SnapVisualAngle(dx, dy, aimMode);
            float rad = snappedAngle * 3.1415926535f / 180.0f;
            knobX = centerX + cosf(rad) * visualMag; knobY = centerY + sinf(rad) * visualMag;
        }
    }
    return res;
}

float* HookOf_WidgetRegionLookUpdate(void* self)
{
    if (g_pcSettings.disableLookBehind && self)
    {
        *(float*)((uintptr_t)self + 68) = 0.0f;
        *(uint8_t*)((uintptr_t)self + 72) = 0;
        *(uint8_t*)((uintptr_t)self + 144) = 0;
    }
    float* res = WidgetRegionLookUpdate(self);
    if (g_pcSettings.disableLookBehind && self)
    {
        *(float*)((uintptr_t)self + 68) = 0.0f;
        *(uint8_t*)((uintptr_t)self + 72) = 0;
        *(uint8_t*)((uintptr_t)self + 144) = 0;
        uintptr_t holdEffect = *(uintptr_t*)((uintptr_t)self + 136);
        if (holdEffect) *(uint32_t*)(holdEffect + 28) = 0;
    }
    return res;
}

int HookOf_IsDoubleTapped(int widgetId, bool a2, int a3)
{
    if (widgetId == 167)
    {
        int res = 0;
        if (!g_pcSettings.disableNativeCrouch) res = IsDoubleTapped(widgetId, a2, a3);
        if (IsActionTouched(ACTION_CROUCH))
        {
            if (!g_crouchPrevState) { g_crouchPrevState = true; res = 1; }
        }
        else g_crouchPrevState = false;
        return res;
    }
    if (widgetId == 31 || widgetId == 168)
    {
        int nativeRes = IsDoubleTapped(widgetId, a2, a3);
        if (g_pcSettings.enableSprintDoubleTapBoost && nativeRes) g_sprintDoubleTapBoost = Z_SPRINT_DOUBLE_TAP_BOOST;
        int res = 0;
        if (!g_pcSettings.disableNativeJump) res = nativeRes;
        if (IsActionTouched(ACTION_JUMP))
        {
            if (!g_jumpPrevState) { g_jumpPrevState = true; res = 1; }
        }
        else g_jumpPrevState = false;
        return res;
    }
    return IsDoubleTapped(widgetId, a2, a3);
}

int HookOf_IsTouched(int widgetId, void* a2, int a3)
{
    int result = IsTouched(widgetId, a2, a3);

    if (IsActionTouched(ACTION_MACRO1))
    {
        bool aimMode = IsAimMode();
        if (!aimMode && IsCustomTargetWidget(widgetId)) result = 1;
        else if (aimMode && IsCustomVCShootWidget(widgetId)) result = 1;
    }

    if (IsActionTouched(ACTION_MACRO2))
    {
        if (IsCustomVCShootWidget(widgetId)) result = 1;
        if (!IsAimMode() && IsCustomTargetWidget(widgetId))
        {
            if (IsMacro2Ready()) result = 1;
        }
    }

    if (IsCustomVCShootWidget(widgetId) && IsActionTouched(ACTION_VC_SHOOT)) result = 1;

    if (IsCustomTargetWidget(widgetId) && IsActionTouched(ACTION_TARGET))
    {
        if (!(IsActionTouched(ACTION_MACRO2) && IsAimMode())) result = 1;
    }

    CameraPatchOnIsTouched(widgetId, result);
    return result;
}

int HookOf_IsReleased(int widgetId, void* a2, int a3)
{
    int result = IsReleased(widgetId, a2, a3);
    if (GetActionReleaseFrames(ACTION_MACRO1) > 0)
    {
        bool aimMode = IsAimMode();
        if (!aimMode && IsCustomTargetWidget(widgetId)) result = 1;
        else if (aimMode && IsCustomVCShootWidget(widgetId)) result = 1;
    }
    if (IsCustomVCShootWidget(widgetId) && GetActionReleaseFrames(ACTION_MACRO2) > 0) result = 1;
    if (IsCustomVCShootWidget(widgetId) && GetActionReleaseFrames(ACTION_VC_SHOOT) > 0) result = 1;
    if (IsCustomTargetWidget(widgetId) && GetActionReleaseFrames(ACTION_TARGET) > 0) result = 1;
    return result;
}

static bool IsCustomSprintTouched()
{
    return IsActionTouched(ACTION_SPRINT);
}

int HookOf_GetSprint(void* self, int sprintType)
{
    if (IsCustomSprintTouched())
    {
        void* player = FindPlayerPed(-1);
        if (player) SetMoveState(player, 6);
        return 1;
    }
    return GetSprint(self, sprintType);
}

int HookOf_SprintJustDown(void* self)
{
    if (IsActionTouched(ACTION_SPRINT))
    {
        if (!g_sprintPrevState)
        {
            g_sprintPrevState = true;
            g_sprintJustDownFrames = 2;
        }
    }
    else g_sprintPrevState = false;

    if (g_sprintJustDownFrames > 0)
    {
        g_sprintJustDownFrames--;
        return 1;
    }

    if (g_pcSettings.enableSprintDoubleTapBoost && g_sprintDoubleTapBoost > 0)
    {
        g_sprintDoubleTapBoost--;
        return 1;
    }
    return SprintJustDown(self);
}

static int g_prevWeaponFrames = 0;
static int g_nextWeaponFrames = 0;

bool HookOf_CycleWeaponLeftJustDown(void* self)
{
    if (IsActionTouched(ACTION_PREV_WEAPON))
    {
        if (!g_prevWeaponPrevState)
        {
            g_prevWeaponPrevState = true;
            g_prevWeaponFrames = 2; // Trigger for 2 frames to handle Aiming -> Clear -> Cycle
        }
    }
    else g_prevWeaponPrevState = false;

    if (g_prevWeaponFrames > 0)
    {
        g_prevWeaponFrames--;
        return true;
    }
    return CycleWeaponLeftJustDown(self);
}

bool HookOf_CycleWeaponRightJustDown(void* self)
{
    if (IsActionTouched(ACTION_NEXT_WEAPON))
    {
        if (!g_nextWeaponPrevState)
        {
            g_nextWeaponPrevState = true;
            g_nextWeaponFrames = 2; // Trigger for 2 frames to handle Aiming -> Clear -> Cycle
        }
    }
    else g_nextWeaponPrevState = false;

    if (g_nextWeaponFrames > 0)
    {
        g_nextWeaponFrames--;
        return true;
    }
    return CycleWeaponRightJustDown(self);
}

void HookOf_CTimeCycle_Update(void* self)
{
    CTimeCycle_Update(self);
    ApplyTimecycOverrides();
}

void HookOf_SampOnInputEnd(void* env, void* thiz, void* text)
{
    if (env && text)
    {
        void** vtable = *(void***)env;
        const char* (*GetStringUTFChars)(void*, void*, unsigned char*) = (const char* (*)(void*, void*, unsigned char*))vtable[169];
        void (*ReleaseStringUTFChars)(void*, void*, const char*) = (void (*)(void*, void*, const char*))vtable[170];
        unsigned char isCopy = 0;
        const char* chars = GetStringUTFChars(env, text, &isCopy);
        if (chars)
        {
            bool handled = HandleLocalCommand(chars);
            ReleaseStringUTFChars(env, text, chars);
            if (handled) return;
        }
    }
    SampOnInputEnd(env, thiz, text);
}

unsigned int HookOf_RenderText(int a1, void* a2, void* a3, void* a4, int a5, float a6)
{
    if (hSAMP)
    {
        float multiplier = *(float*)(hSAMP + addrFontSizeMultiplier);
        float baseUnit = *(float*)(hSAMP + addrBaseFontSize);
        if (fabsf(a6 - baseUnit * 0.5f) < 0.01f || fabsf(a6 - baseUnit / 3.0f) < 0.01f) a6 *= multiplier;
    }
    return RenderText(a1, a2, a3, a4, a5, a6);
}

int HookOf_CalcTextSize(float* a1, int a2, void* a3, float a4)
{
    if (hSAMP)
    {
        float multiplier = *(float*)(hSAMP + addrFontSizeMultiplier);
        float baseUnit = *(float*)(hSAMP + addrBaseFontSize);
        if (fabsf(a4 - baseUnit * 0.5f) < 0.01f || fabsf(a4 - baseUnit / 3.0f) < 0.01f) a4 *= multiplier;
    }
    return CalcTextSize(a1, a2, a3, a4);
}

int HookOf_IsPinchZooming(int a1, int a2, int a3)
{
    if (g_pcSettings.disablePinchZoom) return 0;
    return IsPinchZooming(a1, a2, a3);
}

bool HookOf_InitRenderware()
{
    if (!InitRenderware()) return false;
    InitRenderWareFunctions();
    CameraPatchOnInitRenderware();
    g_pGUI = new PCControlGUI();
    if (!g_pGUI->initialize()) logger->Error("Failed to initialize GUI");
    g_imguiInitialized = true;
    return true;
}

void HookOf_Render2DStuff()
{
    Render2DStuff();
    CameraPatchOnRender2D();
    UpdateWidgetReleaseFrames();
    if (!g_imguiInitialized || !g_pGUI) return;
    g_pGUI->render();
}

void HookOf_OnTouchEvent(int type, int fingerId, int x, int y)
{
    // Safety check for fingerId to prevent out-of-bounds in our own and game's arrays
    if (fingerId < 0 || fingerId >= 10) return;

    if (HandleWidgetDragging(type, fingerId, x, y)) return;
    if (HandleCustomWidgetTouch(type, fingerId, x, y)) return;
    CameraPatchOnTouchEvent(type, fingerId, x, y);

    if (g_imguiInitialized && IsPCControlMenuVisible())
    {
        ImGuiIO& io = ImGui::GetIO();
        if (fingerId == 0)
        {
            io.AddMousePosEvent((float)x, (float)y);
            if (type == 2) // DOWN
            {
                io.AddMouseButtonEvent(0, true);
            }
            else if (type == 1) // UP
            {
                io.AddMouseButtonEvent(0, false);
            }
        }

        // Block game input if menu is open
        return;
    }

    // Original game and SAMP often have a hard limit of 4-5 fingers.
    // To prevent crash, we only pass the first 4 fingers to the original OnTouchEvent.
    // Our custom widgets and camera above already handled all 10 fingers.
    if (fingerId < 4)
    {
        OnTouchEvent(type, fingerId, x, y);
    }
}

int HookOf_ProcessWeaponSwitch(void* self, void* pad)
{
    if (IsActionTouched(ACTION_NEXT_WEAPON) || IsActionTouched(ACTION_PREV_WEAPON))
    {
        uintptr_t playerData = *(uintptr_t*)((uintptr_t)self + 0x444);
        if (playerData)
        {
            *(uint16_t*)(playerData + 0x34) &= ~0x0808;
            *(uint8_t*)(playerData + 0x85) = 0;
        }

        void* intelligence = *(void**)((uintptr_t)self + 0x440);
        if (intelligence)
        {
            uintptr_t taskUseGun = (uintptr_t)GetTaskUseGun(intelligence);
            if (taskUseGun)
            {
                if (*(uint8_t*)(taskUseGun + 16) == 4) // state 4 = firing
                {
                    *(uint8_t*)(taskUseGun + 16) = 1; // state 1 = aiming
                }
            }
        }
    }
    return ProcessWeaponSwitch(self, pad);
}

extern "C" void OnModPreLoad()
{
    g_gtasa = aml->GetLib("libGTASA.so");
    if (!g_gtasa) return;
    pGameHandle = aml->GetLibHandle("libGTASA.so");
    nearScreenZ = (RwReal*)aml->GetSym(pGameHandle, "_ZN9CSprite2d11NearScreenZE");
    recipNearClip = (RwReal*)aml->GetSym(pGameHandle, "_ZN9CSprite2d13RecipNearClipE");
    SetScissorRect = (void (*)(float*))aml->GetSym(pGameHandle, "_ZN7CWidget10SetScissorER5CRect");
    g_touchWidgets = (uintptr_t*)aml->GetSym(pGameHandle, "_ZN15CTouchInterface10m_pWidgetsE");
    CameraPatchPreload(pGameHandle);
}

extern "C" void OnModLoad()
{
    uintptr_t gtasa = g_gtasa ? g_gtasa : aml->GetLib("libGTASA.so");
    hSAMP = aml->GetLib("libsamp.so");
    hSAMP_ORIG = aml->GetLib("libSAMP_ORIG.so");
    InitPCControlSettings();
    if (gtasa)
    {
        g_gtasa = gtasa;
        if (!pGameHandle) pGameHandle = aml->GetLibHandle("libGTASA.so");
        pTheCamera = (CCamera*)aml->GetSym(pGameHandle, "TheCamera");
        if (!g_touchWidgets) g_touchWidgets = (uintptr_t*)aml->GetSym(pGameHandle, "_ZN15CTouchInterface10m_pWidgetsE");
        InitTimecycEditor(pGameHandle);
        CameraPatchLoad(pGameHandle, gtasa);

        HOOK(GetPedWalkLeftRight, gtasa + addrLeftRight + 1);
        HOOK(GetPedWalkUpDown, gtasa + addrUpDown + 1);
        HOOK(WidgetUpdate, gtasa + addrWidgetUpdate + 1);
        HOOK(WidgetRegionLookUpdate, gtasa + addrWidgetRegionLookUpdate + 1);
        HOOK(IsDoubleTapped, gtasa + addrIsDoubleTapped + 1);
        HOOK(IsTouched, gtasa + addrIsTouched + 1);
        HOOK(IsReleased, gtasa + addrIsReleased + 1);
        HOOK(IsPinchZooming, gtasa + addrIsPinchZooming + 1);
        HOOK(GetSprint, gtasa + addrGetSprint + 1);
        HOOK(SprintJustDown, gtasa + addrSprintJustDown + 1);
        HOOK(CycleWeaponLeftJustDown, gtasa + addrCycleWeaponLeft + 1);
        HOOK(CycleWeaponRightJustDown, gtasa + addrCycleWeaponRight + 1);
        GetTaskUseGun = (int (*)(void*))(gtasa + addrGetTaskUseGun + 1);
        FindPlayerPed = (void* (*)(int))(aml->GetSym(pGameHandle, "_Z13FindPlayerPedi"));
        SetMoveState = (void (*)(void*, int))(aml->GetSym(pGameHandle, "_ZN4CPed12SetMoveStateE10eMoveState"));
        if (!SetMoveState) SetMoveState = (void (*)(void*, int))(gtasa + 0x3639A4 + 1);
        HOOK(ProcessWeaponSwitch, gtasa + addrProcessWeaponSwitch + 1);
        HOOK(CTimeCycle_Update, gtasa + 0x41EF28 + 1);
        HOOK(Render2DStuff, aml->GetSym(pGameHandle, "_Z13Render2dStuffv"));
        HOOKPLT(InitRenderware, gtasa + 0x66F2D0);
        HOOKPLT(OnTouchEvent, gtasa + 0x675DE4);

        // Increase multi-touch limit to support 10 fingers
        int* pMaxTouches = (int*)aml->GetSym(pGameHandle, "_ZN15CTouchInterface14m_nMaxTouchesE");
        if (pMaxTouches)
        {
            *pMaxTouches = 10;
        }
        else
        {
            // Fallback for some versions: usually m_nMaxTouches is near m_pWidgets
            // In v2.00, m_nMaxTouches is at 0xBD1840, m_pWidgets is at 0xBD1874
            // But since we have the symbol for m_pWidgets, we can't easily guess.
            // We'll trust the symbol if available.
        }
    }
    if (hSAMP)
    {
        HOOK(SampOnInputEnd, hSAMP + addrSampOnInputEnd + 1);
        HOOK(RenderText, hSAMP + addrRenderText + 1);
        HOOK(CalcTextSize, hSAMP + addrCalcTextSize + 1);
        DeathListHookLoad(hSAMP, false);
    }
    if (hSAMP_ORIG) DeathListHookLoad(hSAMP_ORIG, true);
}
