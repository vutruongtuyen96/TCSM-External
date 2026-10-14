#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>
#include <TlHelp32.h>

class Memory
{
public:
    static HANDLE hProcess;
    static uintptr_t baseAddress;

    static bool Attach(const std::wstring& processName)
    {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return false;

        PROCESSENTRY32W pe{};
        pe.dwSize = sizeof(pe);
        if (!Process32FirstW(snap, &pe)) { CloseHandle(snap); return false; }

        DWORD pid = 0;
        do {
            if (!_wcsicmp(pe.szExeFile, processName.c_str())) { pid = pe.th32ProcessID; break; }
        } while (Process32NextW(snap, &pe));
        CloseHandle(snap);
        if (!pid) return false;

        hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (!hProcess) return false;

        HANDLE modSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        MODULEENTRY32W me{};
        me.dwSize = sizeof(me);
        if (Module32FirstW(modSnap, &me)) baseAddress = (uintptr_t)me.modBaseAddr;
        CloseHandle(modSnap);
        return baseAddress != 0;
    }

    template<typename T> static T Read(uintptr_t addr) {
        T buffer{};
        ReadProcessMemory(hProcess, (LPCVOID)addr, &buffer, sizeof(T), nullptr);
        return buffer;
    }

    template<typename T> static bool Read(uintptr_t addr, T& out) {
        return ReadProcessMemory(hProcess, (LPCVOID)addr, &out, sizeof(T), nullptr);
    }

    static bool ReadBytes(uintptr_t addr, void* buffer, size_t size) {
        return ReadProcessMemory(hProcess, (LPCVOID)addr, buffer, size, nullptr);
    }

    static bool WriteBytes(uintptr_t addr, const void* buffer, size_t size) {
        return WriteProcessMemory(hProcess, (LPVOID)addr, buffer, size, nullptr);
    }

    static bool IsValid(uintptr_t addr) {
        return addr >= 0x10000 && addr < 0x7FFFFFFFFFFF;
    }
};

inline HANDLE Memory::hProcess = nullptr;
inline uintptr_t Memory::baseAddress = 0;