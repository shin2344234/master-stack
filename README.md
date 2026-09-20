# Stack Master

A Crimson Desert plugin that raises how much of an item one inventory slot
holds. Pick a multiplier and every item that already stacks holds that many
times as much: at 5, a stack of 100 becomes 500 and a stack of 20 becomes 100.
Built for Crimson Desert 2.02.00 and 2.03.00.

It changes no game file. The game reads its item table once while it starts, and
the plugin raises each item's own limit as that happens, in memory. That is the
difference from a data mod: nothing has to be reinstalled after a game patch,
and there is no archive to restore when you remove it.

Items the game does not stack, such as gear and quest items, never start
stacking. No stack goes past 999999, and an item the game already lets you hold
more of than that, like money, is left as it is.

## Install

Copy `StackMaster.asi` into `Crimson Desert\bin64`, beside the ASI loader you
already use. On first run it writes `StackMaster.ini` next to itself.

Then set the multiplier in `StackMaster.ini` and restart the game:

    Multiplier=5

`Enabled=0` turns the mod off. `DebugLog=1` writes a line for the first ten
items raised and keeps every session's log, which is what to send with a report.

With [Master Looter](https://www.nexusmods.com/crimsondesert/mods/3402) 1.6.33 or
later you can change the multiplier from its Stacks tab in game. Raising it takes
hold straight away if [Private Storage
Master](https://www.nexusmods.com/crimsondesert/mods/3521) is installed as well,
because this mod borrows its free-play check to know when that is safe. On its
own, and for any smaller number, the change waits for the next start.

## Turning it back down

Lowering the multiplier always waits for the next start. A slot can hold more
than the game allows, and shrinking a limit under a stack already over it would
leave it stranded there.

Your save records the stacks you build, so a slot can still hold more than the
game allows after you set the multiplier back to 1. It keeps what is in it until
you take some out. Empty the big stacks before turning it down.

## Other mods

[Private Storage Master](https://www.nexusmods.com/crimsondesert/mods/3521) 1.1.2
and later carry the same feature. With both installed, Stack Master is the one
that applies and Private Storage Master leaves stacks alone and says so in its
log, so the two never multiply the same limit twice. Set the multiplier here.

Do not run a stack-size data mod such as Fat Stacks as well. Both would change
the same limits and the result is whichever the game reads last.

## Building

MSVC Build Tools 2022 with the CMake and Ninja it bundles, then
`mod\build.bat`, which stages `mod\dist\StackMaster.asi`. MinHook is fetched at
configure time for its instruction length decoder; pass
`-DFETCHCONTENT_FULLY_DISCONNECTED=ON` to reconfigure without fetching again.

For other plugins, [mod/include/stack_api.h](mod/include/stack_api.h) is the
versioned C interface Master Looter's Stacks tab uses. Private Storage Master
exports the same names, so a caller finds whichever is installed.

## Antivirus

A scanner or two may flag the plugin, because the shape of what it does looks
like a trainer to a model: it is a DLL loaded into the game that searches the
game's code for a byte pattern and writes a jump over one of its functions. It
imports only kernel32 and user32, so there is no network code in it, and it reads
and writes no registry key and no game file. It is code signed by Seth Walker
under Microsoft's identity-verified chain. Every line is here to read or build
yourself.

SHA-256 for 1.0.0:

    37483b08773e8f40ebbb671395d49cd2c39e46003af30f9dbc9b90fd8da44b28  StackMaster-1.0.0-DMM.zip
    b294d98cec0615aea776acd1e13e4f498f013cbb94000957c68a3433daf48142  StackMaster-1.0.0.zip
    7eb1e71644466c2f8d42c1440b590402b42f065e442557eddcf9cc758c315319  StackMaster.asi

## Discord and Patreon

[Shin234's Mods 'n Stuff](https://discord.gg/AZ2ztQYy74) is the Discord. Bugs are
best as GitHub issues or on the Nexus bugs tab so they get tracked. Patreon is
[patreon.com/cw/Shin234](https://www.patreon.com/cw/Shin234). Everything
published stays free and nothing is held back for it.
