# TSCM ESP + Aimbot

> External ESP & Aimbot for **The Texas Chain Saw Massacre** (UE 4.27)

## 📖 Overview

External overlay cheat for **The Texas Chain Saw Massacre** built with DirectX 11 + ImGui.
Reads game memory via `ReadProcessMemory` (no DLL injection) and draws an overlay on top
of the game window.

**Built for UE 4.27** — uses `GWorld`, `GObjects`, `GNames` as anchor points to walk
`UWorld → ULevel → Actors` and render ESP boxes for players, items, and objectives.

---

## ✨ Features

### ESP
- 🎯 **2D Box ESP** — rectangle around target, color-coded by category
- 📦 **3D Box ESP** — 8-corner perspective box
- 🌟 **Box Glow Effect** — layered fake glow for visibility (ImGui-based, no memory write)
- 📏 **Name + Distance** label
- 📍 **Line ESP** — from screen bottom to target
- 🎨 **Full color customization** — Victim / Family / Item / Objective
- 👥 **Filters** — toggle each category independently
- 🔍 **Auto pattern matching** for all victim/family class variants (`BP_BBQPlayerVictim*`, `BP_BBQPlayerFamily*`)
- 🎚️ **Box Alpha** — adjustable fade for each box

### Aimbot
- 🎯 **Aim at Head / Chest / Pelvis**
- 🎚️ **FOV Radius** — draw FOV circle for visual feedback
- 🎢 **Smoothness slider** — from instant snap to natural glide
- 🎮 **Multiple methods** — `SendInput` / `mouse_event` / `SetCursorPos`
- 🎨 **Target filters** — Victim / Family / Grandpa (independent)
- 📊 **Live debug panel** — shows locked target, distance, FOV count

### General
- 🖥️ **External overlay** — no injection, uses DX11 + ImGui
- 🖱️ **Clean menu UX** — cursor unlock on open, game focus restore on close
- ⌨️ **INSERT** — toggle menu
- ⌨️ **DELETE** — unload cleanly

---

## ⚙️ Requirements

- **OS**: Windows 10 / 11 (x64)
- **Compiler**: Visual Studio 2022 (MSVC v143)
- **SDK**: Windows 10 SDK or higher
- **Libraries**: DirectX 11, DXGI
- **[Dear ImGui](https://github.com/ocornut/imgui)** (docking branch recommended)

---

## 🔨 Build Instructions

### 1. Clone & Setup

```bash
git clone https://github.com/<your-username>/TSCM-ESP.git
cd TSCM-ESP

# Clone ImGui next to project
git clone https://github.com/ocornut/imgui.git
