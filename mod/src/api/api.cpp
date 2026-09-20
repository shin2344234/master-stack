// The exported interface in include/stack_api.h. Private Storage Master exports
// the same names from its own copy of the feature, so Master Looter's Stacks tab
// talks to whichever of the two is installed. This mod is the one that applies
// when both are, which is why nothing here ever reports standing down for
// another mod.
#define STACK_API __declspec(dllexport)
#include "stack_api.h"

#include <Windows.h>
#include <cstdio>
#include <cstring>

#include "core/log.h"
#include "core/settings.h"
#include "stacks/stacks.h"
#include "version.h"

#define STACK_EXPORT extern "C" STACK_API

// snprintf truncates without complaining, so a name that outgrows its field
// would quietly lose its tail. Fail the build instead.
static_assert(sizeof SM_NAME <= sizeof(StackStatus::provider), "SM_NAME does not fit StackStatus::provider");
static_assert(sizeof SM_MODULE <= sizeof(StackStatus::providerModule), "SM_MODULE does not fit StackStatus::providerModule");
static_assert(sizeof SM_VERSION <= sizeof(StackStatus::version), "SM_VERSION does not fit StackStatus::version");
static_assert(sizeof SM_GAME <= sizeof(StackStatus::gameVersion), "SM_GAME does not fit StackStatus::gameVersion");
static_assert(STACK_STANDDOWN_NONE == sm::stacks::kApplying && STACK_STANDDOWN_OFF == sm::stacks::kOff &&
              STACK_STANDDOWN_OTHER_MOD == sm::stacks::kOtherMod && STACK_STANDDOWN_NO_ANCHOR == sm::stacks::kNoAnchor &&
              STACK_STANDDOWN_HOOK_FAILED == sm::stacks::kHookFailed && STACK_STANDDOWN_TOO_LATE == sm::stacks::kTooLate,
              "the STACK_STANDDOWN_ codes and stacks::Reason must match");
static_assert(STACK_MAX_MULTIPLIER == sm::Settings::kMaxMultiplier,
              "STACK_MAX_MULTIPLIER and Settings::kMaxMultiplier must match");

STACK_EXPORT int StackApiVersion(void) { return STACK_API_VERSION; }

STACK_EXPORT int StackGetStatus(StackStatus* out)
{
    // Any size from the first published layout upward, so a caller built against
    // an older header keeps working: it is a prefix of this struct, it gets the
    // fields it knows, and `size` comes back saying how many bytes were written.
    if (!out || out->size < static_cast<uint32_t>(STACK_STATUS_V1)) return 0;
    const uint32_t want = out->size < sizeof(StackStatus) ? out->size : static_cast<uint32_t>(sizeof(StackStatus));
    const sm::stacks::Report r = sm::stacks::Status();
    StackStatus full;
    StackStatus* const fill = &full;
    memset(fill, 0, sizeof full);
    fill->size = want;
    snprintf(fill->provider, sizeof fill->provider, "%s", SM_NAME);
    snprintf(fill->providerModule, sizeof fill->providerModule, "%s", SM_MODULE);
    snprintf(fill->version, sizeof fill->version, "%s", SM_VERSION);
    snprintf(fill->gameVersion, sizeof fill->gameVersion, "%s", SM_GAME);
    fill->applying = r.reason == sm::stacks::kApplying;
    fill->standDownReason = r.reason;
    fill->hooked = r.hooked;
    fill->multiplier = r.multiplier;
    fill->multiplierSetting = sm::Settings::Get().multiplier;
    // Against what is in force, not against what the launch started with: a live
    // raise makes the two agree, and then no restart is needed. Enabled=0 is the
    // other half of RestartNeeded and is reported as a stand-down, not here.
    fill->restartNeeded = fill->multiplierSetting != fill->multiplier;
    fill->itemsRaised = r.patched;
    fill->itemsUnstackable = r.unstackable;
    fill->ceiling = STACK_CEILING;
    fill->maxMultiplier = STACK_MAX_MULTIPLIER;
    fill->liveRaise = sm::stacks::CanRaiseNow();
    fill->biggest = r.biggest;
    memcpy(out, fill, want);
    return 1;
}

STACK_EXPORT int StackStandDownText(int reason, char* out, int outLen)
{
    if (!out || outLen <= 0) return 0;
    switch (reason)
    {
    case STACK_STANDDOWN_NONE:
        snprintf(out, static_cast<size_t>(outLen), "%s is setting the stack sizes.", SM_NAME);
        return 1;
    case STACK_STANDDOWN_OFF:
        snprintf(out, static_cast<size_t>(outLen), sm::Settings::Get().enabled
                                                      ? "Installed, but switched off: stacks hold what the game gives them."
                                                      : "Turned off in StackMaster.ini with Enabled=0.");
        return 1;
    case STACK_STANDDOWN_OTHER_MOD:
        snprintf(out, static_cast<size_t>(outLen), "Another mod is setting the stack sizes.");
        return 1;
    case STACK_STANDDOWN_NO_ANCHOR:
        snprintf(out, static_cast<size_t>(outLen), "This game version keeps its item table somewhere the mod does not recognise, so stacks are "
                                                  "left alone. An update to %s is needed.", SM_NAME);
        return 1;
    case STACK_STANDDOWN_HOOK_FAILED:
        snprintf(out, static_cast<size_t>(outLen), "The item table could not be hooked, so stacks are left alone. The log says why.");
        return 1;
    case STACK_STANDDOWN_TOO_LATE:
        snprintf(out, static_cast<size_t>(outLen), "The game read its item table before the mod started, so stacks are the game's own this "
                                                  "session. A restart fixes it.");
        return 1;
    default:
        snprintf(out, static_cast<size_t>(outLen), "Stacks are not being changed, and this version does not know why (reason %d).", reason);
        return 0;
    }
}

STACK_EXPORT int StackGetMultiplier(void) { return sm::Settings::Get().multiplier; }

STACK_EXPORT int StackApplyMultiplier(int multiplier, char* why, int whyLen)
{
    if (why && whyLen > 0) why[0] = 0;
    if (multiplier < 1 || multiplier > STACK_MAX_MULTIPLIER)
    {
        if (why && whyLen > 0) snprintf(why, static_cast<size_t>(whyLen), "the multiplier has to be between 1 and %d", STACK_MAX_MULTIPLIER);
        return 0;
    }
    const auto change = [multiplier](sm::Settings::Values& v) { v.multiplier = multiplier; };
    if (!sm::Settings::Update(change, why, why && whyLen > 0 ? static_cast<size_t>(whyLen) : 0)) return 0;
    // Raising can take hold now. Anything else waits for the next launch, which
    // the caller sees as restartNeeded on its next status read. A refusal there is
    // not a failure of this call: the setting is saved either way.
    char note[192];
    if (!sm::stacks::RaiseNow(multiplier, note, sizeof note)) LOG("[stacks] x%d is saved for the next launch. Not now, because %s.", multiplier, note);
    return 1;
}
