#pragma once
#include "UE4.hpp"
#include "imgui.h"
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

// ===== Aimbot config =====
struct AimbotConfig
{
    bool enabled = false;
    int  keyBind = VK_RBUTTON;
    int  aimMethod = 1;
    bool drawFov = true;
    float fovRadius = 300.0f;
    float smoothness = 2.0f;
    float maxDistance = 200.0f;
    bool targetVictims = true;
    bool targetFamily = false;
    bool targetGrandpa = false;
    int  aimBone = 0;

    int   lastTargetsInFov = 0;
    bool  lastKeyPressed = false;
    float lastScreenDist = 0.0f;
    char  lastTargetName[64] = "none";
};

// ===== ESP config =====
struct ESPConfig
{
    bool enabled = true;

    bool showBox = true;
    bool show3DBox = false;
    bool showLine = false;
    bool showNames = true;
    bool showDistance = true;
    bool showClassName = false;

    bool showBoxGlow = true;
    int  boxGlowLayers = 4;
    float boxGlowThickness = 3.0f;

    bool showVictims = true;
    bool showFamily = true;
    bool showItems = true;
    bool showObjectives = true;

    float maxDistance = 300.0f;
    float boxAlpha = 1.0f;

    float playerHeightCm = 180.0f;
    float familyHeightCm = 190.0f;
    float itemHeightCm = 40.0f;
    float objectiveHeightCm = 60.0f;
    float grandpaHeightCm = 130.0f;

    float playerAnchorZ = -30.0f;
    float familyAnchorZ = -30.0f;
    float grandpaAnchorZ = -90.0f;
    float itemAnchorZ = 0.0f;
    float objectiveAnchorZ = 0.0f;

    float itemWorldOffsetZ = 0.0f;
    float objectiveWorldOffsetZ = 0.0f;

    float boxAspectRatio = 0.5f;
    float box3DAspect = 0.4f;

    // ===== FOV =====
    float fovBiasPercent = 112.5f;
    float fovOverride = 0.0f;   // 0 = dung cache FOV
    int   fovAxis = 0;      // 0 = horizontal, 1 = vertical
    float pixelOffsetX = 0.0f;
    float pixelOffsetY = 0.0f;
    float offCenterX = 0.0f;
    float offCenterY = 0.0f;

    ImU32 victimColor = IM_COL32(80, 220, 120, 255);
    ImU32 familyColor = IM_COL32(255, 80, 80, 255);
    ImU32 itemColor = IM_COL32(255, 220, 80, 255);
    ImU32 objColor = IM_COL32(80, 180, 255, 255);
    ImU32 lineColor = IM_COL32(255, 255, 255, 180);
};

// ===== Target data =====
struct ESPTarget
{
    const char* className;
    const char* displayName;
    int category;
    float heightOverride;
};

struct TargetInfo
{
    std::string displayName = "unknown";
    int   category = -1;
    float heightOverride = 0.0f;
};

static const ESPTarget kTargets[] = {
    // ===== ITEMS =====
    { "BP_SmallHealthPickup_C",          "Small Health",  2, 25.0f },
    { "BP_LargeHealthPickup_C",          "Large Health",  2, 45.0f },
    { "BP_Fuse_C",                       "Fuse",          2, 15.0f },
    { "BP_BloodBucket_Dressed_C",        "Blood Bucket",  2, 50.0f },
    { "BP_UnlockToolPickup_C",           "Unlock Tool",   2, 20.0f },
    { "BP_WaterValvePickup_C",           "Water Valve",   2, 20.0f },
    { "BP_TaurusKnifePickup_C",          "Knife",         2, 20.0f },
    { "BP_CarKeyPickup_C",               "Car Key",       2, 15.0f },
    { "BP_SissyPowderPickup_C",          "Sissy Powder",  2, 20.0f },
    { "BP_ShardPickup_C",                "Bone Shard",    2, 15.0f },
    { "BP_NoiseMaker_BoneChimeDuo_C",    "Bone Chime",    2, 30.0f },
    { "BP_NoiseMaker_Chicken_C",         "Chicken",       2, 40.0f },

    // ===== OBJECTIVES =====
    { "BP_BoneShardDispenser_C",              "Bone Shard Box",3, 70.0f },
    { "BP_ToolboxDressingSml_Light_C",        "Toolbox",       3, 50.0f },
    { "BP_Generator_C",                       "Generator",     3, 100.0f },
    { "BP_Fusebox_C",                         "Fusebox",       3, 80.0f },
    { "BP_WaterPump_C",                       "Water Pump",    3, 100.0f },
};

