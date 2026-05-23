#include "widgetcustom.h"
#include "settings.h"
#include "camera.h"
#include "../ImGui/imgui.h"
#include <math.h>

struct WidgetState {
    int activeFinger = -1;
    bool touched = false;
    int releaseFrames = 0;
    float analogX = 0.0f;
    float analogY = 0.0f;
    int macroTimer = 0;
    float currentPosX = 0.0f; // For DPAD dynamic movement
    float currentPosY = 0.0f;
};

static WidgetState s_widgetStates[MAX_CUSTOM_WIDGETS];
static uint32_t s_activeActionsMask = 0;
static int s_fingerOwner[15] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }; // Map fingerId to widget index

static void RefreshActiveActionsMask()
{
    s_activeActionsMask = 0;
    for (int i = 0; i < MAX_CUSTOM_WIDGETS; ++i)
    {
        if (s_widgetStates[i].touched || s_widgetStates[i].releaseFrames > 0)
        {
            s_activeActionsMask |= (1 << g_pcSettings.widgets[i].action);
        }
    }
}

bool IsActionTouched(eWidgetAction action);

static bool IsPointInCustomButton(float x, float y, float centerX, float centerY, float size)
{
    float radius = size * 0.5f;
    float dx = x - centerX;
    if (fabsf(dx) > radius) return false; // Early out
    float dy = y - centerY;
    if (fabsf(dy) > radius) return false; // Early out
    return (dx * dx + dy * dy) <= (radius * radius);
}

bool HandleCustomWidgetTouch(int type, int fingerId, int x, int y)
{
    if (IsAnyMenuOpen()) return false;
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) return false;

    bool blocked = false;
    for (int i = 0; i < MAX_CUSTOM_WIDGETS; ++i)
    {

        // If HUD is hidden, only allow interaction with the toggle button
        if (g_pcSettings.hideCustomWidgets && w.action != ACTION_TOGGLE_HUD) continue;

        WidgetState& state = s_widgetStates[i];
        bool isDPAD = (w.action == ACTION_DPAD);

        bool inside = false;
        if (isDPAD)
        {
            inside = ((float)x >= w.posX && (float)x <= w.posX + w.areaW &&
                      (float)y >= w.posY && (float)y <= w.posY + w.areaH);
        }
        else
        {
            inside = IsPointInCustomButton((float)x, (float)y, w.posX, w.posY, w.size);
        }

        bool isPassType = (w.type == WTYPE_PASSTHROUGH || w.type == WTYPE_SLIDE_PASS);
        bool isSlideType = (w.type == WTYPE_SLIDE || w.type == WTYPE_SLIDE_PASS);

        // Anti-Ghost Touch: Check if this finger is already busy with another widget
        bool fingerIsBusy = false;
        if (fingerId >= 0 && fingerId < 15)
        {
            if (s_fingerOwner[fingerId] != -1 && s_fingerOwner[fingerId] != i) fingerIsBusy = true;
        }

        if (type == 2) // Down
        {
            if (state.activeFinger == -1 && inside && !fingerIsBusy)
            {
                state.activeFinger = fingerId;
                if (fingerId >= 0 && fingerId < 15) s_fingerOwner[fingerId] = i;

                state.touched = true;
                state.macroTimer = 0;

                if (w.action == ACTION_TOGGLE_HUD)
                {
                    g_pcSettings.hideCustomWidgets = !g_pcSettings.hideCustomWidgets;
                }

                // Dynamic position for DPAD
                if (isDPAD)
                {
                    state.currentPosX = (float)x;
                    state.currentPosY = (float)y;
                }

                if (!isPassType) blocked = true;
            }
        }
        else if (type == 3) // Move
        {
            if (fingerId == state.activeFinger)
            {
                state.touched = true;
                if (!isPassType) blocked = true;
            }
            else if (isSlideType && state.activeFinger == -1 && inside && !fingerIsBusy)
            {
                if (!isDPAD)
                {
                    state.activeFinger = fingerId;
                    if (fingerId >= 0 && fingerId < 15) s_fingerOwner[fingerId] = i;
                    state.touched = true;
                    if (!isPassType) blocked = true;
                }
            }
        }
        else if (type == 1) // Up
        {
            if (fingerId == state.activeFinger)
            {
                if (state.touched) state.releaseFrames = 2;
                state.touched = false;
                state.activeFinger = -1;
                if (fingerId >= 0 && fingerId < 15) s_fingerOwner[fingerId] = -1;
                state.analogX = 0;
                state.analogY = 0;
                if (!isPassType) blocked = true;
            }
        }

        if (isDPAD && state.activeFinger != -1 && fingerId == state.activeFinger && (type == 3 || type == 2))
        {
            float dx = (float)x - state.currentPosX;
            float dy = (float)y - state.currentPosY;
            float mag = sqrtf(dx * dx + dy * dy);

            if (mag > 15.0f)
            {
                float angle = atan2f(dy, dx);
                bool aiming = IsCameraInAimMode();
                float step = aiming ? (3.14159265f / 2.0f) : (3.14159265f / 4.0f);
                float snapped = roundf(angle / step) * step;

                state.analogX = cosf(snapped) * 127.0f;
                state.analogY = sinf(snapped) * 127.0f;

                if (fabsf(state.analogX) < 1.0f) state.analogX = 0;
                if (fabsf(state.analogY) < 1.0f) state.analogY = 0;
                if (state.analogX > 120.0f) state.analogX = 127.0f;
                if (state.analogX < -120.0f) state.analogX = -127.0f;
                if (state.analogY > 120.0f) state.analogY = 127.0f;
                if (state.analogY < -120.0f) state.analogY = -127.0f;
            }
            else
            {
                state.analogX = 0;
                state.analogY = 0;
            }
        }
    }

    RefreshActiveActionsMask();
    return blocked;
}

