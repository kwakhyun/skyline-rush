# Unreal runner project instructions

## Active direction — 2026-09-06

The user explicitly replaced all previous game design and setting. The active game is SKYLINE RUSH: a fast third-person endless runner starring the existing silver-haired girl. Previous story, combat, boss, chip, campaign, and city-expansion documents are historical only and must not constrain the runner.

- Reuse suitable project-owned character, animation, environment, audio and implementation assets.
- Automatic forward running; three lanes; jump and timed slide. Distance increases speed and terrain/obstacle difficulty.
- The runner's rules, procedural course and collisions are deterministic C++; visuals share the same obstacle definitions.
- Generated runs must retain a reachable route and sufficient warning distance. Course instances and memory remain bounded.
- The runner has its own entry map, game mode, controls, HUD and record save slot. Do not route its launch through legacy story or combat.
- Do not copy Sonic/Temple Run characters, levels, branding or external copyrighted assets.
- Keep generated Unreal caches out of Git and binary assets in Git LFS. Preserve unrelated user changes.
- Run focused compile and runner tests during iteration.
- The user owns creative/playtest judgement, commercial licensing decisions and Steam account actions.

See Docs/GAMEPLAY.md and Docs/ARCHITECTURE.md for the current controls and implementation.

Project: SkylineRush.uproject. Module: Source/SkylineRush. Entry map: /Game/SkylineRush/Maps/L_SkylineRush. QA outputs belong in Saved/QA.
