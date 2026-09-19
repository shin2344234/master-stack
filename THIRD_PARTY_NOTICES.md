# Third party notices

## MinHook

The build fetches MinHook for one file, the HDE64 instruction length decoder,
which the hook installer uses to measure the bytes it copies out of a function
before writing over them. No MinHook hook is ever created.

MinHook, by Tsuda Kageyu. https://github.com/TsudaKageyu/minhook, MIT licence.
HDE 64C, by Vyacheslav Patkov, is included in MinHook under its own two-clause
BSD licence. Both licence texts ship with that repository.

## Earlier mods with the same idea

Master Stack was written from scratch and contains no code from any other mod.

**Fat Stacks, by momenaya.** https://www.nexusmods.com/crimsondesert/mods/157.
Fat Stacks gives the same result a different way, by replacing a game data group
and the archive manifest that lists it, which is why it has to be rebuilt for
each game version. Nothing from it is used here: no file of its is read, and the
field this plugin writes was found by reading the game's own item table loader.

## Shared code

The logging, path, memory and hook helpers are the same ones used in Private
Storage Master, Master Looter and Glint Spotter, all by the same author and all
MIT.
