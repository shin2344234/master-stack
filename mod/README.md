Stack Master 1.0.2 for Crimson Desert 2.02.00, 2.03.00 and 2.03.02
==================================================================

Raises how much of an item one inventory slot holds. Pick a multiplier and
every item that already stacks holds that many times as much: at 5, a stack of
100 becomes 500 and a stack of 20 becomes 100.

No game file is changed. The game reads its item table while it starts, and the
plugin raises each item's own limit as that happens, in memory. That is the
difference from a data mod: nothing to reinstall after a game patch, and nothing
to put back when you remove it.

Install
-------

1. Ultimate ASI Loader must be in bin64 next to CrimsonDesert.exe. If it is
   named version.dll and nothing loads, rename it to winmm.dll.
2. With the game closed, copy StackMaster.asi into bin64.
3. Start the game once. StackMaster.ini appears beside the plugin.
4. Set the multiplier in the ini and start the game again.

To uninstall, delete the StackMaster files from bin64. Read "Turning it back
down" first.

Settings
--------

StackMaster.ini, written on the first run with every setting explained.

    Multiplier=5      how much one slot holds, as a multiple of the game's own
                      limit for that item. 1 changes nothing and the plugin
                      installs no hook at all.
    Enabled=0         turns the whole mod off.
    DebugLog=1        writes a line for the first ten items raised and keeps
                      every session's log. Send this with a report.

Items the game does not stack, such as gear and quest items, never start
stacking. No stack goes past 999999, and an item the game already lets you hold
more of than that, like money, is left as it is. Replenishing Arrows, Bullets
and Cannonballs keep the game's own stack size.

Editing the ini takes effect the next time the game starts, because the file is
read once. With Master Looter 1.6.33 or later you can change the multiplier from
its Stacks tab while you play, and raising it takes hold at once as long as
Private Storage Master is installed too, since this plugin borrows that mod's
free-play check to know when a change is safe. On its own, a change from the tab
waits for the next start.

Turning it back down
--------------------

Lowering the multiplier always waits for the next start. Your save records the
stacks you built, so a slot can still hold more than the game allows afterwards.
It keeps what is in it until you take some out. Empty the big stacks first, and
the same goes for uninstalling.

Other mods
----------

Private Storage Master 1.1.2 and later carry this same feature. With both
installed, Stack Master is the one that applies and Private Storage Master
leaves stacks alone and says so in its log, so the two never multiply the same
limit twice. Set the multiplier here.

Do not run a stack-size data mod such as Fat Stacks as well. Both change the
same limits and the result is whichever the game reads last.

Reporting a problem
-------------------

Set DebugLog=1, play until it happens, close the game and attach
StackMaster.log from bin64. It says how many items were raised, the biggest
limit written, and names the first ten. For anything about the Stacks tab,
attach MasterLooter.log too.

Building
--------

MSVC Build Tools 2022 with its bundled CMake and Ninja. Run mod\build.bat by
full path; the plugin lands in mod\dist. mod/src/game/addresses.cpp holds the
one byte pattern the plugin searches for, and mod/include/stack_api.h is the
interface another plugin can call.
