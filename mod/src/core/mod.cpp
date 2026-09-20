#include "core/mod.h"

#include <atomic>

#include "core/log.h"
#include "core/paths.h"
#include "core/settings.h"
#include "game/farhook.h"
#include "game/mem.h"
#include "stacks/stacks.h"
#include "version.h"

namespace
{
    std::atomic<bool> g_stop{false};
    HANDLE g_thread = nullptr;

    DWORD WINAPI Worker(LPVOID)
    {
        sm::Settings::Load();
        // Without DebugLog a session is a handful of lines, and the last two are
        // all a report needs, including the one before a crash and a relaunch.
        if (!sm::Log::Debug()) sm::Log::Prune(SM_FILEBASE, 2);
        LOG_NOTE("[mod] %s %s for Crimson Desert %s, game image at 0x%p, %zu bytes", SM_NAME, SM_VERSION, SM_GAME,
                 reinterpret_cast<void*>(sm::mem::Game().base), sm::mem::Game().size);
        if (!sm::Settings::Get().enabled)
        {
            LOG_NOTE("[mod] Enabled=0, so the mod does nothing");
            return 0;
        }
        sm::stacks::Start();
        // The game reads its item table a few seconds in. Wait past that, then say
        // what came of it, so the log answers "did it work" without a key to press.
        const DWORD started = GetTickCount();
        while (!g_stop.load() && GetTickCount() - started < 15000) Sleep(250);
        if (g_stop.load()) return 0;
        sm::stacks::Flush();
        return 0;
    }
}

namespace sm::Mod
{
    // crashpad_handler.exe loads ASI plugins too. That instance does nothing.
    static constexpr size_t kMinGameImage = 64ull * 1024 * 1024;

    void Initialize(HMODULE module)
    {
        Paths::Init(module);
        const size_t size = mem::Game().size;
        // It writes no log: one file per crash handler launch piled up beside the plugin.
        if (!mem::Game().base || size < kMinGameImage) return;
        Log::Claim(SM_FILEBASE);
        g_thread = CreateThread(nullptr, 0, Worker, nullptr, 0, nullptr);
    }

    void Shutdown(bool processExiting)
    {
        g_stop.store(true);
        stacks::Stop();
        if (processExiting)
        {
            Log::Shutdown();
            return;
        }
        if (g_thread)
        {
            WaitForSingleObject(g_thread, 3000);
            CloseHandle(g_thread);
            g_thread = nullptr;
        }
        farhook::RemoveAll();
        Log::Shutdown();
    }
}
