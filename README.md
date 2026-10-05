# Clean Breeding

A Mewgenics mod (work in progress) that adds an in-game menu (**F8**) to tune breeding:

- **Inbreeding:** Hard / Vanilla / Mild / None. Scales how much inbreeding causes birth defects and disorders.
- **Heredity:** Hard / Vanilla / Mild / None. Scales how often parents pass on their disorders and birth defects.
A whitelist in `config.ini` protects the disorders worth keeping (e.g. Eternal Youth, Savant Syndrome).

## Install

Requires [Mewtator](https://www.nexusmods.com/mewgenics/mods/1) and [Mewjector](https://www.nexusmods.com/mewgenics/mods/218), with "DLL Mod Support" enabled in Mewtator.

1. Copy `mod/CleanBreeding/` into Mewtator's `mods` folder.
2. Enable the mod in Mewtator and launch the game from Mewtator.

Supported game version: 1.1.21239.

## Build

Needs Visual Studio 2022 Build Tools with the "Desktop development with C++" workload.

```
powershell -ExecutionPolicy Bypass -File scripts\build.ps1
```

Design and roadmap: [docs/DESIGN.md](docs/DESIGN.md), [docs/PLAN.md](docs/PLAN.md).

## Credits

Built on [p0lymeric](https://github.com/p0lymeric)'s Mewgenics reverse-engineering work and on [mewgenics-cat-bridge](https://github.com/z3ndroot/mewgenics-cat-bridge), both MIT licensed. See [ATTRIBUTION.md](ATTRIBUTION.md).
