# Mad Max – Thunderdome (v0.9.0-beta1)

Mad Max shipped with a fighting arena that the player can never reach: a
caged dome on the edge of The Big Nothing, with stands, a gate, a host
(Crow Dazzle) and a wave spawner, switched off before release. This mod
switches it back on and turns it into a full pit fight.

**This is a beta.** Back up your saves before trying it.

**Download:** the ready-to-install package (plugin + data files) is on the
[Releases](https://github.com/PabloCGSilva/MadMax-Thunderdome/releases) page.
This repository holds the plugin's source.

## What you get

* **The arena on the map.** A *Pit Fight* flag marks it from the start, and
  it is a fast-travel point called *Thunderdome*. You can also drive there.
* **Six waves** of War Boys and Roadkills. Some jump down from the edge of
  the stands, others walk in through the cage gate: defenders with shields,
  gimps, and two **Top Dogs** in the last wave. The gate stays shut while you
  fight.
* **The Tenderloin** as the final opponent. A few seconds after the last
  wave she drops into the cage. Bring her down to win.
* **A crowd.** About 200 spectators cheer on the dome and in the stands.
* When the fight is over you are taken back outside the cage.

## Installation

1. Copy everything in this package into your Mad Max folder (the one with
   `AVAMain.exe`), merging the `scripts` and `dropzone` folders.
2. `dinput8.dll` is the Ultimate ASI Loader. If you already have it from
   another mod, keep yours.
3. Start the game. `scripts\Thunderdome.log` shows what the plugin found.

To uninstall, delete `scripts\Thunderdome.asi` and the files this package
added under `dropzone`.

## Why there is a plugin

Almost everything is game data in the `dropzone`. The small plugin does two
things data can't:

* The enemies that come in through the gate belong to the Roadkills, and the
  game only loads their models near their own territory. The plugin loads
  them when the arena starts. Without it the fight stops at wave 2.
* Saves remember whether each fast-travel point is on. After every save load
  the plugin switches the Thunderdome point back on.

## Requirements and compatibility

* Made and tested on the **GOG** version. Steam is untested: the plugin
  finds the game functions by pattern and waits for Steam's DRM like the
  author's other mods. If it can't find them it says so and the arena still
  loads, but the fight may stop at wave 2.
* It replaces, through the dropzone: `locations/world.bin` (two arena entries
  switched on), the arena files under `locations/a01/activities/thunderdome/`,
  `global/global.blo`, `global/mapicons.mapiconsc`,
  `missions/restartpoints.restartpointsc`, `gui/map.guixc` and the ten
  `text/master_gui_*.stringlookup` files. Another mod that replaces any of
  these files won't work together with this one. For example, mods that edit
  `world.bin`, like *Respawning Scrapulances*.
* Works alongside Enhanced Convoys, Wasteland Storms and Enhanced Archangels.

## Known issues

* **Invisible walls inside the cage.** A few spots on the cage floor block
  you, probably where the original spawn markers were. Under investigation.
* A sandstorm reaching the arena once froze the game for a few seconds
  before it recovered.
* Untested: fighting a second time in the same session, and loading a save
  made inside the arena during a fight.

## What to report

Please send `scripts\Thunderdome.log` (and `Thunderdome.previous.log` if you
restarted the game) with:

* your game version (GOG or Steam),
* how far the fight got (wave number, Top Dogs, Tenderloin),
* anything that stood still, fell through the floor or never showed up.

## Building the plugin

Visual Studio 2022 (v143), x64 Release:

```
MSBuild Thunderdome.vcxproj /p:Configuration=Release /p:Platform=x64
```

The output is `bin\Thunderdome.asi`. `tools/gen_sigs.py` regenerates the
byte signatures from a GOG `AVAMain.exe` (needs `pefile` and `capstone`).

## Shout outs

* **Rick Gibbed** for the Gibbed Mad Max tools, and **gigaHours** for the
  fork they were built with.
* **Tsuda Kageyu** for MinHook, **ThirteenAG** for the Ultimate ASI Loader.
* **Avalanche Studios** for Mad Max, and for leaving the Thunderdome in the
  game files.

## License

MIT for the plugin code (`thunderdome.cpp`, `tools/`), see `LICENSE`. The
bundled MinHook sources keep their own license. The data files in the release
package are modified copies of the game's own files, see
`THIRD-PARTY-NOTICES.md`.