static int s_dragFinger = -1;
static int s_dragWidgetIdx = 0; // 1..10: widgets, 13: AnalogArea

static int AlphaFromOpacity(int alpha)
{
    float opacity = g_pcSettings.customWidgetOpacity;
    if (opacity < 0.0f) opacity = 0.0f;
    if (opacity > 1.0f) opacity = 1.0f;
    return (int)((float)alpha * opacity);
}

static void DrawCustomWidget(const char* label, float centerX, float centerY, float size, bool touched, int selectionIdx, int action, float analogX = 0, float analogY = 0)
{
    if (g_pcSettings.customWidgetOpacity <= 0.0f) return;

    // For DPAD with Area, only draw if touched or menu is visible
    bool isDPAD = (action == ACTION_DPAD);
    if (isDPAD && !touched && !IsAnyMenuOpen()) return;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;

    ImVec2 center(centerX, centerY);
    float radius = size * 0.5f;

    bool isSelected = IsPCControlMenuVisible() && (g_pcSettings.selectedWidget == selectionIdx);

    ImU32 fill = touched ? IM_COL32(255, 255, 255, AlphaFromOpacity(180)) : IM_COL32(20, 20, 20, AlphaFromOpacity(130));
    ImU32 border = touched ? IM_COL32(255, 255, 255, AlphaFromOpacity(230)) : IM_COL32(255, 255, 255, AlphaFromOpacity(160));

    if (isSelected)
    {
        border = IM_COL32(255, 255, 0, AlphaFromOpacity(255));
        fill = IM_COL32(60, 60, 0, AlphaFromOpacity(150));
    }

    ImU32 textCol = IM_COL32(255, 255, 255, AlphaFromOpacity(230));

    // Circle style for all
    dl->AddCircleFilled(center, radius, fill, 48);
    dl->AddCircle(center, radius, border, 48, isSelected ? 5.0f : 3.0f);

    if (isDPAD)
    {
        // Draw 8-way directional hints
        float hintRadius = radius * 0.75f;
        ImU32 hintCol = IM_COL32(255, 255, 255, AlphaFromOpacity(80));
        for (int i = 0; i < 8; ++i)
        {
            float ang = i * (3.14159265f / 4.0f);
            dl->AddCircleFilled(ImVec2(center.x + cosf(ang) * hintRadius, center.y + sinf(ang) * hintRadius), 2.5f, hintCol);
        }

        // Draw Knob for DPAD
        if (touched)
        {
            float knobX = (analogX / 127.0f) * (radius * 0.6f);
            float knobY = (analogY / 127.0f) * (radius * 0.6f);
            dl->AddCircleFilled(ImVec2(center.x + knobX, center.y + knobY), radius * 0.45f, IM_COL32(255, 255, 255, AlphaFromOpacity(210)), 32);
            dl->AddCircle(ImVec2(center.x + knobX, center.y + knobY), radius * 0.45f, IM_COL32(0, 0, 0, AlphaFromOpacity(100)), 32, 2.0f);
        }
    }

    float fontSize = radius * 0.55f;
    if (isDPAD && touched) fontSize *= 0.7f; // Shrink text if knob is over it
    ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, label);
    dl->AddText(ImGui::GetFont(), fontSize, ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f), textCol, label);
}

