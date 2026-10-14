// ============================================================
// TSCM External ESP + Aimbot
// ============================================================
#include <cstdio>
#include <cstring>
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <thread>
#include <atomic>
#include <chrono>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "Offsets.hpp"
#include "Memory.hpp"
#include "UE4.hpp"
#include "ESP.hpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_pRTV = nullptr;
static HWND                    g_hWnd = nullptr;

static int g_width = 1920;
static int g_height = 1080;
static std::atomic<bool> g_running{ true };

static ESPConfig    g_cfg;
static AimbotConfig g_aim;
static bool g_menuOpen = true;
static DWORD g_gamePid = 0;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

// ===== ForceForeground =====
static void ForceForeground(HWND hWnd)
{
    HWND fg = GetForegroundWindow();
    if (fg == hWnd) { SetFocus(hWnd); return; }

    DWORD fgThread = GetWindowThreadProcessId(fg, nullptr);
    DWORD targetThread = GetWindowThreadProcessId(hWnd, nullptr);
    DWORD myThread = GetCurrentThreadId();

    if (fgThread != myThread)     AttachThreadInput(myThread, fgThread, TRUE);
    if (targetThread != myThread) AttachThreadInput(myThread, targetThread, TRUE);

    SetForegroundWindow(hWnd);
    SetActiveWindow(hWnd);
    SetFocus(hWnd);
    BringWindowToTop(hWnd);

    if (targetThread != myThread) AttachThreadInput(myThread, targetThread, FALSE);
    if (fgThread != myThread)     AttachThreadInput(myThread, fgThread, FALSE);
}

// ===== Find game window =====
static BOOL CALLBACK EnumWindowsFindGame(HWND hwnd, LPARAM lParam)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == g_gamePid && IsWindowVisible(hwnd)) {
        char title[256]{};
        GetWindowTextA(hwnd, title, sizeof(title));
        if (strlen(title) > 0) {
            *(HWND*)lParam = hwnd;
            return FALSE;
        }
    }
    return TRUE;
}

static HWND FindGameWindow()
{
    if (!g_gamePid) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32W pe{};
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(snap, &pe)) {
            do {
                if (!_wcsicmp(pe.szExeFile, L"BBQClient-Win64-Shipping.exe")) {
                    g_gamePid = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
    }
    if (!g_gamePid) return nullptr;

    HWND result = nullptr;
    EnumWindows(EnumWindowsFindGame, (LPARAM)&result);
    return result;
}

// ===== DX11 =====
static bool InitD3D()
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = g_width;
    sd.BufferDesc.Height = g_height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL fl;
    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        levels, 2, D3D11_SDK_VERSION,
        &sd, &g_pSwapChain, &g_pDevice, &fl, &g_pContext);
    if (FAILED(hr)) return false;

    ID3D11Texture2D* bb = nullptr;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
    g_pDevice->CreateRenderTargetView(bb, nullptr, &g_pRTV);
    bb->Release();
    return true;
}

static void CleanupD3D()
{
    if (g_pRTV) { g_pRTV->Release();       g_pRTV = nullptr; }
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pContext) { g_pContext->Release();   g_pContext = nullptr; }
    if (g_pDevice) { g_pDevice->Release();    g_pDevice = nullptr; }
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pDevice && wParam != SIZE_MINIMIZED) {
            if (g_pRTV) { g_pRTV->Release(); g_pRTV = nullptr; }
            g_pSwapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            ID3D11Texture2D* bb = nullptr;
            g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
            g_pDevice->CreateRenderTargetView(bb, nullptr, &g_pRTV);
            bb->Release();
            g_width = LOWORD(lParam);
            g_height = HIWORD(lParam);
        }
        return 0;
    case WM_SETCURSOR:
        if (g_menuOpen) {
            SetCursor(LoadCursorW(nullptr, IDC_ARROW));
            return TRUE;
        }
        break;
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ===== Mouse move =====
static void MoveMouse(int dx, int dy, int method)
{
    switch (method) {
    case 0: {
        INPUT inp{};
        inp.type = INPUT_MOUSE;
        inp.mi.dx = dx;
        inp.mi.dy = dy;
        inp.mi.dwFlags = MOUSEEVENTF_MOVE;
        SendInput(1, &inp, sizeof(INPUT));
    } break;
    case 1: {
        mouse_event(MOUSEEVENTF_MOVE, (DWORD)dx, (DWORD)dy, 0, 0);
    } break;
    case 2: {
        POINT pt;
        GetCursorPos(&pt);
        SetCursorPos(pt.x + dx, pt.y + dy);
    } break;
    }
}

