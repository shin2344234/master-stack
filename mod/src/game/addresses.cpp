#include "game/addresses.h"

#include <Windows.h>
#include <cstring>

#include "core/log.h"
#include "game/mem.h"

namespace mst::addr
{
    namespace
    {
        unsigned long long R(uintptr_t a) { return static_cast<unsigned long long>(a ? a - mem::Game().base : 0); }

        uintptr_t Unique(const char* what, const char* pattern)
        {
            size_t hits = 0;
            const uintptr_t a = mem::FindUnique(pattern, &hits);
            if (a) LOG("[addr] %s at +%llX", what, R(a));
            else LOG_ERR("[addr] %s: pattern matched %zu places, expected one", what, hits);
            return a;
        }

        // Primary function entry for an address inside it, following chained
        // unwind info. The read this mod anchors on has an unwind fragment of its
        // own, so the walk is what recovers the entry to hook.
        uintptr_t FunctionEntry(uintptr_t inside)
        {
            DWORD64 imageBase = 0;
            for (int hop = 0; hop < 8; ++hop)
            {
                const RUNTIME_FUNCTION* rf = RtlLookupFunctionEntry(inside, &imageBase, nullptr);
                if (!rf) return 0;
                const uintptr_t begin = static_cast<uintptr_t>(imageBase + rf->BeginAddress);
                uint8_t verFlags = 0, count = 0;
                const uintptr_t unwind = static_cast<uintptr_t>(imageBase + (rf->UnwindData & ~1u));
                if (!mem::Read8(unwind, &verFlags) || !mem::Read8(unwind + 2, &count)) return 0;
                if (!((verFlags >> 3) & 4)) return begin;   // not UNW_FLAG_CHAININFO
                uint32_t parent = 0;
                if (!mem::Read32(unwind + 4 + ((count + 1) & ~1u) * 2, &parent)) return 0;
                inside = static_cast<uintptr_t>(imageBase + parent);
            }
            return 0;
        }
    }

    bool ResolveStacks(Stacks& s)
    {
        // The read of ItemInfo's _maxStackCount, eight bytes into record+0x18.
        // TradeMarketItemInfo's reader has the same eight-byte read at the same
        // offset, so the failure branch decides: every field read is followed by
        // a test and a jump to a block that loads the field's own error string,
        // and the string names both the class and the field. That makes this one
        // hit on 1.0.0.2850 and 1.0.0.2944 alike, and a build that reorders the
        // record gets no match rather than a wrong offset. The research is in
        // Private Storage Master's R9-stack-size notes.
        //
        //   mov  rax, [rdi]          ; the stream
        //   lea  rdx, [rsi + 0x18]   ; _maxStackCount
        //   mov  r8d, 8
        //   mov  rcx, rdi
        //   call [rax + 8]           ; stream->ReadBytes
        //   test al, al / jne        ; then lea rax, [rip + "ItemInfo... _maxStackCount..."]
        const uintptr_t field = Unique("ItemInfo _maxStackCount read",
            "48 8B 07 48 8D 56 18 41 B8 08 00 00 00 48 8B CF FF 50 08 84 C0 75 ?? 48 8D 05 ?? ?? ?? ??");
        if (!field) return false;
        // The message is Korean apart from the two names in it, so it is read as
        // bytes and searched for those. A printable-ASCII read would refuse it at
        // the first byte past ASCII.
        char err[96] = {};
        const bool named = mem::ReadBytes(mem::RipAt(field + 23, 7), err, sizeof err - 1) && strncmp(err, "ItemInfo", 8) == 0 &&
                           strstr(err, "_maxStackCount") != nullptr;
        if (!named)
        {
            LOG_ERR("[addr] the eight-byte read at +%llX does not name ItemInfo's _maxStackCount", R(field));
            return false;
        }
        const uintptr_t entry = FunctionEntry(field);
        if (!entry || field - entry > 0x200)
        {
            LOG_ERR("[addr] ItemInfo reader: no function entry near +%llX", R(field));
            return false;
        }
        // (stream in rcx, record in rdx, kept in rsi), then _key as four bytes.
        if (!mem::MatchAt(entry, "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B 01 48 8B F2"))
        {
            LOG_ERR("[addr] ItemInfo reader at +%llX does not start the way it did on 2.02 and 2.03", R(entry));
            return false;
        }
        s.itemInfoRead = entry;
        LOG("[addr] ItemInfo reader +%llX", R(entry));
        return true;
    }
}
