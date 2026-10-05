# RE notes

Facts about Mewgenics internals found or confirmed in this project. Each entry says how it was checked.
For facts taken from upstream, see `reference/mewgenics-cat-bridge/docs/DEVELOPMENT.md` and
`reference/mewgenics_analysis/imhex_patterns/`.

## Confirmed before S0 (2026-10-05)
- The local `Mewgenics.exe` SHA256 is `4127cd6a792ae528bca6f65a8873dd61789591937d87656c2b586a5e30eb77ea`, the same as `EXE_SHA256` in Amoeba and in the template (1.1.21239). How checked: `sha256sum`.
- Breeding probabilities are not in the game data (`data/*.gon`); they are hardcoded in the exe. How checked: grep of every `.gon` extracted from `resources.gpak`.
- `data/passives/disorders.gon` defines 125 disorders, all `class Disorder`. Birth-defect body parts are tagged `tag birth_defect` in `data/mutations/*.gon`, ids 700+ and -2. How checked: parsed the extracted files.
- The save is SQLite: table `cats(key, data BLOB)`, with LZ4 blobs. How checked: read-only open of the save.

## S3 probe results (2026-10-05, test slot 1, 3 `breed` calls)

Source: `mod_logs/chainloader.log` of the run at 12:56-12:58, hook in `src/clean_breeding/breed.cpp`. Calls: #1 and #2 = 955 x 958 (coi_param 0.25), #3 = 955 x 876 (coi_param 0).

1. **One call per kitten?** Open, 🛑 waiting for the user's count of kittens born. The 3 calls have 3 different `kitten_ptr`, and `kitten->sql_key` is -1 at return (the game assigns the key later). If the user saw 3 kittens, the answer is yes. Twins are not yet separated from two separate births.
2. **Empty disorder slot** (verified, all 6 slots of 2 parents x 3 calls, and the 3 kittens): the string is `"None"` (size=4, res=15, bytes `4e6f6e65`), level `1`. It is not an empty string and not level 0. Matches cat-bridge (`SET_PASSIVE ... None` clears a slot). Hypothesis: the "None" slot always has level 1; seen only on cats without disorders. The PLAN steps that "hide" or "remove" a disorder must therefore write `"None"` with level 1, not an empty string.
3. **`kitten->coi` after `breed` = `coi` parameter?** Yes in all 3 calls (0.25, 0.25, 0). Note the parents' own `coi` fields (0 and 0.4296 for key 876) are not the pair's coi: the parameter is the pair's kinship (#3: parentB has its own coi 0.43, param 0).
4. **Slot → `.gon` map.** Verified against the `.gon`: `data/mutations/legs.gon` id 700 = "lobster claw", `birth_defect`; `data/mutations/eyes.gon` id 701 = "anophthalmia", `birth_defect`.
   - Kitten of call #1 (parents without defects, coi 0.25): `arm1 = arm2 = 700` -> arms use `legs.gon` (matches DESIGN).
   - Kitten of call #2: `leye = reye = 701` -> eyes use `eyes.gon` (matches DESIGN).
   - Hypothesis, still to confirm in game by the user: those two kittens really show lobster-claw arms / missing eyes.
   - Observed in every log line: `arm1/arm2`, `leg1/leg2`, `leye/reye`, `lbrow/rbrow`, `lear/rear` always hold the same id, in parents and kittens. Hypothesis: the game inherits them as pairs. If so, the "defect replacement" in S7 must treat each pair as one unit.
   - Not yet seen: `body`, `head`, `tail`, `ears`, `mouth`, `eyebrows`, `texture` defect ids (no defect there in these 3 calls).
5. **Two disorders in a kitten, slot order?** Not answerable: no parent or kitten had any disorder in these 3 calls. Needs a pair where one parent has a disorder.

### S3 second run, with house snapshots (2026-10-05, 13:05, slot 1)

Source: `snapshots/snap_000..002` of the Mewtator mod folder (JSON of all cats in memory after each `breed`), 2 `breed` calls: #1 955 x 876 (day 323), #2 970 x 953 (day 324), both coi_param 0.

- **Q1, one call per kitten: yes, in this run.** Each call produced exactly one new cat in memory: call #1 -> cat 971 "Deimos" (15 of 15 slots equal to the kitten returned by `breed`, already present in the first snapshot, taken the frame after `breed`); call #2 -> cat 973 "Dr. Cortex" (identical parts and disorders to the kitten returned by `breed`). Cat 974 "Ailsa" appeared at the same time but is the daily stray (`in_house=false`, no room, parts unrelated to any pair), not a result of `breed`. No twins occurred, so twins = 2 calls is still unverified (hypothesis).
- **Kitten identity:** `kitten->sql_key` is -1 when `breed` returns; the game assigns the real key within the next frame. The kitten as returned by `breed` equals the cat the game keeps (parts, disorders): nothing else rewrites it afterwards in these 2 cases.
- **Birthday:** `birthday` of the kittens = `current_day` at `breed` time minus 1 (322 at day 323, 323 at day 324). Hypothesis, 2 samples.
- **Q5 and disorder representation:** only one cat has a disorder (970 "Miguel Felipe": `BirdFlu` level 1 in `mutation_0`, `None` level 1 in `mutation_1`). So with a single disorder it sits in slot 0 (1 sample). Its kitten 973 inherited nothing (both `None`, level 1). No cat in the house has 2 disorders, so the order of two disorders is still not observable. Hypothesis: slots are filled 0 then 1.
- Snapshot detail: the "new key" detection of `snapshot.cpp` missed the kitten of call #1, because the game creates the kitten's cat before the first snapshot. Use the snapshot diff offline, not the `new_keys` field alone.

