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

#include "core/settings.h"
#include "stacks/stacks.h"
#include "version.h"

#define STACK_EXPORT extern "C" STACK_API

// snprintf truncates without complaining, so a name that outgrows its field
// would quietly lose its tail. Fail the build instead.
static_assert(sizeof MST_NAME <= sizeof(StackStatus::provider), "MST_NAME does not fit StackStatus::provider");
static_assert(sizeof MST_MODULE <= sizeof(StackStatus::providerModule), "MST_MODULE does not fit StackStatus::providerModule");
static_assert(sizeof MST_VERSION <= sizeof(StackStatus::version), "MST_VERSION does not fit StackStatus::version");
static_assert(sizeof MST_GAME <= sizeof(StackStatus::gameVersion), "MST_GAME does not fit StackStatus::gameVersion");
static_assert(STACK_STANDDOWN_NONE == mst::stacks::kApplying && STACK_STANDDOWN_OFF == mst::stacks::kOff &&
              STACK_STANDDOWN_OTHER_MOD == mst::stacks::kOtherMod && STACK_STANDDOWN_NO_ANCHOR == mst::stacks::kNoAnchor &&
              STACK_STANDDOWN_HOOK_FAILED == mst::stacks::kHookFailed && STACK_STANDDOWN_TOO_LATE == mst::stacks::kTooLate,
              "the STACK_STANDDOWN_ codes and stacks::Reason must match");
static_assert(STACK_MAX_MULTIPLIER == mst::Settings::kMaxMultiplier,
              "STACK_MAX_MULTIPLIER and Settings::kMaxMultiplier must match");

STACK_EXPORT int StackApiVersion(void) { return STACK_API_VERSION; }

STACK_EXPORT int StackGetStatus(StackStatus* out)
{
    if (!out || out->size != sizeof *out) return 0;
    const mst::stacks::Report r = mst::stacks::Status();
    memset(out, 0, sizeof *out);
    out->size = sizeof *out;
    snprintf(out->provider, sizeof out->provider, "%s", MST_NAME);
    snprintf(out->providerModule, sizeof out->providerModule, "%s", MST_MODULE);
    snprintf(out->version, sizeof out->version, "%s", MST_VERSION);
    snprintf(out->gameVersion, sizeof out->gameVersion, "%s", MST_GAME);
    out->applying = r.reason == mst::stacks::kApplying;
    out->standDownReason = r.reason;
    out->hooked = r.hooked;
    out->multiplier = r.multiplier;
    out->multiplierSetting = mst::Settings::Get().multiplier;
    out->restartNeeded = mst::Settings::RestartNeeded();
    out->itemsRaised = r.patched;
    out->itemsUnstackable = r.unstackable;
    out->ceiling = STACK_CEILING;
    out->maxMultiplier = STACK_MAX_MULTIPLIER;
    out->biggest = r.biggest;
    return 1;
}

STACK_EXPORT int StackStandDownText(int reason, char* out, int outLen)
{
    if (!out || outLen <= 0) return 0;
    switch (reason)
    {
    case STACK_STANDDOWN_NONE:
        snprintf(out, static_cast<size_t>(outLen), "%s is setting the stack sizes.", MST_NAME);
        return 1;
    case STACK_STANDDOWN_OFF:
        snprintf(out, static_cast<size_t>(outLen), mst::Settings::Get().enabled
                                                      ? "Installed, but switched off: stacks hold what the game gives them."
                                                      : "Turned off in MasterStack.ini with Enabled=0.");
        return 1;
    case STACK_STANDDOWN_OTHER_MOD:
        snprintf(out, static_cast<size_t>(outLen), "Another mod is setting the stack sizes.");
        return 1;
    case STACK_STANDDOWN_NO_ANCHOR:
        snprintf(out, static_cast<size_t>(outLen), "This game version keeps its item table somewhere the mod does not recognise, so stacks are "
                                                  "left alone. An update to %s is needed.", MST_NAME);
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

STACK_EXPORT int StackGetMultiplier(void) { return mst::Settings::Get().multiplier; }

STACK_EXPORT int StackApplyMultiplier(int multiplier, char* why, int whyLen)
{
    if (why && whyLen > 0) why[0] = 0;
    if (multiplier < 1 || multiplier > STACK_MAX_MULTIPLIER)
    {
        if (why && whyLen > 0) snprintf(why, static_cast<size_t>(whyLen), "the multiplier has to be between 1 and %d", STACK_MAX_MULTIPLIER);
        return 0;
    }
    const auto change = [multiplier](mst::Settings::Values& v) { v.multiplier = multiplier; };
    return mst::Settings::Update(change, why, why && whyLen > 0 ? static_cast<size_t>(whyLen) : 0) ? 1 : 0;
}
