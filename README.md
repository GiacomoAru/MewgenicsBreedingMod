# Unnatural Selection

A Mewgenics mod that lets you tune how breeding passes on disorders and birth defects. Press **F8** in game to open the menu.

Download and page on Nexus Mods: <https://www.nexusmods.com/mewgenics/mods/543>

## How it works

Two settings, each with four levels:

| Setting | What it controls |
|---|---|
| **Inbreeding penalties** | New disorders and birth defects caused by breeding related cats. |
| **Inherited flaws** | Disorders and birth defects passed down from the parents. |

| Level | Inbreeding penalties | Inherited flaws |
|---|---|---|
| **Off** | Related parents count as unrelated: no new birth defects, and no disorder "out of nowhere". | Parents never pass on disorders or birth defects. |
| **Reduced** | Inbreeding counts half. | Each inherited flaw has a 50% chance of being blocked. |
| **Normal** | Game default. | Game default. |
| **Increased** | Inbreeding counts double: a pair with 25% inbreeding behaves like one with 50%. | Every flaw of a parent gets a second chance to pass on. |

Every disorder and every birth defect counts as a flaw, with no exceptions. If you want parents to pass on a trait you like (Eternal Youth, Cyclops...), keep *Inherited flaws* on Normal.

### Presets

| Preset | Inbreeding penalties / Inherited flaws | For |
|---|---|---|
| **Vanilla** | Normal / Normal | The game's own rules. |
| **Gentle** | Reduced / Reduced | Breed relatives with half the risks. |
| **Carefree** | Off / Normal | Breed relatives freely; parents still pass on their own flaws. |
| **Clean** | Off / Off | No disorders or birth defects from breeding. |
| **Hardcore** | Increased / Increased | Inbreeding hits harder and flaws spread more. |

Settings are saved automatically in `config.ini`, next to the mod's DLL.

### What the mod does not touch

Active abilities, passives, stats, normal mutations, and cats that are already born. The mod only acts at the moment a kitten is born, in the game's memory; the game then saves as usual. It never writes to your save file.

## Install

Requires [Mewtator](https://www.nexusmods.com/mewgenics/mods/1) and [Mewjector](https://www.nexusmods.com/mewgenics/mods/218), with "DLL Mod Support" enabled in Mewtator.

1. Unzip `UnnaturalSelection-<version>.zip` and copy the `UnnaturalSelection` folder into Mewtator's `mods` folder.
2. Enable the mod in Mewtator and launch the game from Mewtator.

Supported game version: **1.1.21239**. On any other version the menu shows "Inactive" and the mod changes nothing.

## Build from source

Needs Visual Studio 2022 Build Tools with the "Desktop development with C++" workload.

```
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Release    # release DLL -> mod\UnnaturalSelection\
powershell -ExecutionPolicy Bypass -File scripts\package.ps1           # release build + zip in outputs\
powershell -ExecutionPolicy Bypass -File scripts\build.ps1             # development build (simulator, test suite) -> outputs\dev\
```

Design and roadmap: [docs/DESIGN.md](docs/DESIGN.md), [docs/PLAN.md](docs/PLAN.md).

## Credits

Built on [p0lymeric](https://github.com/p0lymeric)'s Mewgenics reverse-engineering work and on [mewgenics-cat-bridge](https://github.com/z3ndroot/mewgenics-cat-bridge) by z3ndroot, both MIT licensed. See [ATTRIBUTION.md](ATTRIBUTION.md).
