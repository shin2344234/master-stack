#pragma once
#include <cstdint>

// The one game address this mod needs, found at runtime from a byte pattern
// rather than written down as an offset, so a game patch that moves code costs
// nothing and a patch that changes the code turns the feature off with a log
// line instead of writing somewhere wrong.
namespace mst::addr
{
    struct Stacks
    {
        uintptr_t itemInfoRead = 0;   // bool (stream, ItemInfo record)
    };

    // False, with what is missing in the log.
    bool ResolveStacks(Stacks& out);
}