// ===== FindTarget (exact) =====
static const ESPTarget* FindTarget(const std::string& cls)
{
    for (const auto& t : kTargets)
        if (cls == t.className) return &t;
    return nullptr;
}

// ===== FindTargetInfo (pattern) =====
inline bool FindTargetInfo(const std::string& cls, TargetInfo& out)
{
    // Exact whitelist truoc
    for (const auto& t : kTargets) {
        if (cls == t.className) {
            out.displayName = t.displayName;
            out.category = t.category;
            out.heightOverride = t.heightOverride;
            return true;
        }
    }

    // Loai tru noise
    if (cls.find("Camera") != std::string::npos) return false;
    if (cls.find("Dummy") != std::string::npos) return false;
    if (cls.find("Dressing") != std::string::npos) return false;
    if (cls.find("Imposter") != std::string::npos) return false;
    if (cls.find("_MV_") != std::string::npos) return false;

    // Family
    if (cls.rfind("BP_BBQPlayerFamily", 0) == 0) {
        std::string name = cls.substr(18);
        if (name.length() >= 3 && name.substr(name.length() - 2) == "_C")
            name = name.substr(0, name.length() - 2);
        if (name.empty()) name = "Family";

        out.displayName = name;
        out.category = 1;
        out.heightOverride = 0.0f;
        return true;
    }

    // Victim
    if (cls.rfind("BP_BBQPlayerVictim", 0) == 0) {
        std::string name = cls.substr(18);
        if (name.length() >= 3 && name.substr(name.length() - 2) == "_C")
            name = name.substr(0, name.length() - 2);
        if (name.empty()) name = "Victim";

        out.displayName = name;
        out.category = 0;
        out.heightOverride = 0.0f;
        return true;
    }

    return false;
}

inline float DefaultHeightFor(int cat, const ESPConfig& cfg) {
    switch (cat) {
    case 0: return cfg.playerHeightCm;
    case 1: return cfg.familyHeightCm;
    case 2: return cfg.itemHeightCm;
    default:return cfg.objectiveHeightCm;
    }
}

inline ImU32 ApplyAlpha(ImU32 color, float alpha)
{
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    int r = (color >> IM_COL32_R_SHIFT) & 0xFF;
    int g = (color >> IM_COL32_G_SHIFT) & 0xFF;
    int b = (color >> IM_COL32_B_SHIFT) & 0xFF;
    int a = (int)(((color >> IM_COL32_A_SHIFT) & 0xFF) * alpha);
    return IM_COL32(r, g, b, a);
}

inline void DrawBoxGlow(ImDrawList* draw, float leftX, float topY,
    float rightX, float bottomY,
    ImU32 color, float baseAlpha,
    int layers, float thickness)
{
    if (layers < 1) layers = 1;
    if (layers > 6) layers = 6;

    for (int i = layers; i >= 1; i--) {
        float expand = (float)i * thickness;
        float a = baseAlpha * (0.08f + 0.12f * (layers - i + 1) / layers);
        ImU32 glowCol = ApplyAlpha(color, a);
        draw->AddRect(
            ImVec2(leftX - expand, topY - expand),
            ImVec2(rightX + expand, bottomY + expand),
            glowCol, 0.f, 0, thickness + 1.0f);
    }
}

