# DayZPortals

Craftable portals for DayZ, after Valheim's: build two, tune them to the same frequency, and
touching one plate puts you at the other. In DayZ they're the **Pereval transit apparatus
(Object 47)**, a Soviet field gate that was clearly never meant to leave the test range.
For now the gate is kit-bashed from vanilla models; a proper model can replace it later
without touching the logic. Targets DayZ 1.30 (stable since 2026-10). Server and client.

**Version 0.1.** Download the mod from the GitHub release, or build it yourself (below).

## Playing it

1. **Crate it:** combine an *electronic repair kit* with *metal wire* in your hands. You get
   a *Pereval apparatus (crated)*. It's heavy, so it goes in your hands only.
2. **Place it** from your hands like a fence kit. The hologram shows the whole gate.
3. **Fit the parts** into its inventory: 10 *metal sheets*, *metal wire*, a *truck battery*
   and a *walkie-talkie*.
4. **Assemble it** with a *hammer* in your hands. This uses up the parts. The ring stands
   up, the mist comes on and the rim starts to spark.
5. **Tune it:** look at the cabinet and use *Tune up* / *Tune down*. There are 20
   channels, 412.0-421.5 MHz in 0.5 MHz steps. A new gate starts on a random channel.
6. **Go through:** walk up to the plate, from either side. You come out 2.6 m in front of
   another assembled gate on the same frequency. If more than two gates share a frequency,
   the field picks one at random.

The interlock won't send anyone carrying **ammunition or explosives**: loose rounds, loaded
magazines, a chambered or internally loaded gun, grenades (smoke is fine), mines and
charges. Empty guns and magazines are fine. *Crate* (hammer) packs a gate back into its
crate: an empty unassembled gate loses nothing, an assembled one loses its parts.

## Layout

- `DayZPortals/` - the mod (packed as-is into a PBO, no binarising).
  - `config.cpp` - `DZP_GateKit`, `DZP_GateHologram`, `DZP_Gate`.
  - `Scripts/3_Game/DayZPortals/DZP_PPEShimmer.c` - the post-process requester for the
    air shimmer.
  - `Scripts/4_World/DayZPortals/`
    - `DZP_Layout.c` - where every vanilla piece sits relative to the cabinet, the trigger
      box, the arrival point, the channels and the material counts. **Tune the look here.**
    - `DZP_Decor.c` - spawns the pieces as local, non-persistent objects on the server
      (for collision) and on each client (for looks).
    - `DZP_Gate.c` - the gate: frequency, assembly, crating, persistence, ambience.
    - `DZP_GateRegistry.c` - server: watches the plates (every 250 ms), runs the
      contraband check and the jump.
    - `DZP_GateKit.c` - the crate, and the placement hologram.
    - `DZP_GateShimmer.c` - client: near an assembled gate the view wobbles round the
      grille, stronger the closer you are. **Tune its strength here.**
    - `DZP_Actions.c`, `DZP_Hooks.c` - tune/assemble/crate actions, the recipe, and the
      vanilla hooks (`ActionConstructor`, `PluginRecipesManagerBase`, `Hammer`, `PlayerBase`).
- `mission/portals.chernarusplus/` - the test mission (`init.c` has the headless checks and
  the `-tour` camera walk).
- `ServerFiles/types.xml` - CE entries. Without them the gates don't persist. Register the
  file in the mission's `cfgeconomycore.xml` with a `<ce folder="...">` block.

## Building and testing

The build and test tooling lives in
[DayZMissions](https://github.com/jasonuithol/DayZMissions), cloned next to this repo. The test
mission is here, in `mission/portals.chernarusplus`. Link both into DayZMissions; the links are
local only, not part of that repo:

```bash
cd ~/Projects/DayZMissions
ln -s ../../DayZPortals/DayZPortals mods/DayZPortals
ln -s ../../DayZPortals/mission/portals.chernarusplus missions/portals.chernarusplus
```

The mission loads the mod as a `local` mod. It puts an assembled pair of gates on 412.0 MHz,
one at Balota airfield where you spawn and one at the NW airfield. Next to you are two crated
gates with the parts and a hammer, and an AKM with a loaded magazine to try the interlock.

```bash
cd ~/Projects/DayZMissions
tools/test.sh portals          # headless: builds, assembles, contraband check, a scripted jump
KEEP=1 tools/test.sh portals   # same, on the last run's persistence (gates must survive)
tools/run_server.sh portals    # play it: you spawn next to an assembled gate at Balota
tools/run_client.sh portals    #   linked to one at the NW airfield, with kits and parts
```

`tools/build_mod.py mods/DayZPortals build` (from DayZMissions) builds and signs
`build/@DayZPortals` on its own.

## Not checked in game yet

The headless test covers the logic. These need a client:

- The look has been checked from the front and side (`screenshots/`), with
  `tools/run_server.sh portals -tour=1`: the server walks the player round the gate every
  12 s. Still unseen: the unbuilt gate (blast door lying flat), and the hologram.
- The placement hologram, the attachment slots in the inventory screen and the action texts.
- The mist, the sparks and the sounds (only a faint puff and sparks were visible) (all vanilla: contaminated-area gas, lever sparks,
  bunker hum, defibrillator).
- The jump on a real client. Server-side `SetPosition` is what admin tools use, so it
  should be fine.
