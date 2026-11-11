# Changelog

## 0.2 - 2026-11-11

The gates are no longer built by players: they are derelict Soviet installations.

- New look, designed in the DayZ Editor: three steel gantries round a chamber, fence panels,
  a gas tank and a transformer, and the radio panel - which is the gate itself, and where you
  tune it. A rusting, peeling, buckled ОПАСНО - ВЗРЫВЧАТКА ЗАПРЕЩЕНА sign hangs by the panel.
- Step into the chamber: lightning strikes, and a moment later everyone in it goes - people on
  foot and one car or motorbike with its crew.
- Five seconds of deep space in transit.
- Nothing is refused and nothing is announced. Explosives go off in the lightning; ammunition
  cooks off (2 health a round); a box of rounds kills; smoke grenades hurt.
- A gate with no partner on its frequency is dead: no shimmer, no crackle, no strike.
- Tuning remembers which way you last tuned.
- No more gas mist; you arrive inside the far chamber and it won't fire until you step out.
- Placing gates: `DZP_Gate.DZP_SpawnAssembled(pos, channel)` from a mission's `init.c`.

## 0.1 - 2026-10-29

First playable version: a craftable ring gate kit-bashed from vanilla models, linked by radio
frequency, with an interlock that refused ammunition and explosives.