void RenderCustomWidgets()
{
    const char* actionLabels[] = { "NONE", "VC", "TGT", "JMP", "CRH", "SPR", "DPD", "W-P", "W-N", "M1", "M2", "TOG", "WLK" };

    for (int i = 0; i < MAX_CUSTOM_WIDGETS; ++i)
    {
        CustomWidget& w = g_pcSettings.widgets[i];
        if (!w.enabled) continue;
        if (g_pcSettings.widgets[i].enabled)
        {
            CustomWidget& w = g_pcSettings.widgets[i];

            // Hide if HUD is toggled off, but always show Toggle HUD button and always show everything if menu is open
            if (g_pcSettings.hideCustomWidgets && w.action != ACTION_TOGGLE_HUD && !IsPCControlMenuVisible()) continue;

            int actionIdx = w.action;
            if (actionIdx < 0 || actionIdx >= 13) actionIdx = 0;

            float drawX = w.posX;
            float drawY = w.posY;

            if (actionIdx == ACTION_DPAD)
            {
                // In menu, DPAD is centered in its area
                if (IsAnyMenuOpen())
                {
                    drawX = w.posX + w.areaW * 0.5f;
                    drawY = w.posY + w.areaH * 0.5f;

                    // Draw Area Box
                    ImDrawList* dl = ImGui::GetBackgroundDrawList();
                    if (dl)
                    {
                        ImU32 col = (g_pcSettings.selectedWidget == (i + 1)) ? IM_COL32(255, 255, 0, 150) : IM_COL32(255, 255, 255, 60);
                        dl->AddRect(ImVec2(w.posX, w.posY), ImVec2(w.posX + w.areaW, w.posY + w.areaH), col, 0.0f, 0, 2.0f);
                        char buf[32]; sprintf(buf, "DPAD Area %d", i + 1);
                        dl->AddText(ImVec2(w.posX + 5, w.posY + 5), col, buf);
                    }
                }
                else if (s_widgetStates[i].touched)
                {
                    // Follow touch position when active
                    drawX = s_widgetStates[i].currentPosX;
                    drawY = s_widgetStates[i].currentPosY;
                }
                else
                {
                    // Hidden when not touched and menu is closed
                    continue;
                }
            }

            DrawCustomWidget(actionLabels[actionIdx], drawX, drawY, w.size,
                                  s_widgetStates[i].touched, i + 1, w.action,
                                  s_widgetStates[i].analogX, s_widgetStates[i].analogY);
        }
    }
}

static float s_dragOffsetX = 0.0f;
static float s_dragOffsetY = 0.0f;