inline void Draw3DBox(ImDrawList* draw, const FVector& center,
    const FVector& extent,
    const FMinimalViewInfo& pov,
    int w, int h, ImU32 color,
    float fovBias, float offX, float offY,
    float pxOffX, float pxOffY,
    float fovOverride, int fovAxis)
{
    FVector corners[8]{};
    for (int i = 0; i < 8; i++) {
        corners[i].X = center.X + ((i & 1) ? +extent.X : -extent.X);
        corners[i].Y = center.Y + ((i & 2) ? +extent.Y : -extent.Y);
        corners[i].Z = center.Z + ((i & 4) ? +extent.Z : -extent.Z);
    }

    FVector2D scr[8]{};
    bool ok[8]{};
    for (int i = 0; i < 8; i++) {
        ok[i] = WorldToScreen(corners[i], pov, w, h, scr[i],
            fovBias, offX, offY, fovOverride, fovAxis);
        if (ok[i]) { scr[i].X += pxOffX; scr[i].Y += pxOffY; }
    }

    static const int edges[12][2] = {
        {0,1},{1,3},{3,2},{2,0},
        {4,5},{5,7},{7,6},{6,4},
        {0,4},{1,5},{2,6},{3,7},
    };

    for (int i = 0; i < 12; i++) {
        int a = edges[i][0], b = edges[i][1];
        if (!ok[a] || !ok[b]) continue;
        draw->AddLine(ImVec2(scr[a].X, scr[a].Y),
            ImVec2(scr[b].X, scr[b].Y), color, 1.5f);
    }
}

