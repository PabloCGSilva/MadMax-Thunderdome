// Thunderdome -- runtime part of the Thunderdome mod for Mad Max (2015, PC).
//
// The mod itself is data (dropzone files: the arena, its waves, the crowd, the
// Tenderloin finale, the map icon and the fast-travel point). Two things the
// data cannot do are done here; until this plugin existed they lived in the
// diagnostic ThunderdomeProbe, so this is exactly what the tests ran with:
//
//  1. Character resources for the gate waves. The arena's gate enemies are
//     Roadkill defenders and gimps; the game only loads a faction's resource
//     sets where that faction lives, so in The Big Nothing their spawns waited
//     forever and the wave never finished. At the first arena spawner update of
//     a session (CArenaSpawner::UpdatePostSim) the two sets are requested with
//     CSpawnedEntityHandler::LoadResourceSets and held for the session.
//  2. The fast-travel point. A save stores every fast-travel point's state, so
//     a save made while the arena's point was off would turn it off again.
//     After every CMissionManager::Load the arena's point (0x37601FEE) is
//     switched on with CMissionManager::SetRestartPointEnable.
//
// Depends on MinHook only; every game address is found at startup by byte
// signature (generated from the GOG 2015-12-07 build by tools/gen_sigs.py).
// Startup on Steam (code encrypted by the DRM until the game starts) works as
// in Wasteland Storms: install from the C runtime's GetStartupInfoW call.

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdarg>
#include "MinHook.h"

#define TD_VERSION "0.9.0-beta1"

// ---------------------------------------------------------------- logging --
static char g_modDir[MAX_PATH] = "";
static char g_logPath[MAX_PATH] = "Thunderdome.log";
static DWORD g_logStart = 0;

static void InitPaths(HMODULE self) {
    if (GetModuleFileNameA(self, g_modDir, MAX_PATH)) {
        char* slash = strrchr(g_modDir, '\\');
        if (slash) slash[1] = 0;
        snprintf(g_logPath, MAX_PATH, "%sThunderdome.log", g_modDir);
    }
}

static void LogLine(const char* fmt, ...) {
    FILE* f = nullptr;
    if (fopen_s(&f, g_logPath, "a") != 0 || !f) return;
    SYSTEMTIME st; GetLocalTime(&st);
    DWORD t = GetTickCount() - g_logStart;
    fprintf(f, "[%02d:%02d:%02d +%5lu.%01lus] ", st.wHour, st.wMinute, st.wSecond, (unsigned long)(t / 1000), (unsigned long)((t % 1000) / 100));
    va_list ap; va_start(ap, fmt); vfprintf(f, fmt, ap); va_end(ap);
    fputc('\n', f);
    fclose(f);
}

// The previous session's log is kept as Thunderdome.previous.log.
static void LogStart() {
    g_logStart = GetTickCount();
    char prev[MAX_PATH];
    snprintf(prev, MAX_PATH, "%sThunderdome.previous.log", g_modDir);
    MoveFileExA(g_logPath, prev, MOVEFILE_REPLACE_EXISTING);
    FILE* f = nullptr;
    if (fopen_s(&f, g_logPath, "w") == 0 && f) fclose(f);
}

// ------------------------------------------------------ signature scanning --
struct GameSignature {
    const char* name;
    const char* pattern;   // "48 8B ?? ..." -- ?? is a wildcard byte
    uintptr_t resolved;
    int matches;
    bool alreadyHooked;     // found behind another mod's hook (E9 jmp at the start)
};
static GameSignature g_sigs[] = {
    { "CMissionManager::Load", "40 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 D9 48 81 EC A0 00 00 00 48 C7 45 1F FE FF FF FF 48 89 9C 24 E8 00 00 00 48 8B F2", 0, 0, false },
    { "CMissionManager::SetRestartPointEnable", "48 89 5C 24 08 57 48 83 EC 20 45 0F B6 D8 48 8B FA 48 8B D9", 0, 0, false },
    { "CArenaSpawner::UpdatePostSim", "48 89 54 24 10 48 89 4C 24 08 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 18 FF FF FF", 0, 0, false },
    { "CSpawnedEntityHandler::GetResourceSetSpawnTypeId", "48 89 5C 24 08 41 8B D8 4C 63 DA 4C 8B D1 41 83 F8 FF 74 ??", 0, 0, false },
    { "CSpawnedEntityHandler::LoadResourceSets", "48 89 6C 24 20 41 54 41 55 41 56 48 83 EC 30 48 89 5C 24 50", 0, 0, false },
    { "s_SpawnSystemContext (load of the pointer)", "48 8B 05 ?? ?? ?? ?? 88 88 8B 01 00 00 C3 CC CC", 0, 0, false },
};
enum { SIG_LOAD = 0, SIG_SETENABLE, SIG_ARENAUPDATE, SIG_GETSETID, SIG_LOADSETS, SIG_SPAWNCTX, SIG_COUNT };

