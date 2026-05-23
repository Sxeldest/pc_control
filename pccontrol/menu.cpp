#include "menu.h"
#include "settings.h"
#include "deathlist.h"
#include "timecyc.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include <stdio.h>

// Dex Control Panel v1.2 Build Fix
static void SetupModernStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style = ImGuiStyle(); // RESET STYLE DULU

    float safeScale = g_pcSettings.menuFontScale;
    if (safeScale < 0.6f) safeScale = 0.6f;
    if (safeScale > 1.5f) safeScale = 1.5f;

    ImVec4* colors = style.Colors;

    float r = g_pcSettings.menuAccentR;
    float g = g_pcSettings.menuAccentG;
    float b = g_pcSettings.menuAccentB;

    style.WindowRounding = g_pcSettings.menuRounding;

    style.ChildRounding =
        (g_pcSettings.menuRounding > 2.0f)
        ? g_pcSettings.menuRounding - 2.0f
        : 0.0f;

    style.FrameRounding = 8.0f;
    style.TabRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;

    style.FramePadding = ImVec2(8.0f, 6.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);

    style.WindowPadding = ImVec2(12.0f, 12.0f);

    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 12.0f;

    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.5f);

    colors[ImGuiCol_WindowBg] =
        ImVec4(0.11f, 0.11f, 0.14f, g_pcSettings.menuBgOpacity);

    colors[ImGuiCol_ChildBg] =
        ImVec4(0.14f, 0.14f, 0.17f, 0.60f);

    colors[ImGuiCol_Button] =
        ImVec4(r, g, b, 0.40f);

    colors[ImGuiCol_ButtonHovered] =
        ImVec4(r, g, b, 1.00f);

    colors[ImGuiCol_ButtonActive] =
        ImVec4(r, g, b, 0.80f);

    colors[ImGuiCol_CheckMark] =
        ImVec4(r, g, b, 1.00f);

    colors[ImGuiCol_Header] =
        ImVec4(r, g, b, 0.31f);

    colors[ImGuiCol_HeaderHovered] =
        ImVec4(r, g, b, 0.80f);

    colors[ImGuiCol_HeaderActive] =
        ImVec4(r, g, b, 1.00f);

    colors[ImGuiCol_Tab] =
        ImVec4(r * 0.7f, g * 0.7f, b * 0.7f, 0.86f);

    colors[ImGuiCol_TabHovered] =
        ImVec4(r, g, b, 0.80f);

    colors[ImGuiCol_TabActive] =
        ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, 1.00f);

    colors[ImGuiCol_FrameBg] =
        ImVec4(0.18f, 0.18f, 0.22f, 1.00f);

    colors[ImGuiCol_FrameBgHovered] =
        ImVec4(0.24f, 0.24f, 0.30f, 1.00f);

    colors[ImGuiCol_FrameBgActive] =
        ImVec4(0.28f, 0.28f, 0.35f, 1.00f);

    // Apply scaling to all sizes and global font scale
    style.ScaleAllSizes(safeScale);
    ImGui::GetIO().FontGlobalScale = safeScale;
}


static float GetButtonHeight()
{
    float scale = g_pcSettings.menuFontScale;

    if (scale < 0.6f)
        scale = 0.6f;

    if (scale > 1.5f)
        scale = 1.5f;

    return 48.0f * scale;
}