// ===== RENDER ESP =====
inline void RenderESP(ESPConfig& cfg, ImDrawList* draw, int w, int h)
{
    if (!cfg.enabled) return;

    FMinimalViewInfo pov{};
    uintptr_t localPC = 0;
    if (!GetCameraPOV(pov, localPC)) return;

    uintptr_t localPawn = GetLocalPawn(localPC);
    FVector pawnLoc = GetActorLocation(localPawn);

    auto actors = GetActors();

    struct Found {
        uintptr_t actor = 0;
        FVector   world{};
        float     distM = 0.0f;
        TargetInfo info{};
        std::string cls;
    };
    std::vector<Found> found;

    for (uintptr_t actor : actors)
    {
        std::string cls = GetClassName(actor);
        TargetInfo tgt{};
        if (!FindTargetInfo(cls, tgt)) continue;
        if (actor == localPawn) continue;

        int cat = tgt.category;
        if (cat == 0 && !cfg.showVictims)    continue;
        if (cat == 1 && !cfg.showFamily)     continue;
        if (cat == 2 && !cfg.showItems)      continue;
        if (cat == 3 && !cfg.showObjectives) continue;

        FVector world = GetActorLocation(actor);
        if (world.X == 0 && world.Y == 0 && world.Z == 0) continue;
        if (fabsf(world.Z) > 10000.0f) continue;

        FVector base = (pawnLoc.X || pawnLoc.Y || pawnLoc.Z) ? pawnLoc : pov.Location;
        if (fabsf(world.Z - base.Z) > 500.0f) continue;

        float dx = world.X - base.X, dy = world.Y - base.Y, dz = world.Z - base.Z;
        float distM = sqrtf(dx * dx + dy * dy + dz * dz) / 100.0f;
        if (distM > cfg.maxDistance) continue;

        bool dup = false;
        for (const auto& f : found) {
            if (f.cls != cls) continue;
            float ddx = f.world.X - world.X, ddy = f.world.Y - world.Y, ddz = f.world.Z - world.Z;
            if (ddx * ddx + ddy * ddy + ddz * ddz < 100.0f * 100.0f) { dup = true; break; }
        }
        if (dup) continue;

        Found f;
        f.actor = actor;
        f.world = world;
        f.distM = distM;
        f.info = tgt;
        f.cls = cls;
        found.push_back(f);
    }

    for (const auto& f : found)
    {
        int cat = f.info.category;
        const FVector& world = f.world;
        float distM = f.distM;

        float worldOffZ = 0.0f;
        float worldHeightCm = (f.info.heightOverride > 0.5f)
            ? f.info.heightOverride
            : DefaultHeightFor(cat, cfg);

        bool isGrandpa = (f.cls == "BP_BBQGrandpa_C" ||
            f.cls.find("Grandpa") != std::string::npos);
        if (isGrandpa) {
            worldOffZ = cfg.grandpaAnchorZ;
            worldHeightCm = cfg.grandpaHeightCm;
        }
        else if (cat == 0) {
            worldOffZ = cfg.playerAnchorZ;
        }
        else if (cat == 1) {
            worldOffZ = cfg.familyAnchorZ;
        }
        else if (cat == 2) {
            worldOffZ = cfg.itemWorldOffsetZ + cfg.itemAnchorZ;
        }
        else {
            worldOffZ = cfg.objectiveWorldOffsetZ + cfg.objectiveAnchorZ;
        }

        ImU32 color;
        switch (cat) {
        case 0: color = cfg.victimColor; break;
        case 1: color = cfg.familyColor; break;
        case 2: color = cfg.itemColor;   break;
        default: color = cfg.objColor;   break;
        }
        ImU32 drawColor = ApplyAlpha(color, cfg.boxAlpha);

        // ===== 3D BOX =====
        if (cfg.show3DBox) {
            FVector center = world;
            center.Z += worldOffZ + worldHeightCm * 0.5f;
            FVector extent;
            extent.Z = worldHeightCm * 0.5f;
            extent.X = worldHeightCm * cfg.box3DAspect * 0.5f;
            extent.Y = worldHeightCm * cfg.box3DAspect * 0.5f;
            Draw3DBox(draw, center, extent, pov, w, h, drawColor,
                cfg.fovBiasPercent, cfg.offCenterX, cfg.offCenterY,
                cfg.pixelOffsetX, cfg.pixelOffsetY,
                cfg.fovOverride, cfg.fovAxis);
        }

        // ===== 2D BOX =====
        FVector bottom = world; bottom.Z += worldOffZ;
        FVector top = bottom; top.Z += worldHeightCm;

        FVector2D sBottom{}, sTop{};
        if (!WorldToScreen(bottom, pov, w, h, sBottom,
            cfg.fovBiasPercent, cfg.offCenterX, cfg.offCenterY,
            cfg.fovOverride, cfg.fovAxis) ||
            !WorldToScreen(top, pov, w, h, sTop,
                cfg.fovBiasPercent, cfg.offCenterX, cfg.offCenterY,
                cfg.fovOverride, cfg.fovAxis))
            continue;

        sBottom.X += cfg.pixelOffsetX; sBottom.Y += cfg.pixelOffsetY;
        sTop.X += cfg.pixelOffsetX; sTop.Y += cfg.pixelOffsetY;

        float boxH = fabsf(sBottom.Y - sTop.Y);
        if (boxH < 4.0f)     boxH = 4.0f;
        if (boxH > h * 0.8f) boxH = h * 0.8f;
        float boxW = boxH * cfg.boxAspectRatio;

        float centerX = (sBottom.X + sTop.X) * 0.5f;
        float topY = sTop.Y, bottomY = sBottom.Y;
        if (topY > bottomY) std::swap(topY, bottomY);

        float leftX = centerX - boxW * 0.5f;
        float rightX = centerX + boxW * 0.5f;

        if (cfg.showBoxGlow && cfg.showBox) {
            DrawBoxGlow(draw, leftX, topY, rightX, bottomY,
                color, cfg.boxAlpha, cfg.boxGlowLayers, cfg.boxGlowThickness);
        }
        if (cfg.showBox && !cfg.show3DBox) {
            draw->AddRect(ImVec2(leftX, topY), ImVec2(rightX, bottomY), drawColor, 0.f, 0, 1.5f);
        }
        if (cfg.showLine) {
            ImU32 lineCol = ApplyAlpha(cfg.lineColor, cfg.boxAlpha);
            draw->AddLine(ImVec2(w * 0.5f, (float)h),
                ImVec2(centerX, bottomY), lineCol, 1.5f);
        }

        if (cfg.showNames || cfg.showDistance) {
            const char* name = cfg.showClassName ? f.cls.c_str() : f.info.displayName.c_str();
            char label[128]{};
            if (cfg.showNames && cfg.showDistance)
                snprintf(label, sizeof(label), "%s [%.0fm]", name, distM);
            else if (cfg.showNames)
                snprintf(label, sizeof(label), "%s", name);
            else
                snprintf(label, sizeof(label), "%.0fm", distM);

            ImVec2 ts = ImGui::CalcTextSize(label);
            draw->AddText(ImVec2(centerX - ts.x * 0.5f + 1, topY - ts.y - 4),
                IM_COL32(0, 0, 0, 255), label);
            draw->AddText(ImVec2(centerX - ts.x * 0.5f, topY - ts.y - 5),
                (cat < 2 ? IM_COL32(255, 255, 255, 255) : drawColor), label);
        }
    }
}