bool HandleWidgetDragging(int type, int fingerId, int x, int y)
{
    if (!IsAnyMenuOpen()) return false;
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) return false;

    if (type == 2) // Down
    {
        for (int i = 0; i < MAX_CUSTOM_WIDGETS; ++i)
        {
            CustomWidget& w = g_pcSettings.widgets[i];
            if (!w.enabled) continue;

            if (w.action == ACTION_DPAD)
            {
                // Dragging the DPAD area
                if (x >= w.posX && x <= w.posX + w.areaW && y >= w.posY && y <= w.posY + w.areaH)
                {
                    s_dragFinger = fingerId;
                    s_dragWidgetIdx = i + 1;
                    s_dragOffsetX = (float)x - w.posX;
                    s_dragOffsetY = (float)y - w.posY;
                    g_pcSettings.selectedWidget = i + 1;
                    return true;
                }
            }
            else if (IsPointInCustomButton((float)x, (float)y, w.posX, w.posY, w.size))
            {
                s_dragFinger = fingerId;
                s_dragWidgetIdx = i + 1;
                s_dragOffsetX = (float)x - w.posX;
                s_dragOffsetY = (float)y - w.posY;
                g_pcSettings.selectedWidget = i + 1;
                return true;
            }
        }
    }
    else if (type == 3) // Move
    {
        if (fingerId == s_dragFinger)
        {
            if (s_dragWidgetIdx >= 1 && s_dragWidgetIdx <= MAX_CUSTOM_WIDGETS)
            {
                g_pcSettings.widgets[s_dragWidgetIdx - 1].posX = (float)x - s_dragOffsetX;
                g_pcSettings.widgets[s_dragWidgetIdx - 1].posY = (float)y - s_dragOffsetY;
            }
            return true;
        }
    }
    else if (type == 1) // Up
    {
        if (fingerId == s_dragFinger)
        {
            s_dragFinger = -1;
            s_dragWidgetIdx = 0;
            return true;
        }
    }

    return false;
}

bool IsActionTouched(eWidgetAction action)
{
    if (g_pcSettings.hideCustomWidgets && action != ACTION_TOGGLE_HUD) return false;
    return (s_activeActionsMask & (1 << (int)action)) != 0;
}

int GetActionReleaseFrames(eWidgetAction action)
{
    int maxFrames = 0;
    for (int i = 0; i < MAX_CUSTOM_WIDGETS; ++i)
    {
        CustomWidget& w = g_pcSettings.widgets[i];
        if (!w.enabled) continue;
        if (g_pcSettings.widgets[i].enabled && g_pcSettings.widgets[i].action == (int)action)
        {
            if (s_widgetStates[i].releaseFrames > maxFrames) maxFrames = s_widgetStates[i].releaseFrames;
        }
    }

    return maxFrames;
}

void UpdateWidgetReleaseFrames()
{
    for (int i = 0; i < MAX_CUSTOM_WIDGETS; ++i)
    {
        CustomWidget& w = g_pcSettings.widgets[i];
        if (!w.enabled) continue;
        if (s_widgetStates[i].releaseFrames > 0) s_widgetStates[i].releaseFrames--;

        if (s_widgetStates[i].touched)
        {
            if (g_pcSettings.widgets[i].action == ACTION_MACRO2) s_widgetStates[i].macroTimer++;
        }
        else
        {
            s_widgetStates[i].analogX = 0;
            s_widgetStates[i].analogY = 0;
            s_widgetStates[i].macroTimer = 0;
        }
    }
    RefreshActiveActionsMask();
}

void GetCustomAnalogValues(float& x, float& y)
{
    x = 0; y = 0;
    for (int i = 0; i < MAX_CUSTOM_WIDGETS; ++i)
    {
        CustomWidget& w = g_pcSettings.widgets[i];
        if (!w.enabled) continue;
        if (g_pcSettings.widgets[i].enabled && g_pcSettings.widgets[i].action == ACTION_DPAD)
        {
            if (s_widgetStates[i].touched)
            {
                x = s_widgetStates[i].analogX;
                y = s_widgetStates[i].analogY;
                return;
            }
        }
    }
}

bool IsMacro2Ready()
{
    for (int i = 0; i < MAX_CUSTOM_WIDGETS; ++i)
    {
        CustomWidget& w = g_pcSettings.widgets[i];
        if (!w.enabled) continue;
        if (g_pcSettings.widgets[i].enabled && g_pcSettings.widgets[i].action == ACTION_MACRO2)
        {
            if (s_widgetStates[i].touched && s_widgetStates[i].macroTimer >= (int)(g_pcSettings.widgets[i].macroDelay * 60.0f))
            {
                return true;
            }
        }
    }
    return false;
}