static uintptr_t g_textStart = 0, g_textEnd = 0;
static unsigned int g_exeTimeStamp = 0;
static char g_versionStatus[300] = "not checked";

static bool ScanBytes(const unsigned char* bytes, const bool* wild, int len, GameSignature& sig) {
    sig.matches = 0; sig.resolved = 0;
    const unsigned char* p = (const unsigned char*)g_textStart;
    const unsigned char* end = (const unsigned char*)g_textEnd - len;
    for (; p <= end; p++) {
        if (!wild[0] && p[0] != bytes[0]) continue;
        int i = 1;
        for (; i < len; i++) if (!wild[i] && p[i] != bytes[i]) break;
        if (i == len) {
            if (++sig.matches == 1) sig.resolved = (uintptr_t)p;
            else break;
        }
    }
    return sig.matches == 1;
}

// Another ASI may already have hooked the same function: MinHook replaces the
// first 5 bytes with `jmp rel32` and leaves the rest untouched, so if the plain
// pattern finds nothing, look for E9 + 4 wildcards + the pattern from byte 5 on.
static bool ScanPattern(GameSignature& sig) {
    unsigned char bytes[128]; bool wild[128]; int len = 0;
    for (const char* c = sig.pattern; *c && len < 128; ) {
        while (*c == ' ') c++;
        if (!*c) break;
        if (c[0] == '?') { wild[len] = true; bytes[len] = 0; len++; c += 2; continue; }
        unsigned int v = 0; sscanf_s(c, "%2x", &v); bytes[len] = (unsigned char)v; wild[len] = false; len++; c += 2;
    }
    sig.alreadyHooked = false;
    if (ScanBytes(bytes, wild, len, sig)) return true;
    if (sig.matches != 0 || len < 12) return false;
    bytes[0] = 0xE9; wild[0] = false;
    for (int i = 1; i < 5; i++) wild[i] = true;
    if (!ScanBytes(bytes, wild, len, sig)) return false;
    sig.alreadyHooked = true;
    return true;
}

static bool ResolveGameAddresses() {
    uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
    const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) { snprintf(g_versionStatus, sizeof(g_versionStatus), "no DOS header"); return false; }
    const IMAGE_NT_HEADERS64* nt = (const IMAGE_NT_HEADERS64*)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) { snprintf(g_versionStatus, sizeof(g_versionStatus), "no PE header"); return false; }
    g_exeTimeStamp = nt->FileHeader.TimeDateStamp;
    const IMAGE_SECTION_HEADER* sec = IMAGE_FIRST_SECTION(nt);
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++) {
        if (memcmp(sec->Name, ".text", 5) == 0) {
            g_textStart = base + sec->VirtualAddress;
            g_textEnd = g_textStart + sec->Misc.VirtualSize;
            break;
        }
    }
    if (!g_textStart) { snprintf(g_versionStatus, sizeof(g_versionStatus), "no .text section"); return false; }

    int failed = 0; char failList[240] = "";
    for (int i = 0; i < SIG_COUNT; i++) {
        if (!ScanPattern(g_sigs[i])) {
            failed++;
            size_t l = strlen(failList);
            snprintf(failList + l, sizeof(failList) - l, "%s%s(x%d)", l ? ", " : "", g_sigs[i].name, g_sigs[i].matches);
        }
    }
    if (failed) {
        snprintf(g_versionStatus, sizeof(g_versionStatus), "signature not found: %s (exe timestamp 0x%08X)", failList, g_exeTimeStamp);
        return false;
    }
    snprintf(g_versionStatus, sizeof(g_versionStatus), "OK (exe timestamp 0x%08X, %s)", g_exeTimeStamp,
        g_exeTimeStamp == 0x566520B2u ? "the GOG build this was made on" : "a build not seen by the author -- please report how it works");
    return true;
}

static bool NoSignatureMatched() {
    for (int i = 0; i < SIG_COUNT; i++) if (g_sigs[i].matches) return false;
    return true;
}

// ------------------------------------------------- 1. resource sets --
// Spawn type and faction hashes of the sets the gate waves use (Roadkills). Loading more at once (10 sets)
// froze the game for a moment in testing, so only these two.
struct WantedSet { const char* name; unsigned type, faction; uint8_t id; };
static WantedSet g_wanted[] = {
    { "roadkills defender", 1049477367u, 4264560240u, 0xFF },
    { "roadkills gimp", 2970139294u, 4264560240u, 0xFF },
};
static const int WANTED = sizeof(g_wanted) / sizeof(g_wanted[0]);

