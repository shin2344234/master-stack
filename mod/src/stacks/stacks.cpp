#include "stacks/stacks.h"

#include <Windows.h>
#include <atomic>

#include "core/log.h"
#include "core/settings.h"
#include "game/addresses.h"
#include "game/farhook.h"
#include "game/mem.h"

namespace mst::stacks
{
    namespace
    {
        using Fn4 = uintptr_t(__fastcall*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);

        // ItemInfo, from Private Storage Master's R9 notes.
        constexpr unsigned kMaxStackCount = 0x18;   // eight bytes
        constexpr unsigned kKey = 0x00;             // four bytes, the item number
        // No stack is taken past this. Fat Stacks' largest option is 999999 and
        // the game carries it in this same field, so it is a number the game is
        // known to cope with. A stock limit already at or past it is left alone:
        // money sits far above it and clamping would shrink it.
        constexpr int64_t kCeiling = 999999;

        addr::Stacks g_addr;
        void* oRead = nullptr;
        int g_multiplier = 1;
        std::atomic<bool> g_stop{false};
        std::atomic<bool> g_patching{false};
        std::atomic<bool> g_hooked{false};
        std::atomic<int> g_reason{kOff};
        std::atomic<int> g_patched{0};
        std::atomic<int64_t> g_biggest{0};
        std::atomic<int> g_unstackable{0};
        std::atomic<int> g_huge{0};
        std::atomic<int> g_logged{0};

        // Loader threads call this. It reads two fields and writes one, and does
        // nothing else: no allocation, no lock, no game call. The stream fills the
        // record from the data file immediately before this runs, so the value
        // read here is always the game's own and a record cannot be raised twice.
        void PatchRecord(uintptr_t rec)
        {
            int64_t stock = 0;
            if (!mem::ReadBytes(rec + kMaxStackCount, &stock, sizeof stock)) return;
            // 0 and 1 are the game saying this item does not stack. Multiplying
            // those would let gear, quest items and mounts pile into one slot.
            if (stock <= 1) { ++g_unstackable; return; }
            if (stock >= kCeiling) { ++g_huge; return; }
            int64_t want = stock * g_multiplier;
            if (want > kCeiling) want = kCeiling;
            if (want <= stock) return;
            __try
            {
                *reinterpret_cast<int64_t*>(rec + kMaxStackCount) = want;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { return; }
            ++g_patched;
            if (want > g_biggest.load()) g_biggest = want;
            if (Log::Debug() && g_logged.fetch_add(1) < 10)
            {
                uint32_t key = 0;
                mem::Read32(rec + kKey, &key);
                LOG("[stacks] item %u stacks to %lld instead of %lld", key, static_cast<long long>(want), static_cast<long long>(stock));
            }
        }

        uintptr_t __fastcall hkRead(uintptr_t stream, uintptr_t rec, uintptr_t r8, uintptr_t r9)
        {
            const uintptr_t r = static_cast<Fn4>(oRead)(stream, rec, r8, r9);
            if ((r & 0xFF) && rec && g_patching.load()) PatchRecord(rec);
            return r;
        }
    }

    void Start()
    {
        const int mult = Settings::Startup().multiplier;
        if (mult <= 1)
        {
            g_reason = kOff;
            LOG_NOTE("[stacks] Multiplier=1, so stacks hold what the game gives them");
            return;
        }
        g_multiplier = mult;
        // Early in a launch the game has not finished unpacking its code and no
        // pattern matches yet. The item table is read later than that, so waiting
        // here still lands the hook before it.
        bool resolved = false;
        for (int i = 0; i < 120 && !g_stop.load(); ++i)
        {
            if ((resolved = addr::ResolveStacks(g_addr)) != false) break;
            if (i == 0) LOG("[stacks] the item table reader was not found yet, retrying");
            Sleep(500);
        }
        if (!resolved)
        {
            g_reason = kNoAnchor;
            LOG_ERR("[stacks] the item table reader was not found in this game version, so stacks are left alone");
            return;
        }
        char why[128];
        if (!farhook::Install("ItemInfoRead", g_addr.itemInfoRead, hkRead, &oRead, why, sizeof why))
        {
            g_reason = kHookFailed;
            LOG_ERR("[stacks] hooking the item table reader failed: %s. Stacks are left alone.", why);
            return;
        }
        g_patching = true;
        g_hooked = true;
        g_reason = kApplying;
        LOG_NOTE("[stacks] Multiplier=%d: every item that already stacks holds %d times as much, up to %lld", mult, mult,
                 static_cast<long long>(kCeiling));
    }

    void Flush()
    {
        if (g_multiplier <= 1 || !g_hooked.load()) return;
        const int n = g_patched.load();
        if (n)
            LOG_NOTE("[stacks] %d items stack x%d now, the biggest holding %lld; %d items the game does not stack were left alone", n, g_multiplier,
                     static_cast<long long>(g_biggest.load()), g_unstackable.load());
        else
        {
            g_reason = kTooLate;
            LOG_ERR("[stacks] no item was raised, so the game read its item table before the mod started. Stacks are the game's this session.");
        }
    }

    Report Status()
    {
        Report r;
        r.hooked = g_hooked.load();
        r.reason = static_cast<Reason>(g_reason.load());
        r.multiplier = r.reason == kApplying ? g_multiplier : 1;
        r.patched = g_patched.load();
        r.biggest = g_biggest.load();
        r.unstackable = g_unstackable.load();
        return r;
    }

    void Stop()
    {
        // The hook itself is removed with all the others by farhook::RemoveAll.
        g_stop = true;
        g_patching = false;
    }
}
