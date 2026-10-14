#pragma once
#include "Offsets.hpp"
#include "Memory.hpp"
#include <string>
#include <vector>
#include <cmath>

// ===== Math =====
struct FVector { float X, Y, Z; };
struct FRotator { float Pitch, Yaw, Roll; };
struct FVector2D { float X, Y; };

struct FMinimalViewInfo
{
    FVector   Location;
    FRotator  Rotation;
    float     FOV;
    float     DesiredFOV;
    float     OrthoWidth;
    float     OrthoNear;
    float     OrthoFar;
    float     AspectRatio;
};

// ===== FName =====
inline std::string ReadFName(uint32_t id)
{
    if (!id) return "";
    uintptr_t pool = Memory::baseAddress + Offsets::GNames;
    if (!Memory::IsValid(pool)) return "";

    uint32_t blockIdx = id >> 16;
    uint32_t offset = id & 0xFFFF;

    uintptr_t block = Memory::Read<uintptr_t>(pool + Offsets::FNamePool_Blocks + blockIdx * 8);
    if (!Memory::IsValid(block)) return "";

    uintptr_t entry = block + offset * Offsets::FNameEntryStride;
    uint16_t  header = Memory::Read<uint16_t>(entry);
    bool isWide = (header & 1) != 0;
    int  len = header >> 6;
    if (len <= 0 || len > 255) return "";

    char buffer[256]{};
    if (isWide) {
        wchar_t wbuf[256]{};
        Memory::ReadBytes(entry + 2, wbuf, len * sizeof(wchar_t));
        for (int i = 0; i < len && i < 255; i++) buffer[i] = (char)(wbuf[i] & 0xFF);
        return std::string(buffer);
    }
    else {
        Memory::ReadBytes(entry + 2, buffer, len);
        return std::string(buffer, len);
    }
}

inline std::string GetObjectName(uintptr_t object)
{
    if (!Memory::IsValid(object)) return "";
    uint32_t nameId = Memory::Read<uint32_t>(object + Offsets::UObject_NamePrivate);
    return ReadFName(nameId);
}

inline std::string GetClassName(uintptr_t object)
{
    if (!Memory::IsValid(object)) return "";
    uintptr_t uclass = Memory::Read<uintptr_t>(object + Offsets::UObject_ClassPrivate);
    if (!Memory::IsValid(uclass)) return "";
    return GetObjectName(uclass);
}

inline FVector GetActorLocation(uintptr_t actor)
{
    FVector loc{};
    if (!Memory::IsValid(actor)) return loc;

    uintptr_t root = Memory::Read<uintptr_t>(actor + Offsets::AActor_RootComponent);
    if (!Memory::IsValid(root)) return loc;

    Memory::Read(root + 0x100, loc);
    if (loc.X == 0 && loc.Y == 0 && loc.Z == 0) {
        Memory::Read(root + Offsets::USceneComponent_RelativeLocation, loc);
    }
    return loc;
}

inline bool IsPrimitiveComponent(uintptr_t comp)
{
    if (!Memory::IsValid(comp)) return false;
    if ((comp & 0x7) != 0) return false;

    uintptr_t uclass = Memory::Read<uintptr_t>(comp + Offsets::UObject_ClassPrivate);
    if (!Memory::IsValid(uclass)) return false;

    std::string cls = GetObjectName(uclass);
    if (cls.empty()) return false;

    if (cls.find("MeshComponent") != std::string::npos) return true;
    if (cls.find("ShapeComponent") != std::string::npos) return true;
    if (cls.find("CapsuleComponent") != std::string::npos) return true;
    if (cls.find("SphereComponent") != std::string::npos) return true;
    if (cls.find("BoxComponent") != std::string::npos) return true;
    if (cls.find("DecalComponent") != std::string::npos) return true;
    if (cls.find("BillboardComponent") != std::string::npos) return true;

    return false;
}