// ===== Aimbot =====
static void RunAimbot()
{
    g_aim.lastTargetsInFov = 0;
    g_aim.lastKeyPressed = false;
    g_aim.lastScreenDist = 0.0f;
    strcpy_s(g_aim.lastTargetName, "none");

    if (!g_aim.enabled) return;

    bool keyDown = (GetAsyncKeyState(g_aim.keyBind) & 0x8000) != 0;
    g_aim.lastKeyPressed = keyDown;
    if (!keyDown) return;

    FMinimalViewInfo pov{};
    uintptr_t localPC = 0;
    if (!GetCameraPOV(pov, localPC)) return;

    uintptr_t localPawn = GetLocalPawn(localPC);
    if (!localPawn) return;
    FVector localLoc = GetActorLocation(localPawn);

    auto actors = GetActors();

    float cx = g_width * 0.5f;
    float cy = g_height * 0.5f;

    uintptr_t bestActor = 0;
    float bestDist = FLT_MAX;
    FVector2D bestScreen{};
    std::string bestName;

    int targetsInFov = 0;

    for (uintptr_t actor : actors) {
        if (actor == localPawn) continue;
        std::string cls = GetClassName(actor);
        TargetInfo tgt{};
        if (!FindTargetInfo(cls, tgt)) continue;
        int cat = tgt.category;

        bool isGrandpa = (cls.find("Grandpa") != std::string::npos);
        if (cat == 0 && !g_aim.targetVictims) continue;
        if (cat == 1 && isGrandpa && !g_aim.targetGrandpa) continue;
        if (cat == 1 && !isGrandpa && !g_aim.targetFamily) continue;
        if (cat >= 2) continue;

        FVector world = GetActorLocation(actor);
        if (world.X == 0 && world.Y == 0 && world.Z == 0) continue;

        float dx = world.X - localLoc.X;
        float dy = world.Y - localLoc.Y;
        float dz = world.Z - localLoc.Z;
        float distM = sqrtf(dx * dx + dy * dy + dz * dz) / 100.0f;
        if (distM > g_aim.maxDistance) continue;

        FVector aimPos = world;
        if (g_aim.aimBone == 0)      aimPos.Z += 165.0f;
        else if (g_aim.aimBone == 1) aimPos.Z += 120.0f;
        else                          aimPos.Z += 80.0f;

        FVector2D screen{};
        if (!WorldToScreen(aimPos, pov, g_width, g_height, screen,
            g_cfg.fovBiasPercent, 0.0f, 0.0f,
            g_cfg.fovOverride, g_cfg.fovAxis)) continue;

        float sx = screen.X - cx;
        float sy = screen.Y - cy;
        float screenDist = sqrtf(sx * sx + sy * sy);
        if (screenDist > g_aim.fovRadius) continue;

        targetsInFov++;

        if (screenDist < bestDist) {
            bestDist = screenDist;
            bestScreen = screen;
            bestActor = actor;
            bestName = cls;
        }
    }

    g_aim.lastTargetsInFov = targetsInFov;

    if (bestActor) {
        g_aim.lastScreenDist = bestDist;
        strcpy_s(g_aim.lastTargetName, bestName.c_str());

        float dx = (bestScreen.X - cx) / g_aim.smoothness;
        float dy = (bestScreen.Y - cy) / g_aim.smoothness;

        if (fabsf(dx) < 0.5f && fabsf(dy) < 0.5f) return;

        MoveMouse((int)dx, (int)dy, g_aim.aimMethod);
    }
}

