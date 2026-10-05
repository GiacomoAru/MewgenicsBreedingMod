# RE notes

Facts about Mewgenics internals found or confirmed in this project. Each entry says how it was checked.
For facts taken from upstream, see `reference/mewgenics-cat-bridge/docs/DEVELOPMENT.md` and
`reference/mewgenics_analysis/imhex_patterns/`.

## Confirmed before S0 (2026-10-05)
- The local `Mewgenics.exe` SHA256 is `4127cd6a792ae528bca6f65a8873dd61789591937d87656c2b586a5e30eb77ea`, the same as `EXE_SHA256` in Amoeba and in the template (1.1.21239). How checked: `sha256sum`.
- Breeding probabilities are not in the game data (`data/*.gon`); they are hardcoded in the exe. How checked: grep of every `.gon` extracted from `resources.gpak`.
- `data/passives/disorders.gon` defines 125 disorders, all `class Disorder`. Birth-defect body parts are tagged `tag birth_defect` in `data/mutations/*.gon`, ids 700+ and -2. How checked: parsed the extracted files.
- The save is SQLite: table `cats(key, data BLOB)`, with LZ4 blobs. How checked: read-only open of the save.

## S3 probe results
(to fill in)