typedef uint8_t (*GetSetIdFn)(void* handler, int type, unsigned spawnTypeHash, unsigned factionHash);
typedef void (*LoadSetsFn)(void* handler, const uint8_t* ids /* 0xFF-terminated, max 20 */, char priority);
typedef void (*ArenaUpdateFn)(void* spawner, void* ctx);
static GetSetIdFn GetResourceSetSpawnTypeId = nullptr;
static LoadSetsFn LoadResourceSets = nullptr;
static ArenaUpdateFn ArenaUpdate_orig = nullptr;
static void** g_spawnContext = nullptr;          // CSpawnedEntityHandler* s_SpawnSystemContext
static void* g_preloadedFor = nullptr;           // the handler the sets were requested on
static int g_preloads = 0;

static void PreloadSets(void* h) {
    uint8_t ids[21]; int n = 0;
    for (int i = 0; i < WANTED; i++) {
        uint8_t id = 0xFF;
        __try { id = GetResourceSetSpawnTypeId(h, 0, g_wanted[i].type, g_wanted[i].faction); }
        __except (EXCEPTION_EXECUTE_HANDLER) { id = 0xFF; }
        g_wanted[i].id = id;
        LogLine("  resource set %-20s -> id %u%s", g_wanted[i].name, id, id == 0xFF ? " (not found, skipped)" : "");
        if (id != 0xFF && n < 20) ids[n++] = id;
    }
    ids[n] = 0xFF;
    if (n) {
        LoadResourceSets(h, ids, 0);
        LogLine("  requested %d resource set(s), held for the rest of the session", n);
    }
}

// The probe this replaces requested the sets once per session; this does it once per spawn handler, in case
// the game ever rebuilds the handler (a new pointer) within a session.
static void ArenaUpdate_hook(void* spawner, void* ctx) {
    void* h = g_spawnContext ? *g_spawnContext : nullptr;
    if (h && h != g_preloadedFor) {
        g_preloadedFor = h;
        g_preloads++;
        LogLine("arena spawner active (%p): loading the gate enemies' resources", spawner);
        PreloadSets(h);
    }
    ArenaUpdate_orig(spawner, ctx);
}

// ------------------------------------------------- 2. fast-travel point --
static const uint64_t THUNDERDOME_RESTART_POINT = 0x37601FEEull;   // CRestartPoint in global/global.blo
typedef void (*SetEnableFn)(void* missionManager, uint64_t id, bool enable);
typedef void (*LoadFn)(void* missionManager, const void* saveContainer);
static SetEnableFn SetRestartPointEnable = nullptr;
static LoadFn Load_orig = nullptr;
static int g_loads = 0;

static void Load_hook(void* mgr, const void* save) {
    Load_orig(mgr, save);
    SetRestartPointEnable(mgr, THUNDERDOME_RESTART_POINT, true);
    g_loads++;
    LogLine("save loaded: Thunderdome fast-travel point switched on");
}

// ------------------------------------------------------------- startup --
static const char* InstallHook(uintptr_t target, LPVOID hook, LPVOID* orig) {
    if (!target) return "not found";
    MH_STATUS c = MH_CreateHook((LPVOID)target, hook, orig);
    if (c != MH_OK) return MH_StatusToString(c);
    return MH_StatusToString(MH_EnableHook((LPVOID)target));
}

// Test switch, shared with the author's other mods: MADMAX_MODS_DEFER=1 in the environment, or a file
// scripts\force_steam_path.txt, forces the deferred (Steam) startup path.
static bool g_forcePollOnly = false;
static bool ForceDefer() {
    char v[8];
    if (GetEnvironmentVariableA("MADMAX_MODS_DEFER", v, sizeof(v)) > 0 && v[0] == '1') return true;
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char* slash = strrchr(path, '\\'); if (slash) slash[1] = 0;
    strcat_s(path, "scripts\\force_steam_path.txt");
    FILE* f = nullptr;
    if (fopen_s(&f, path, "r") != 0 || !f) return false;
    char text[8] = { 0 };
    fread(text, 1, 4, f); fclose(f);
    g_forcePollOnly = strncmp(text, "poll", 4) == 0;
    return true;
}

