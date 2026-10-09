# HRIP — Hearthstone Rip

New repo (2026-10-09), NORTHSTAR only — no code yet, local repo (no upstream yet). Purpose: a
pure-PARENA, batteries-included Windows app that extracts Hearthstone's own card data from a real
local install (Unity AssetBundles under `Program Files\...\Hearthstone\Data\Win\`) and emits JSON
shaped to match WOTAN's `hs_cards` table, which a second screen upserts into IDUNA. Community card
dumps (e.g. HearthstoneJSON) are a verification/feedback-loop reference only, per founder direction
— never the shipped data path. Ships as a zip (`run.bat` + exe), crash/error logs always written to
a file next to the exe (no command-line diagnosis required). **Read `NORTHSTAR.md` first** — it
holds the checked PARENA-capability audit (what's real vs. source-only-unexecuted), the real gaps
named honestly (no Unity-asset-reader, no Windows crash-handler, no log-streaming-with-projection
precedent anywhere in this monorepo), the V0 cut, and the phased plan now carded into
`EMILY/BACKLOG.md` SECTION 600 / kanban.

## Related repos
`PARENA` (stdlib this app is built from: `io`, `map`, `csv`, `json`, `compress/lz4`, `sdl2`),
`IDUNA` (the upsert endpoint this app depends on, not yet built — HRIP-10), `WOTAN` (`hs_cards`
schema this app's JSON output must match exactly), `REDGARDEN` (the crash-to-file-log signal-handler
pattern HRIP's Windows logger is modeled on), `SHANKPIT` (the CI/CD auto-release shape HRIP's own
release workflow mirrors).

## Standing rules (monorepo)
Core deps implemented in PARENA first (`IDUNA/CLAUDE.md` "Core Deps Are PARENA-First") — if the
PARENA emitter can't yet execute a needed stdlib package end-to-end, fixing that is the real next
task, not reaching for a third-party library as the shipped mechanism. LZ4 is the default
compression codec. Always commit and push completed work immediately. Founder real-time direction
goes through `emily observe -s info "Founder real-time: ..."` first, then gets curated into
`EMILY/BACKLOG.md`, then worked. Every commit message ends with a blank line then `session: <tag>`.

## README Reality — SAGA reconciliation (standing instruction, monorepo-wide)

If a change substantially changes this project's core README's claims (a capability ships, status
moves from planned to working, install/run steps change), update `README.md` in the same unit of
work. See root `CLAUDE.md` for the full protocol.
