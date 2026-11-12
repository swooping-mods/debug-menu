# Debug Menu: behavior and implementation

Describes the retained payload **0.1.0**. The behavior prose describes the retained payload; current candidate source and metadata are identified below. Gameplay qualification is a separate requirement.


<!-- source-only-sync -->
## Current source candidate

Source version **0.1.1** is saved from workshop revision `ca32bbf79ce0840d5166a232bdb3a9fdcd761254`. The retained installable payload remains **0.1.0** with its original bytes. This source update does not rebuild or requalify that payload. The current source and metadata define the candidate; gameplay, rendered behavior and co-op remain unqualified for rebuilt bytes.

The authored behavior and lifecycle descriptions below are retained from the published payload. They are not new qualification evidence for the source candidate. The integration table below follows the current candidate metadata.
<!-- /source-only-sync -->

## Behavior

F4 toggles an in-game menu with Player, World and Inspect tabs. Controls expose native funds/catalog actions, movement/flight settings, position bookmarks and diagnostic snapshots. The panel uses bounded native UI and invokes the game-facing operations from the admitted player callback. It is a development/debug feature whose individual actions can change world or economy state.

## State and lifecycle

Menu visibility, UI roots and bookmarks belong to the active session. Native operations retain their own save semantics; a menu action is not a separate save format. World/walker changes release the mod-owned UI and captured state. Input focus and UI interaction gates prevent unrelated controls firing through the panel.

## Integration and configuration

No additional feature-mod runtime dependency is declared. Included shared headers are source reuse, not extra DLL dependencies.

The [module metadata](../source/mods/debug-menu/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

## Implementation sources

- [mod.c](../source/mods/debug-menu/mod.c)
- [menu.h](../source/mods/debug-menu/menu.h)
- [core.h](../source/mods/debug-menu/core.h)

## Verification boundary

A menu opening does not qualify every debug action, save mutation or multiplayer use. Apply action-specific checks on disposable worlds; this is not an access-control or server-administration system.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
