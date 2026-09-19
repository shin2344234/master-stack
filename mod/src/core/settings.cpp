#include "core/settings.h"

#include <Windows.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "core/log.h"
#include "core/paths.h"
#include "version.h"

namespace mst::Settings
{
    namespace
    {
        void Trim(char* s)
        {
            char* p = s;
            while (*p == ' ' || *p == '\t') ++p;
            if (p != s) memmove(s, p, strlen(p) + 1);
            size_t n = strlen(s);
            while (n && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r' || s[n - 1] == '\n')) s[--n] = 0;
        }

        bool WriteIni(const Values& v)
        {
            FILE* f = nullptr;
            if (_wfopen_s(&f, Paths::File(MST_INI).c_str(), L"w") != 0 || !f) return false;
            fprintf(f,
                    "; %s %s\n"
                    "; Bigger stacks of stackable items, for Crimson Desert %s.\n"
                    "; Nothing in the game's own files is changed: the mod raises each item's\n"
                    "; stack limit in memory as the game reads its item table.\n\n"
                    "[MasterStack]\n\n"
                    "; 0 turns the whole mod off.\n"
                    "Enabled=%d\n\n"
                    "; 1 writes a line per item raised and keeps every session's log.\n"
                    "DebugLog=%d\n\n"
                    "; How much of one item a single slot holds, as a multiple of the game's own\n"
                    "; limit. 1 leaves every stack alone. 5 turns a stack of 100 into 500 and a\n"
                    "; stack of 20 into 100, so the game's own differences between item kinds\n"
                    "; stay. Items the game does not stack, like gear and quest items, never\n"
                    "; start stacking, and no stack is taken past 999999. An item the game\n"
                    "; already lets you hold more of than that, such as money, is left alone.\n"
                    "; The change takes effect the next time the game starts.\n"
                    "; Your save does record the bigger stacks you build, so if you set this\n"
                    "; back to 1 a slot can still hold more than the game allows. It keeps what\n"
                    "; is in it until you take some out. Empty the big stacks first.\n"
                    "; Do not run a stack-size data mod such as Fat Stacks as well. With\n"
                    "; Private Storage Master installed this mod is the one that applies, and\n"
                    "; its own StackMultiplier is ignored.\n"
                    "Multiplier=%d\n",
                    MST_NAME, MST_VERSION, MST_GAME, v.enabled ? 1 : 0, v.debugLog ? 1 : 0, v.multiplier);
            fclose(f);
            return true;
        }

        void ReadIni(Values& out)
        {
            FILE* f = nullptr;
            if (_wfopen_s(&f, Paths::File(MST_INI).c_str(), L"r") != 0 || !f) return;
            char line[512];
            while (fgets(line, sizeof line, f))
            {
                Trim(line);
                if (!line[0] || line[0] == ';' || line[0] == '#' || line[0] == '[') continue;
                char* eq = strchr(line, '=');
                if (!eq) continue;
                *eq = 0;
                char* key = line;
                char* val = eq + 1;
                Trim(key);
                Trim(val);
                if (_stricmp(key, "Enabled") == 0) out.enabled = atoi(val) != 0;
                else if (_stricmp(key, "DebugLog") == 0) out.debugLog = atoi(val) != 0;
                else if (_stricmp(key, "Multiplier") == 0) out.multiplier = atoi(val);
                // The name the setting has inside Private Storage Master, so a
                // player who copies the line across gets what they meant.
                else if (_stricmp(key, "StackMultiplier") == 0) out.multiplier = atoi(val);
                else LOG_NOTE("[settings] %s is not a setting this version knows; ignored", key);
            }
            fclose(f);
        }

        void Clamp(Values& v)
        {
            if (v.multiplier < 1) v.multiplier = 1;
            if (v.multiplier > kMaxMultiplier) v.multiplier = kMaxMultiplier;
        }

        // Published copies are never freed: a reader may still hold the old one,
        // and a copy is a few bytes changed a handful of times a session.
        std::atomic<const Values*> g_cur{nullptr};
        const Values* g_startup = nullptr;
        SRWLOCK g_writeLock = SRWLOCK_INIT;

        void Publish(const Values& v)
        {
            g_cur.store(new Values(v));
            Log::SetDebug(v.debugLog);
        }

        int CommitLocked(Values v, char* why, size_t whyLen)
        {
            Clamp(v);
            Publish(v);
            if (WriteIni(v)) return 1;
            if (why && whyLen) snprintf(why, whyLen, "applied, but %s could not be written", Paths::FileUtf8(MST_INI).c_str());
            return 2;
        }
    }

    Values Defaults() { return Values{}; }

    void Load()
    {
        Values v = Defaults();
        const bool have = GetFileAttributesW(Paths::File(MST_INI).c_str()) != INVALID_FILE_ATTRIBUTES;
        if (have) ReadIni(v);
        Clamp(v);
        if (!have) WriteIni(v);
        g_startup = new Values(v);
        Publish(v);
        if (!have) LOG_NOTE("[settings] wrote a default %s", Paths::FileUtf8(MST_INI).c_str());
        LOG_NOTE("[settings] Enabled=%d DebugLog=%d Multiplier=%d", v.enabled ? 1 : 0, v.debugLog ? 1 : 0, v.multiplier);
    }

    const Values& Get()
    {
        static const Values kEmpty{};
        const Values* v = g_cur.load();
        return v ? *v : kEmpty;
    }

    const Values& Startup() { return g_startup ? *g_startup : Get(); }

    bool Apply(const Values& in, char* why, size_t whyLen)
    {
        if (why && whyLen) why[0] = 0;
        AcquireSRWLockExclusive(&g_writeLock);
        const int result = CommitLocked(in, why, whyLen);
        ReleaseSRWLockExclusive(&g_writeLock);
        LOG("[settings] changed in game%s", result == 1 ? " and saved" : ", but the ini could not be written");
        return result == 1;
    }

    bool Update(const std::function<void(Values&)>& change, char* why, size_t whyLen)
    {
        if (why && whyLen) why[0] = 0;
        AcquireSRWLockExclusive(&g_writeLock);
        Values v = Get();
        change(v);
        const int result = CommitLocked(v, why, whyLen);
        ReleaseSRWLockExclusive(&g_writeLock);
        LOG("[settings] changed in game%s", result == 1 ? " and saved" : ", but the ini could not be written");
        return result == 1;
    }

    bool RestartNeeded()
    {
        const Values& a = Get();
        const Values& b = Startup();
        return a.enabled != b.enabled || a.multiplier != b.multiplier;
    }
}