static void DrawFovCircle()
{
    if (!g_aim.enabled || !g_aim.drawFov) return;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    dl->AddCircle(ImVec2(g_width * 0.5f, g_height * 0.5f),
        g_aim.fovRadius, IM_COL32(255, 255, 255, 120), 64, 1.5f);
}

// ============================================================
// MENU — chỉ có 2 tab: ESP và Aimbot
// ============================================================
static void DrawMenu()
{
    // ★ Tăng width để 2 tab nhìn cân đối
    ImGui::SetNextWindowSize(ImVec2(400, 550), ImGuiCond_FirstUseEver);
    ImGui::Begin("TSCM ESP", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    // ★ Push padding NGANG lớn để 2 tab kéo dài fill menu
    // FramePadding.y giữ nguyên (3) để slider/checkbox không bị phình
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(60.0f, 3.0f));

    if (ImGui::BeginTabBar("MainTabs", ImGuiTabBarFlags_FittingPolicyResizeDown))
    {
        // ============================================================
        // TAB ESP
        // ============================================================
        if (ImGui::BeginTabItem("ESP"))
        {
            ImGui::Checkbox("Enable", &g_cfg.enabled);
            ImGui::Separator();

            ImGui::Text("Filters");
            ImGui::Checkbox("Victims", &g_cfg.showVictims);
            ImGui::Checkbox("Family", &g_cfg.showFamily);
            ImGui::Checkbox("Items", &g_cfg.showItems);
            ImGui::Checkbox("Objectives", &g_cfg.showObjectives);
            ImGui::Separator();

            ImGui::Text("Visual");
            ImGui::Checkbox("Box (2D)", &g_cfg.showBox);
            ImGui::Checkbox("Box (3D)", &g_cfg.show3DBox);
            ImGui::Checkbox("Line", &g_cfg.showLine);
            ImGui::Checkbox("Names", &g_cfg.showNames);
            ImGui::Checkbox("Distance", &g_cfg.showDistance);
            ImGui::Separator();

            ImGui::Text("Box Glow");
            ImGui::Checkbox("Glow Effect", &g_cfg.showBoxGlow);
            ImGui::SliderInt("Glow Layers", &g_cfg.boxGlowLayers, 1, 6);
            ImGui::SliderFloat("Glow Thickness", &g_cfg.boxGlowThickness, 1.0f, 8.0f);
            ImGui::Separator();

            ImGui::SliderFloat("Box Alpha", &g_cfg.boxAlpha, 0.05f, 1.0f, "%.2f");
            ImGui::SliderFloat("Max Dist (m)", &g_cfg.maxDistance, 10.f, 2000.f);
            ImGui::Separator();

            ImGui::Text("Colors");
            float vc[4] = { ((g_cfg.victimColor >> IM_COL32_R_SHIFT) & 0xFF) / 255.f, ((g_cfg.victimColor >> IM_COL32_G_SHIFT) & 0xFF) / 255.f, ((g_cfg.victimColor >> IM_COL32_B_SHIFT) & 0xFF) / 255.f, ((g_cfg.victimColor >> IM_COL32_A_SHIFT) & 0xFF) / 255.f };
            if (ImGui::ColorEdit4("Victim", vc)) g_cfg.victimColor = IM_COL32(vc[0] * 255, vc[1] * 255, vc[2] * 255, vc[3] * 255);
            float fc[4] = { ((g_cfg.familyColor >> IM_COL32_R_SHIFT) & 0xFF) / 255.f, ((g_cfg.familyColor >> IM_COL32_G_SHIFT) & 0xFF) / 255.f, ((g_cfg.familyColor >> IM_COL32_B_SHIFT) & 0xFF) / 255.f, ((g_cfg.familyColor >> IM_COL32_A_SHIFT) & 0xFF) / 255.f };
            if (ImGui::ColorEdit4("Family", fc)) g_cfg.familyColor = IM_COL32(fc[0] * 255, fc[1] * 255, fc[2] * 255, fc[3] * 255);
            float ic[4] = { ((g_cfg.itemColor >> IM_COL32_R_SHIFT) & 0xFF) / 255.f, ((g_cfg.itemColor >> IM_COL32_G_SHIFT) & 0xFF) / 255.f, ((g_cfg.itemColor >> IM_COL32_B_SHIFT) & 0xFF) / 255.f, ((g_cfg.itemColor >> IM_COL32_A_SHIFT) & 0xFF) / 255.f };
            if (ImGui::ColorEdit4("Item", ic)) g_cfg.itemColor = IM_COL32(ic[0] * 255, ic[1] * 255, ic[2] * 255, ic[3] * 255);
            float oc[4] = { ((g_cfg.objColor >> IM_COL32_R_SHIFT) & 0xFF) / 255.f, ((g_cfg.objColor >> IM_COL32_G_SHIFT) & 0xFF) / 255.f, ((g_cfg.objColor >> IM_COL32_B_SHIFT) & 0xFF) / 255.f, ((g_cfg.objColor >> IM_COL32_A_SHIFT) & 0xFF) / 255.f };
            if (ImGui::ColorEdit4("Objective", oc)) g_cfg.objColor = IM_COL32(oc[0] * 255, oc[1] * 255, oc[2] * 255, oc[3] * 255);

            ImGui::EndTabItem();
        }

        // ============================================================
        // TAB AIMBOT
        // ============================================================
        if (ImGui::BeginTabItem("Aimbot"))
        {
            ImGui::Checkbox("Enable Aimbot", &g_aim.enabled);
            ImGui::Separator();

            ImGui::Text("Key Bind");
            const char* keys[] = { "Right Mouse", "Left Mouse", "Shift", "Ctrl", "Alt", "Mouse 4", "Mouse 5", "E", "Q", "F", "C", "V" };
            const int keyVals[] = { VK_RBUTTON, VK_LBUTTON, VK_SHIFT, VK_CONTROL, VK_MENU, VK_XBUTTON1, VK_XBUTTON2, 'E', 'Q', 'F', 'C', 'V' };
            static int keyIdx = 0;
            if (ImGui::Combo("Key", &keyIdx, keys, IM_ARRAYSIZE(keys)))
                g_aim.keyBind = keyVals[keyIdx];
            ImGui::Separator();

            ImGui::Text("Method");
            ImGui::RadioButton("SendInput", &g_aim.aimMethod, 0); ImGui::SameLine();
            ImGui::RadioButton("mouse_event", &g_aim.aimMethod, 1); ImGui::SameLine();
            ImGui::RadioButton("SetCursorPos", &g_aim.aimMethod, 2);
            ImGui::Separator();

            ImGui::Checkbox("Draw FOV Circle", &g_aim.drawFov);
            ImGui::SliderFloat("FOV Radius (px)", &g_aim.fovRadius, 20.f, 800.f);
            ImGui::SliderFloat("Smoothness", &g_aim.smoothness, 1.0f, 20.0f, "%.1f");
            ImGui::SliderFloat("Max Dist (m)", &g_aim.maxDistance, 5.f, 500.f);
            ImGui::Separator();

            ImGui::Text("Target");
            ImGui::Checkbox("Victims", &g_aim.targetVictims);
            ImGui::Checkbox("Family", &g_aim.targetFamily);
            ImGui::Checkbox("Grandpa", &g_aim.targetGrandpa);
            ImGui::Separator();

            ImGui::Text("Aim Bone");
            ImGui::RadioButton("Head", &g_aim.aimBone, 0); ImGui::SameLine();
            ImGui::RadioButton("Chest", &g_aim.aimBone, 1); ImGui::SameLine();
            ImGui::RadioButton("Pelvis", &g_aim.aimBone, 2);
            ImGui::Separator();

            ImGui::TextColored(ImVec4(0.6f, 1, 0.6f, 1), "--- Aimbot Debug ---");
            ImGui::Text("Key Pressed: %s", g_aim.lastKeyPressed ? "YES" : "no");
            ImGui::Text("Targets in FOV: %d", g_aim.lastTargetsInFov);
            ImGui::Text("Locked target: %s", g_aim.lastTargetName);
            ImGui::Text("Screen dist: %.1f px", g_aim.lastScreenDist);

            ImGui::EndTabItem();
        }

        // ============================================================
        // TAB SIZES — ẨN KHỎI MENU (giữ code để bật lại sau)
        // Mở comment block này khi cần test/tune
        // ============================================================
        /*
        if (ImGui::BeginTabItem("Sizes"))
        {
            ImGui::Text("Heights (cm)");
            ImGui::SliderFloat("Player H",    &g_cfg.playerHeightCm,    50.f, 250.f);
            ImGui::SliderFloat("Family H",    &g_cfg.familyHeightCm,    50.f, 250.f);
            ImGui::SliderFloat("Grandpa H",   &g_cfg.grandpaHeightCm,   50.f, 250.f);
            ImGui::SliderFloat("Item H",      &g_cfg.itemHeightCm,      10.f, 200.f);
            ImGui::SliderFloat("Objective H", &g_cfg.objectiveHeightCm, 10.f, 300.f);
            ImGui::Separator();

            ImGui::Text("Anchor Z Offset (cm)");
            ImGui::SliderFloat("Player Anchor",   &g_cfg.playerAnchorZ,    -150.f, 100.f);
            ImGui::SliderFloat("Family Anchor",   &g_cfg.familyAnchorZ,    -150.f, 100.f);
            ImGui::SliderFloat("Grandpa Anchor",  &g_cfg.grandpaAnchorZ,   -200.f, 100.f);
            ImGui::SliderFloat("Item Anchor",     &g_cfg.itemAnchorZ,      -100.f, 100.f);
            ImGui::SliderFloat("Objective Anchor",&g_cfg.objectiveAnchorZ, -100.f, 100.f);
            ImGui::Separator();

            ImGui::Text("Box Aspect");
            ImGui::SliderFloat("2D Aspect", &g_cfg.boxAspectRatio, 0.2f, 1.5f);
            ImGui::SliderFloat("3D Aspect", &g_cfg.box3DAspect,    0.2f, 1.5f);

            ImGui::EndTabItem();
        }
        */

        // ============================================================
        // TAB FOV TUNE — ẨN KHỎI MENU (giữ code để bật lại sau)
        // Mở comment block này khi cần test/tune
        // ============================================================
        /*
        if (ImGui::BeginTabItem("FOV Tune"))
        {
            ImGui::TextColored(ImVec4(0.6f,1,0.6f,1), "=== FOV Axis ===");
            ImGui::RadioButton("Horizontal", &g_cfg.fovAxis, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Vertical", &g_cfg.fovAxis, 1);
            ImGui::Separator();

            ImGui::Text("FOV Bias: %.1f%%", g_cfg.fovBiasPercent);
            ImGui::SliderFloat("##fovbias", &g_cfg.fovBiasPercent, 50.f, 200.f, "%.1f%%");
            ImGui::Separator();

            ImGui::Text("FOV Override: %.0f (0 = auto)", g_cfg.fovOverride);
            ImGui::SliderFloat("##fovoverride", &g_cfg.fovOverride, 0.f, 150.f, "%.0f");
            ImGui::Separator();

            ImGui::Text("OffCenter (NDC)");
            ImGui::SliderFloat("OffCenter X", &g_cfg.offCenterX, -0.3f, 0.3f);
            ImGui::SliderFloat("OffCenter Y", &g_cfg.offCenterY, -0.3f, 0.3f);
            ImGui::Separator();

            ImGui::Text("Pixel Offset");
            ImGui::SliderFloat("Pixel X", &g_cfg.pixelOffsetX, -100.f, 100.f);
            ImGui::SliderFloat("Pixel Y", &g_cfg.pixelOffsetY, -100.f, 100.f);
            ImGui::Separator();

            if (ImGui::Button("Reset FOV")) {
                g_cfg.fovBiasPercent = 112.5f;
                g_cfg.fovOverride = 0.0f;
                g_cfg.fovAxis = 0;
                g_cfg.offCenterX = 0.0f; g_cfg.offCenterY = 0.0f;
                g_cfg.pixelOffsetX = 0.0f; g_cfg.pixelOffsetY = 0.0f;
            }
            ImGui::EndTabItem();
        }
        */

        ImGui::EndTabBar();
    }

    ImGui::PopStyleVar();   // Pop FramePadding

    ImGui::Separator();
    ImGui::Text("INSERT: menu  |  DEL: unload");
    ImGui::End();
}