void RenderPCControlMenu()
{
    static float lastScale = -1.0f;
    if (lastScale != g_pcSettings.menuFontScale)
    {
        SetupModernStyle();
        lastScale = g_pcSettings.menuFontScale;
    }

    static bool wasVisible = false;
    if (g_pcSettings.showMenu && !wasVisible)
    {
        ImGui::SetNextWindowFocus();
    }
    wasVisible = g_pcSettings.showMenu;

    bool changed = false;
    ImGui::SetNextWindowPos(
        ImVec2(g_pcSettings.menuPosX, g_pcSettings.menuPosY),
        ImGuiCond_FirstUseEver
    );
    
    ImGui::SetNextWindowSize(
        ImVec2(g_pcSettings.menuWidth, g_pcSettings.menuHeight),
        ImGuiCond_FirstUseEver
    );
    // Add NoNav flag to prevent issues with focus in some environments
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;

    if (ImGui::Begin("Dexsociety Menu", &g_pcSettings.showMenu, flags))
    {
        float safeScale = g_pcSettings.menuFontScale;
        
        if (safeScale < 0.6f)
            safeScale = 0.6f;
        
        if (safeScale > 1.5f)
            safeScale = 1.5f;
        
        // Font scale is now handled globally via FontGlobalScale in SetupModernStyle
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        
        g_pcSettings.menuPosX = winPos.x;
        g_pcSettings.menuPosY = winPos.y;
        
        g_pcSettings.menuWidth = winSize.x;
        g_pcSettings.menuHeight = winSize.y; // Slightly larger for better readability

        if (ImGui::BeginTabBar("MainTabs", ImGuiTabBarFlags_None))
        {
            // TAB 1: General Settings
            if (ImGui::BeginTabItem("General"))
            {
                ImGui::BeginChild("GeneralScroll", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Mouse & Camera");
                ImGui::Separator();
                changed |= ImGui::Checkbox("Enable Camera Patch", &g_pcSettings.enableCameraPatch);
                ImGui::SameLine();
                changed |= ImGui::Checkbox("Analog WASD Patch", &g_pcSettings.enableAnalogPatch);

                changed |= ImGui::SliderFloat("Horizontal Sensitivity", &g_pcSettings.camSensX, 1.0f, 100.0f, "%.1f");
                changed |= ImGui::SliderFloat("Vertical Sensitivity", &g_pcSettings.camSensY, 1.0f, 100.0f, "%.1f");
                changed |= ImGui::SliderFloat("Aim Sensitivity X", &g_pcSettings.aimSensX, 1.0f, 100.0f, "%.1f");
                changed |= ImGui::SliderFloat("Aim Sensitivity Y", &g_pcSettings.aimSensY, 1.0f, 100.0f, "%.1f");
                changed |= ImGui::SliderFloat("Camera Smoothness", &g_pcSettings.smoothness, 1.0f, 20.0f, "%.1f");

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Native Overrides");
                ImGui::Separator();
                changed |= ImGui::Checkbox("Disable Auto-Crouch", &g_pcSettings.disableNativeCrouch);
                ImGui::SameLine();
                changed |= ImGui::Checkbox("Disable Auto-Jump", &g_pcSettings.disableNativeJump);
                changed |= ImGui::Checkbox("Sprint Double Tap Boost", &g_pcSettings.enableSprintDoubleTapBoost);

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Miscellaneous");
                ImGui::Separator();
                changed |= ImGui::Checkbox("Disable Look Behind", &g_pcSettings.disableLookBehind);
                ImGui::SameLine();
                changed |= ImGui::Checkbox("Disable Pinch Zooming", &g_pcSettings.disablePinchZoom);

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // TAB 2: Custom Widgets
            if (ImGui::BeginTabItem("Widgets"))
            {
                ImGui::BeginChild(
                    "WidgetsScroll",
                    ImVec2(0, 0),
                    false,
                    ImGuiWindowFlags_AlwaysVerticalScrollbar
                );
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Widget Manager");
                ImGui::Separator();
                changed |= ImGui::SliderFloat("Global Opacity", &g_pcSettings.customWidgetOpacity, 0.0f, 1.0f, "%.2f");

                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
                
                if (ImGui::Button(
                    "Add New Button",
                    ImVec2(-1, GetButtonHeight())
                ))
                {
                    for (int i = 0; i < MAX_CUSTOM_WIDGETS; ++i)
                    {
                        if (!g_pcSettings.widgets[i].enabled)
                        {
                            g_pcSettings.widgets[i].enabled = true;
                            g_pcSettings.widgets[i].action = (int)ACTION_NONE;
                            g_pcSettings.widgets[i].type = (int)WTYPE_DEFAULT;
                            g_pcSettings.widgets[i].posX = 1500.0f;
                            g_pcSettings.widgets[i].posY = 500.0f;
                            g_pcSettings.widgets[i].size = 120.0f;
                            g_pcSettings.selectedWidget = i + 1;
                            changed = true;
                            break;
                        }
                    }
                }

                ImGui::Columns(2, "WidgetSplit", true);
                ImGui::SetColumnWidth(
                    0,
                    320.0f * g_pcSettings.menuFontScale
                );

                ImGui::Text("Active Buttons");
                ImGui::BeginChild("WidgetList", ImVec2(0, 400.0f * g_pcSettings.menuFontScale), true);
                for (int i = 0; i < MAX_CUSTOM_WIDGETS; ++i)
                {
                    if (g_pcSettings.widgets[i].enabled)
                    {
                        char label[64];
                        const char* actionNames[] = { "NONE", "VC Shoot", "Target", "Jump", "Crouch", "Sprint", "Analog/DPAD", "Prev Weapon", "Next Weapon", "Macro 1", "Macro 2", "Toggle HUD", "Walk", "Clear Aim" };
                        int act = g_pcSettings.widgets[i].action;
                        if (act < 0 || act > 13) act = 0;

                        sprintf(label, "%d: %s##%d", i + 1, actionNames[act], i);
                        if (ImGui::Selectable(label, g_pcSettings.selectedWidget == (i + 1)))
                        {
                            g_pcSettings.selectedWidget = i + 1;
                        }
                    }
                }
                ImGui::EndChild();

                ImGui::NextColumn();

                if (g_pcSettings.selectedWidget >= 1 && g_pcSettings.selectedWidget <= MAX_CUSTOM_WIDGETS)
                {
                    int idx = g_pcSettings.selectedWidget - 1;
                    ImGui::Text("Editing Button %d", idx + 1);
                    ImGui::Separator();

                    const char* actions[] = { "NONE", "VC Shoot", "Target", "Jump", "Crouch", "Sprint", "Analog/DPAD", "Prev Weapon", "Next Weapon", "Macro 1 (Auto Shoot)", "Macro 2 (Delayed)", "Toggle HUD (Show/Hide)", "Walk (Slow Mode)", "Clear Aim (Stop Aiming)" };
                    changed |= ImGui::Combo("Action", &g_pcSettings.widgets[idx].action, actions, IM_ARRAYSIZE(actions));

                    const char* types[] = { "Default (Block)", "Passthrough", "Slide-to-Activate", "Slide + Pass" };
                    if (g_pcSettings.widgets[idx].action != ACTION_DPAD)
                    {
                        changed |= ImGui::Combo("Behavior", &g_pcSettings.widgets[idx].type, types, IM_ARRAYSIZE(types));
                    }

                    if (g_pcSettings.widgets[idx].action == ACTION_DPAD)
                    {
                        changed |= ImGui::SliderFloat("Area Pos X", &g_pcSettings.widgets[idx].posX, 0.0f, 3000.0f, "%.0f");
                        changed |= ImGui::SliderFloat("Area Pos Y", &g_pcSettings.widgets[idx].posY, 0.0f, 2000.0f, "%.0f");
                        changed |= ImGui::SliderFloat("Area Width", &g_pcSettings.widgets[idx].areaW, 50.0f, 2000.0f, "%.0f");
                        changed |= ImGui::SliderFloat("Area Height", &g_pcSettings.widgets[idx].areaH, 50.0f, 2000.0f, "%.0f");
                        changed |= ImGui::SliderFloat("Analog Size", &g_pcSettings.widgets[idx].size, 20.0f, 800.0f, "%.0f");
                    }
                    else
                    {
                        changed |= ImGui::SliderFloat("Position X", &g_pcSettings.widgets[idx].posX, 0.0f, 3000.0f, "%.0f");
                        changed |= ImGui::SliderFloat("Position Y", &g_pcSettings.widgets[idx].posY, 0.0f, 2000.0f, "%.0f");
                        changed |= ImGui::SliderFloat("Button Size", &g_pcSettings.widgets[idx].size, 20.0f, 800.0f, "%.0f");
                    }

                    if (g_pcSettings.widgets[idx].action == ACTION_MACRO2)
                    {
                        changed |= ImGui::SliderFloat("Macro Delay", &g_pcSettings.widgets[idx].macroDelay, 0.0f, 5.0f, "%.2f");
                    }

                    ImGui::Spacing();
                    if (ImGui::Button(
                        "Delete This Button",
                        ImVec2(-1, GetButtonHeight())
                    ))
                    {
                        g_pcSettings.widgets[idx].enabled = false;
                        g_pcSettings.selectedWidget = 0;
                        changed = true;
                    }
                }
                else
                {
                    ImGui::TextDisabled("Select a button from the list to edit.");
                }
                ImGui::Columns(1);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::EndChild();
                ImGui::EndTabItem();
                ImGui::PopStyleVar();
            }

            // TAB 3: Visuals & HUD
            if (ImGui::BeginTabItem("Visuals"))
            {
                ImGui::BeginChild("VisualsScroll");

                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Death List (Killfeed)");
                ImGui::Separator();
                changed |= ImGui::SliderFloat("Scale", &g_pcSettings.deathListFontSize, 0.1f, 3.0f, "%.2f");
                changed |= ImGui::SliderFloat("Screen X", &g_pcSettings.deathListPosX, 0.0f, 3000.0f, "%.0f");
                changed |= ImGui::SliderFloat("Screen Y", &g_pcSettings.deathListPosY, 0.0f, 2000.0f, "%.0f");
                changed |= ImGui::SliderFloat("Line Spacing", &g_pcSettings.deathListSpacing, 0.0f, 100.0f, "%.0f");

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "TimeCycle Overrides");
                ImGui::Separator();
                RenderTimecycEditorTab();

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // TAB 4: UI Settings
            if (ImGui::BeginTabItem("Settings"))
            {
                ImGui::BeginChild("UIScroll");
            
                ImGui::TextColored(
                    ImVec4(0.4f, 0.7f, 1.0f, 1.0f),
                    "Menu Style"
                );
            
                ImGui::Separator();
            
                changed |= ImGui::SliderFloat(
                    "Font Scale",
                    &g_pcSettings.menuFontScale,
                    0.5f,
                    2.0f,
                    "%.2f"
                );
            
                changed |= ImGui::SliderFloat(
                    "Background Opacity",
                    &g_pcSettings.menuBgOpacity,
                    0.1f,
                    1.0f,
                    "%.2f"
                );
            
                changed |= ImGui::SliderFloat(
                    "Menu Rounding",
                    &g_pcSettings.menuRounding,
                    0.0f,
                    20.0f,
                    "%.1f"
                );
            
                ImGui::Spacing();
            
                ImGui::Text("Accent Color");
            
                changed |= ImGui::SliderFloat(
                    "Red",
                    &g_pcSettings.menuAccentR,
                    0.0f,
                    1.0f
                );
            
                changed |= ImGui::SliderFloat(
                    "Green",
                    &g_pcSettings.menuAccentG,
                    0.0f,
                    1.0f
                );
            
                changed |= ImGui::SliderFloat(
                    "Blue",
                    &g_pcSettings.menuAccentB,
                    0.0f,
                    1.0f
                );
            
                SetupModernStyle();
            
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();

    if (changed)
    {
        SavePCControlSettings();
    }
}