static void InstallMod(bool ok) {
    LogLine("game check: %s", g_versionStatus);
    for (int i = 0; i < SIG_COUNT; i++)
        LogLine("  %-50s %s (matches %d)%s", g_sigs[i].name, g_sigs[i].matches == 1 ? "OK" : "--", g_sigs[i].matches,
            g_sigs[i].alreadyHooked ? " -- already hooked by another mod, chaining behind it" : "");
    if (!ok) {
        char msg[700];
        snprintf(msg, sizeof(msg),
            "Thunderdome: the plugin could not find the game functions it needs in this executable.\n\n"
            "Reason: %s\n\n"
            "The arena still loads, but the enemies that come in through the gate may never appear "
            "(the fight then stops at wave 2) and the fast-travel point may switch off after loading a save. "
            "Please report your game version (store + patch) to the mod author.", g_versionStatus);
        MessageBoxA(NULL, msg, "Mad Max - Thunderdome", MB_OK | MB_ICONWARNING);
        return;
    }
    uintptr_t insn = g_sigs[SIG_SPAWNCTX].resolved;                  // mov rax,[rip+disp32]
    g_spawnContext = (void**)(insn + 7 + *(int32_t*)(insn + 3));
    GetResourceSetSpawnTypeId = (GetSetIdFn)g_sigs[SIG_GETSETID].resolved;
    LoadResourceSets = (LoadSetsFn)g_sigs[SIG_LOADSETS].resolved;
    SetRestartPointEnable = (SetEnableFn)g_sigs[SIG_SETENABLE].resolved;
    LogLine("arena hook: %s", InstallHook(g_sigs[SIG_ARENAUPDATE].resolved, (LPVOID)ArenaUpdate_hook, (LPVOID*)&ArenaUpdate_orig));
    LogLine("save-load hook: %s", InstallHook(g_sigs[SIG_LOAD].resolved, (LPVOID)Load_hook, (LPVOID*)&Load_orig));
    LogLine("ready");
}

static volatile LONG g_installed = 0;
static CRITICAL_SECTION g_installLock;
static bool TryInstall(const char* how) {
    EnterCriticalSection(&g_installLock);
    bool done = g_installed != 0;
    if (!done) {
        bool ok = ResolveGameAddresses();
        if (ok || !NoSignatureMatched()) {
            g_installed = 1;
            LogLine("game code ready (%s), installing", how);
            InstallMod(ok);
            done = true;
        }
    }
    LeaveCriticalSection(&g_installLock);
    return done;
}

typedef void (WINAPI* GetStartupInfoWFn)(LPSTARTUPINFOW info, uintptr_t chain);
static GetStartupInfoWFn GetStartupInfoW_orig = nullptr;
// The second argument is not part of GetStartupInfoW; mm_sdk-based plugins pass a marker through it, so it is
// forwarded untouched.
static void WINAPI GetStartupInfoW_hook(LPSTARTUPINFOW info, uintptr_t chain) {
    GetStartupInfoW_orig(info, chain);
    if (!g_installed) TryInstall("C runtime startup");
}

static DWORD WINAPI WaitForGameCode(LPVOID) {
    for (int i = 0; i < 720 && !g_installed; i++) {
        Sleep(250);
        if (TryInstall("found while waiting")) return 0;
    }
    if (!g_installed)
        LogLine("gave up after 3 minutes: none of the game functions were found -- please send this log to the mod author.");
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_DETACH) {
        LogLine("session end: %d save load(s), resources requested %d time(s)", g_loads, g_preloads);
        return TRUE;
    }
    if (reason != DLL_PROCESS_ATTACH) return TRUE;

    DisableThreadLibraryCalls(hModule);
    InitPaths(hModule);
    LogStart();
    MH_STATUS init = MH_Initialize();
    InitializeCriticalSection(&g_installLock);
    bool defer = ForceDefer();
    bool ok = ResolveGameAddresses();
    LogLine("Thunderdome %s", TD_VERSION);
    LogLine("MinHook: %s", MH_StatusToString(init));
    if ((!ok && NoSignatureMatched()) || defer) {
        MH_STATUS e = MH_ERROR_DISABLED;
        if (!g_forcePollOnly) {
            LPVOID gsi = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetStartupInfoW");
            MH_STATUS c = gsi ? MH_CreateHook(gsi, (LPVOID)GetStartupInfoW_hook, (LPVOID*)&GetStartupInfoW_orig) : MH_ERROR_FUNCTION_NOT_FOUND;
            e = (c == MH_OK) ? MH_EnableHook(gsi) : c;
        }
        LogLine("game code not readable yet (Steam DRM?), installing at game startup: %s; also checking again every 250 ms", MH_StatusToString(e));
        HANDLE th = CreateThread(nullptr, 0, WaitForGameCode, nullptr, 0, nullptr);
        if (th) CloseHandle(th);
        return TRUE;
    }
    InstallMod(ok);
    return TRUE;
}