// ===== MAIN =====
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    if (!Memory::Attach(L"BBQClient-Win64-Shipping.exe")) {
        MessageBoxW(nullptr, L"Khong tim thay BBQClient-Win64-Shipping.exe", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    printf("[INIT] Attached. Base = %p\n", (void*)Memory::baseAddress);

    g_width = GetSystemMetrics(SM_CXSCREEN);
    g_height = GetSystemMetrics(SM_CYSCREEN);
    printf("[INIT] Screen = %dx%d\n", g_width, g_height);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"TSCM_ESP_Overlay";
    RegisterClassExW(&wc);

    g_hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        wc.lpszClassName, L"TSCM ESP",
        WS_POPUP, 0, 0, g_width, g_height,
        nullptr, nullptr, wc.hInstance, nullptr);

    SetLayeredWindowAttributes(g_hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(g_hWnd, SW_SHOW);

    SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, g_width, g_height,
        SWP_SHOWWINDOW | SWP_NOACTIVATE);

    if (!InitD3D()) {
        MessageBoxW(nullptr, L"Khong khoi tao duoc D3D11", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX11_Init(g_pDevice, g_pContext);

    bool lastInsert = false;

    while (g_running)
    {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) g_running = false;
        }

        // ===== INSERT toggle menu =====
        bool insert = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
        if (insert && !lastInsert) {
            g_menuOpen = !g_menuOpen;
            LONG ex = GetWindowLongW(g_hWnd, GWL_EXSTYLE);

            if (g_menuOpen) {
                ex &= ~WS_EX_TRANSPARENT;
                SetWindowLongW(g_hWnd, GWL_EXSTYLE, ex);
                ClipCursor(nullptr);
                ShowCursor(TRUE);
                ForceForeground(g_hWnd);
                printf("[MENU] Opened\n");
            }
            else {
                ex |= WS_EX_TRANSPARENT;
                SetWindowLongW(g_hWnd, GWL_EXSTYLE, ex);
                ShowCursor(FALSE);

                HWND game = FindGameWindow();
                if (game) {
                    SetForegroundWindow(game);
                    SetFocus(game);
                }
                printf("[MENU] Closed\n");
            }
        }
        lastInsert = insert;

        if (g_menuOpen) {
            CURSORINFO ci{};
            ci.cbSize = sizeof(ci);
            if (GetCursorInfo(&ci) && !(ci.flags & CURSOR_SHOWING)) ShowCursor(TRUE);
        }

        // ===== DELETE unload =====
        static bool lastDel = false;
        bool del = (GetAsyncKeyState(VK_DELETE) & 0x8000) != 0;
        if (del && !lastDel) {
            printf("[UNLOAD] Exiting...\n");
            g_running = false;
            PostQuitMessage(0);
        }
        lastDel = del;

        // ===== AIMBOT =====
        if (!g_menuOpen) {
            RunAimbot();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (g_cfg.enabled) {
            ImDrawList* dl = ImGui::GetBackgroundDrawList();
            RenderESP(g_cfg, dl, g_width, g_height);
        }

        DrawFovCircle();

        if (g_menuOpen) DrawMenu();
        ImGui::Render();

        const float clear[4] = { 0, 0, 0, 0 };
        g_pContext->OMSetRenderTargets(1, &g_pRTV, nullptr);
        g_pContext->ClearRenderTargetView(g_pRTV, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(0, 0);

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupD3D();
    DestroyWindow(g_hWnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    if (Memory::hProcess) CloseHandle(Memory::hProcess);

    return 0;
}