inline uintptr_t GetMeshComponent(uintptr_t actor)
{
    if (!Memory::IsValid(actor)) return 0;

    uintptr_t m1 = Memory::Read<uintptr_t>(actor + Offsets::ACharacter_Mesh);
    if (IsPrimitiveComponent(m1)) return m1;

    uintptr_t root = Memory::Read<uintptr_t>(actor + Offsets::AActor_RootComponent);
    if (IsPrimitiveComponent(root)) return root;

    uintptr_t instData = Memory::Read<uintptr_t>(actor + Offsets::AActor_InstanceComps);
    int32_t   instNum = Memory::Read<int32_t>(actor + Offsets::AActor_InstanceComps + 0x8);
    if (Memory::IsValid(instData) && instNum > 0 && instNum < 200) {
        for (int i = 0; i < instNum; i++) {
            uintptr_t comp = Memory::Read<uintptr_t>(instData + i * 8);
            if (IsPrimitiveComponent(comp)) return comp;
        }
    }

    if (Memory::IsValid(root)) {
        uintptr_t childData = Memory::Read<uintptr_t>(root + Offsets::USceneComponent_AttachChildren);
        int32_t   childNum = Memory::Read<int32_t>(root + Offsets::USceneComponent_AttachChildren + 0x8);
        if (Memory::IsValid(childData) && childNum > 0 && childNum < 200) {
            for (int i = 0; i < childNum; i++) {
                uintptr_t child = Memory::Read<uintptr_t>(childData + i * 8);
                if (IsPrimitiveComponent(child)) return child;

                uintptr_t grandData = Memory::Read<uintptr_t>(child + Offsets::USceneComponent_AttachChildren);
                int32_t   grandNum = Memory::Read<int32_t>(child + Offsets::USceneComponent_AttachChildren + 0x8);
                if (Memory::IsValid(grandData) && grandNum > 0 && grandNum < 100) {
                    for (int j = 0; j < grandNum; j++) {
                        uintptr_t grand = Memory::Read<uintptr_t>(grandData + j * 8);
                        if (IsPrimitiveComponent(grand)) return grand;
                    }
                }
            }
        }
    }

    return 0;
}

// ===== Camera =====
inline bool GetCameraPOV(FMinimalViewInfo& out, uintptr_t& outPC)
{
    outPC = 0;
    uintptr_t gWorld = Memory::Read<uintptr_t>(Memory::baseAddress + Offsets::GWorld);
    if (!Memory::IsValid(gWorld)) return false;

    uintptr_t gameInstance = Memory::Read<uintptr_t>(gWorld + Offsets::UWorld_OwningGameInstance);
    if (!Memory::IsValid(gameInstance)) return false;

    uintptr_t localPlayers = Memory::Read<uintptr_t>(gameInstance + Offsets::UGameInstance_LocalPlayers);
    if (!Memory::IsValid(localPlayers)) return false;

    uintptr_t localPlayer = Memory::Read<uintptr_t>(localPlayers);
    if (!Memory::IsValid(localPlayer)) return false;

    uintptr_t pc = Memory::Read<uintptr_t>(localPlayer + Offsets::UPlayer_PlayerController);
    if (!Memory::IsValid(pc)) return false;
    outPC = pc;

    uintptr_t camMgr = Memory::Read<uintptr_t>(pc + Offsets::APlayerController_PlayerCameraManager);
    if (!Memory::IsValid(camMgr)) return false;

    uintptr_t povAddr = camMgr + Offsets::APlayerCameraManager_CameraCache + Offsets::FCameraCacheEntry_POV;
    Memory::Read(povAddr + Offsets::FMinimalViewInfo_Location, out.Location);
    Memory::Read(povAddr + Offsets::FMinimalViewInfo_Rotation, out.Rotation);
    Memory::Read(povAddr + Offsets::FMinimalViewInfo_FOV, out.FOV);
    Memory::Read(povAddr + Offsets::FMinimalViewInfo_Aspect, out.AspectRatio);
    return out.FOV > 0.1f;
}