### S3 third run, disorders injected into live cats (2026-10-05, 13:14, slot 1)

Setup: `[debug] test_disorders` wrote disorders into 8 cats at launch (cat-bridge technique `destroy()` + `construct()`, confirmed by the log: previous value `'None' lvl 1` in every slot). 5 `breed` calls observed (`snap_000..002` of that run).

- **Q5 answered (1 sample, still a hypothesis for the general case).** Call 1: A=969 `[Fidgety, Dyslexia]` x B=958 `[None, Insomnia]` (disorder only in slot 1, hole in slot 0), coi 0.304 -> kitten `[Insomnia, Schizophrenia]`, levels 1,1.
  - The inherited disorder (Insomnia, slot 1 of the parent) landed in **slot 0** of the kitten: the game fills the kitten's first free slot, it does not copy the parent's slot index.
  - The second disorder (Schizophrenia) is neither parent's: it is the inbreeding roll (coi 0.304), placed in slot 1 after the inherited one. So slots are filled 0 then 1, no holes.
  - Consequence for the plan: when S6/S7 remove disorders from a kitten they must compact the slots (e.g. new-from-inbreeding in slot 1 removed leaves slot 0 as is; an inherited one in slot 0 removed with a new one in slot 1 means moving slot 1 to slot 0). A hole in slot 0 on a parent (injected cat, `None` then a disorder) did not stop the game from running or breeding in this run (no crash report, 5 breed calls logged); whether the UI displays such a cat correctly was not checked by the user explicitly. Hypothesis: avoid creating holes in kittens anyway.
- Calls 2-4 (A=971 `[None, None]` x B=963 `[None, Pox]`, coi 0.304): kittens had no disorders: Pox (15%) not inherited in any of the 3 calls, no inbreeding disorder either. Call 5 (970 `BirdFlu` x 958 `Insomnia`, coi 0): kitten `[None, None]`.
- **Defects:** call 3 produced a new defect: `leg1 = leg2 = 703` with both parents `122` (legs.gon 703 = "syndactyly", `birth_defect`, checked in the `.gon`). So leg slots also receive new defects as a pair. Mouth id 1500 (rat mouth) appears in parents and kittens, but it is **not** a defect (`tag animal` in mouth.gon): the rule "id >= 700 = defect" applies only to ids with `tag birth_defect`, never to 1500.
- Kitten at `breed` return equals the final cat in all 5 calls (cats 975, 976, 978, 979, 980: 15 of 15 part slots and the disorder names equal). Cat 977 is not from `breed` (daily stray, hypothesis from the gap in keys).

Also seen: the 2 defects above are new (neither parent had a 700+ id in those slots) and appeared with coi 0.25 (formula: 37.5% per kitten for a new defect), consistent with the inbreeding mechanic.

## S5 simulator baselines (2026-10-05, Vanilla behaviour, N = 10000 kittens per case, 5 synthetic parent pairs each)

Source: test suite of `simulator.cpp`, log of 2026-10-05 (menu level was inbreeding=2 but the breed hook did not act on settings yet, so these are the unmodified game rates). Parents are random game strays cleaned of disorders and bad parts.

| Case | Result | Expected (DESIGN) |
|---|---|---|
| clean x clean, coi 0 | new disorders 2.3%, new bad parts 0% | 2%, 0% |
| coi 0.125 | 1.9%, 18.8% | 2%, 18.75% |
| coi 0.25 | 4.2%, 37.6% | 4%, 37.5% |
| coi 0.5 | 14.3%, 74.8% | 14%, 75% |
| coi 1.0 | 33.8%, 100% | 34%, 100% |
| A: Pox, coi 0 | inherited 15.0% (new 2.0%) | 15% |
| A: Pox, B: Flu | inherited (any) 28.0% | 27.75% |
| A: Pox+Flu | inherited 15.6% | 15% |
| A: EternalYouth | inherited 14.8% | 15% |

Facts verified by these rates (the formulas of DESIGN.md, which came from the wiki, match the game): disorder inherited 15% per parent, one disorder per parent; inbreeding disorder `max(2%, 0.4*coi - 6%)` up to 34%; new defect `min(1, 1.5*coi)`.

Baselines for S7 (defective parent x clean parent, coi 0, share of kittens that inherit at least one defective slot): legs 700 on all 4 leg/arm slots 73.4%; eyes 701 48.7%; head 704 49.2%. So each independent slot group (legs, arms) is inherited from a defective parent about 49% of the time (73.4% = 1 - (1-p)^2 with p about 0.49, hypothesis: legs and arms are two independent groups), a bit under 50%.

Ability check baselines (the mod must not change them): active from a parent 21-22%, passive from a parent 0% (synthetic strays have no passives). Stable across all cases and coi values.
