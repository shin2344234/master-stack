#pragma once
#include <cstddef>
#include <functional>

// StackMaster.ini. Three settings and nothing else: whether the mod runs at all,
// whether the log is verbose, and how much deeper stacks go.
//
// Settings can change while the game runs (Master Looter's Stacks tab through
// StackApplyMultiplier). Get() hands out the current copy; a change publishes a
// new copy and the old one is kept alive, so a reader on another thread never
// sees a half-written value. The multiplier in force is the one this launch
// started with, so a change needs a restart and Startup() is what the hook uses.
namespace sm::Settings
{
    // The largest multiple a stack limit is raised by. The per-item ceiling in
    // stacks/stacks stops the result running away; this stops a typo like
    // 100000 from turning every small stack into a huge one.
    inline constexpr int kMaxMultiplier = 1000;

    struct Values
    {
        bool enabled = true;
        bool debugLog = false;
        // How much of one item a slot holds, as a multiple of the game's own
        // limit for that item. 1 changes nothing and installs no hook.
        int  multiplier = 1;
    };

    void Load();               // once, at startup
    const Values& Get();       // current settings
    const Values& Startup();   // what this launch started with; the hook uses these
    Values Defaults();

    // Checks, publishes and writes the ini. False with a reason when refused.
    bool Apply(const Values& v, char* why, size_t whyLen);
    // The same for a change to some fields, under the write lock, so two threads
    // changing different fields cannot undo each other.
    bool Update(const std::function<void(Values&)>& change, char* why, size_t whyLen);
    // True when a setting differs from what this launch started with.
    bool RestartNeeded();
}