inline uintptr_t GetLocalPawn(uintptr_t pc)
{
    if (!Memory::IsValid(pc)) return 0;
    return Memory::Read<uintptr_t>(pc + Offsets::APlayerController_AcknowledgedPawn);
}

// ===== WorldToScreen — co FOV Axis + Override =====
inline bool WorldToScreen(const FVector& world, const FMinimalViewInfo& pov,
    int screenW, int screenH, FVector2D& screen,
    float fovBiasPercent = 112.5f,
    float offCenterX = 0.0f, float offCenterY = 0.0f,
    float fovOverride = 0.0f,
    int   fovAxis = 0)   // 0 = horizontal, 1 = vertical
{
    float dx = world.X - pov.Location.X;
    float dy = world.Y - pov.Location.Y;
    float dz = world.Z - pov.Location.Z;

    const float PI = 3.14159265f;
    float pitch = pov.Rotation.Pitch * PI / 180.f;
    float yaw = pov.Rotation.Yaw * PI / 180.f;
    float roll = pov.Rotation.Roll * PI / 180.f;

    float sp = sinf(pitch), cp = cosf(pitch);
    float sy = sinf(yaw), cy = cosf(yaw);
    float sr = sinf(roll), cr = cosf(roll);

    float m00 = cp * cy;
    float m01 = cp * sy;
    float m02 = sp;
    float m10 = sr * sp * cy - cr * sy;
    float m11 = sr * sp * sy + cr * cy;
    float m12 = -sr * cp;
    float m20 = -(cr * sp * cy + sr * sy);
    float m21 = cy * sr - cr * sp * sy;
    float m22 = cr * cp;

    float depth = dx * m00 + dy * m01 + dz * m02;
    if (depth < 1.0f) return false;

    float right = dx * m10 + dy * m11 + dz * m12;
    float up = dx * m20 + dy * m21 + dz * m22;

    float fovBase = (fovOverride > 0.1f) ? fovOverride : pov.FOV;
    float fovReal = fovBase * (fovBiasPercent / 100.0f);

    float aspect = (float)screenW / (float)screenH;

    float tanHalfFovH, tanHalfFovV;
    if (fovAxis == 0) {
        tanHalfFovH = tanf(fovReal * PI / 360.f);
        tanHalfFovV = tanHalfFovH / aspect;
    }
    else {
        tanHalfFovV = tanf(fovReal * PI / 360.f);
        tanHalfFovH = tanHalfFovV * aspect;
    }

    if (tanHalfFovH < 0.0001f || tanHalfFovV < 0.0001f) return false;

    float ndcX = (right / depth) / tanHalfFovH - offCenterX;
    float ndcY = (up / depth) / tanHalfFovV - offCenterY;

    screen.X = (screenW * 0.5f) * (1.0f + ndcX);
    screen.Y = (screenH * 0.5f) * (1.0f - ndcY);
    return true;
}

// ===== Actor list =====
inline std::vector<uintptr_t> GetActors()
{
    std::vector<uintptr_t> actors;
    uintptr_t gWorld = Memory::Read<uintptr_t>(Memory::baseAddress + Offsets::GWorld);
    if (!Memory::IsValid(gWorld)) return actors;

    uintptr_t level = Memory::Read<uintptr_t>(gWorld + Offsets::UWorld_PersistentLevel);
    if (!Memory::IsValid(level)) return actors;

    uintptr_t actorsData = Memory::Read<uintptr_t>(level + Offsets::ULevel_Actors);
    int32_t   actorsCount = Memory::Read<int32_t>(level + Offsets::ULevel_Actors + 0x8);
    if (!Memory::IsValid(actorsData)) return actors;
    if (actorsCount <= 0 || actorsCount > 200000) return actors;

    actors.reserve(actorsCount);
    for (int i = 0; i < actorsCount; i++) {
        uintptr_t a = Memory::Read<uintptr_t>(actorsData + i * 8);
        if (Memory::IsValid(a)) actors.push_back(a);
    }
    return actors;
}