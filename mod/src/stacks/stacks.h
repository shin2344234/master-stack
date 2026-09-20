#pragma once
#include <cstdint>

// The whole feature: hook the reader that fills each ItemInfo record and
// multiply the record's own stack limit. Nothing in the game's files is touched,
// which is what separates this from a data mod like Fat Stacks, and why a game
// patch does not break it.
namespace sm::stacks
{
    // Why stacks are not being changed. The numbers are the STACK_STANDDOWN_*
    // values in include/stack_api.h and must stay in step with them.
    enum Reason
    {
        kApplying = 0,
        kOff = 1,          // Multiplier is 1, or Enabled=0
        kOtherMod = 2,     // unused here: this mod is the one that takes precedence
        kNoAnchor = 3,     // the item table reader was not found
        kHookFailed = 4,
        kTooLate = 5,      // the game read its item table before the hook went in
    };

    void Start();
    // One line saying what the item table came out as, once the game has read it.
    void Flush();
    void Stop();

    // Raise the limits already in memory to a bigger multiplier, without a
    // restart. Only upwards: a slot can hold more than the game allows, so
    // lowering a limit under a stack already over it is left to the next launch,
    // where the game builds everything from the raised data itself.
    //
    // False when it cannot be done now, with the reason in `why`. The setting is
    // saved by whoever called this either way.
    bool RaiseNow(int multiplier, char* why, size_t whyLen);
    // Whether a raise could apply this session at all. It needs the hook and a
    // way to tell that the player is in free play with no storage screen open,
    // which this mod gets from Private Storage Master when that is installed. On
    // its own it has no view of the game's screens, so it answers false and a new
    // multiplier waits for the next launch.
    bool CanRaiseNow();

    struct Report
    {
        bool hooked = false;
        Reason reason = kOff;
        int multiplier = 1;          // in force this launch; 1 when nothing is changed
        int patched = 0;
        int64_t biggest = 0;
        int unstackable = 0;
    };
    Report Status();
}
