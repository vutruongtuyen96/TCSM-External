#pragma once
#include <cstdint>

namespace Offsets
{
    // ===== GLOBALS =====
    constexpr uintptr_t GWorld = 0x6D9AA40;
    constexpr uintptr_t GObjects = 0x6C55AF0;
    constexpr uintptr_t GNames = 0x6C19780;
    constexpr uintptr_t GEngine = 0x6D97138;

    // ===== UWorld =====
    constexpr uintptr_t UWorld_PersistentLevel = 0x0030;
    constexpr uintptr_t UWorld_OwningGameInstance = 0x0180;

    // ===== ULevel =====
    constexpr uintptr_t ULevel_Actors = 0x0098;

    // ===== UObject =====
    constexpr uintptr_t UObject_ClassPrivate = 0x0010;
    constexpr uintptr_t UObject_NamePrivate = 0x0018;

    // ===== AActor =====
    constexpr uintptr_t AActor_RootComponent = 0x0138;
    constexpr uintptr_t AActor_InstanceComps = 0x01F8;   // TArray<UActorComponent*>

    // ===== ACharacter =====
    constexpr uintptr_t ACharacter_Mesh = 0x0288;

    // ===== APlayerController =====
    constexpr uintptr_t APlayerController_AcknowledgedPawn = 0x02A8;
    constexpr uintptr_t APlayerController_PlayerCameraManager = 0x02C0;
    constexpr uintptr_t APlayerController_PlayerState = 0x0230;

    // ===== APlayerCameraManager =====
    constexpr uintptr_t APlayerCameraManager_CameraCache = 0x1B30;

    // ===== FCameraCacheEntry =====
    constexpr uintptr_t FCameraCacheEntry_POV = 0x0010;

    // ===== FMinimalViewInfo =====
    constexpr uintptr_t FMinimalViewInfo_Location = 0x0000;
    constexpr uintptr_t FMinimalViewInfo_Rotation = 0x000C;
    constexpr uintptr_t FMinimalViewInfo_FOV = 0x0018;
    constexpr uintptr_t FMinimalViewInfo_Aspect = 0x002C;

    // ===== USceneComponent =====
    constexpr uintptr_t USceneComponent_RelativeLocation = 0x011C;
    constexpr uintptr_t USceneComponent_AttachChildren = 0x00D0;

    // ===== UGameInstance / UPlayer =====
    constexpr uintptr_t UGameInstance_LocalPlayers = 0x0038;
    constexpr uintptr_t UPlayer_PlayerController = 0x0030;

    // ===== FNamePool (UE 4.27) =====
    constexpr uintptr_t FNamePool_Blocks = 0x0010;
    constexpr uintptr_t FNameEntryStride = 0x0002;

    // ===== UPrimitiveComponent =====
    constexpr uintptr_t UPrim_bRenderCustomDepth = 0x0217;
    constexpr uintptr_t UPrim_CustomDepthStencilWriteMask = 0x021C;
    constexpr uintptr_t UPrim_CustomDepthStencilValue = 0x0220;
}