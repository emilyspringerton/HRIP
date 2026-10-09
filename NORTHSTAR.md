# HRIP — Hearthstone Rip — NORTHSTAR (scoping pass, 2026-10-09)

Founder real-time (routed via `emily observe`, Apple #22222): a new repo, **HRIP**, whose job is to
rip Hearthstone's own card data out of a real local Windows install — "from scratch," not from a
third-party database — and produce JSON that gets uploaded into WOTAN's Hearthstone section
(`WOTAN/HEARTHSTONE_NORTHSTAR.md` §4, `IDUNA/migrations/truestore/202610090001_hs_social.sql`'s own
`hs_cards` table, created there **intentionally empty**, card data named as the founder's explicit
"last step"). **This repo is that last step.**

Founder's own starting material was explicitly "advice from Google... probably half bonkers but you
will get the idea" — per THE_EMILY_WAY Principle 19, this doc investigates what's real before
committing to an approach, same discipline as DEADWEIGHT/LO/BIG_O's own northstars. Two real
founder clarifications landed mid-scoping and are binding:

1. **Self-extraction is the product, not a shortcut.** Community pre-parsed card dumps (e.g.
   HearthstoneJSON on GitHub) may be used **only as a verification/feedback-loop reference** while
   building HRIP's own extractor — diffing our output against theirs to catch bugs — never as the
   thing that actually ships data into WOTAN. Falling back to a third-party source as the real data
   path happens **only if self-extraction utterly fails**, and that's a founder-level call to make
   later, not a default to reach for early.
2. **"Pure PARENA, batteries included."** The app ships as a single Windows exe + `run.bat`, no
   external runtime the founder has to install, no command line required to operate it.

## 1. Where the data actually lives (real, checked)

Hearthstone does not expose a SQLite/MySQL file. Card data and definitions are packed inside Unity
**AssetBundle** files under the install's `Hearthstone\Data\Win\` directory (historically a bundle
such as `cardxml0.unity3d`, containing a `CardDefs.xml`-shaped payload, plus separate texture/asset
bundles for card art). This is genuinely binary, versioned, and sometimes-compressed (Unity bundles
commonly use LZMA or **LZ4** block compression depending on build — convenient overlap with this
monorepo's own `PARENA/stdlib/compress/lz4.prn` default). **No prior Hearthstone-asset-format
research exists anywhere in this monorepo** — the only existing Hearthstone code,
`IDUNA/internal/hsdeck/hsdeck.go`, is a deckstring (base64 varint) codec with no card database and
no asset parsing. HRIP starts from zero on the format itself.

## 2. Checked PARENA capability audit — what's real vs. aspirational

Investigated directly against `PARENA/STDLIB.md` and `PARENA/stdlib/` (not assumed):

| Need | Status |
|---|---|
| File I/O (scan Program Files, read/write files) | **Real.** `stdlib/io.prn` — `file-open/close`, `path-exists?`, `list-dir`, `is-dir?`, `read-string`, `write-string`, `read-line`. Syscall-backed via `#target` C. Covers the Windows filesystem walk. |
| `defstruct` / tagged data (a `Card` record) | **Real.** Confirmed in `STDLIB.md` and used throughout the stdlib (e.g. `NDArray`, `ParseError`). |
| `map` (dbf_id → Card index) | **Real source exists** (`stdlib/map.prn`, `mapbuilder`), open-addressing hash table. |
| `csv` | **Real source exists** (`stdlib/csv.prn`, `read-csv`). Useful for the verification feedback loop (diffing against a community CSV/JSON export), not a reason to avoid parsing the real binary format. |
| JSON encode (final output) | **Real but limited.** `stdlib/json.prn` exists; its own header notes `JObject` avoids `Map` because generic `K`/`V` fail the current emitter — uses parallel `Vec`s instead. Source exists; **not confirmed executing end-to-end** by prior work. |
| LZ4 decompress (bundle decompression) | **Real source exists**, `stdlib/compress/lz4.prn` — matches the monorepo's standing LZ4-default convention. Pure-PARENA-scaffolded, not FFI-bound to reference liblz4; **not confirmed executing end-to-end**. |
| GUI (the upsert/log-streaming page) | **Real, but FFI, not native.** `sdl2` package — live-verified window/renderer/input/text (SDL2_ttf) via FFI to real SDL2. Founder's own stated long-term goal is a from-scratch reimplementation; today it's a wrapper, which is an accepted, named exception (same shape as any other FFI stdlib package), not a gap to fix first. |
| Windows build target | **Real, via cross-compile.** PARENA emits C; `PARENA/NORTHSTAR.md` repeatedly cites mingw cross-compiling that C to a Windows binary, verified clean elsewhere in this monorepo. There is no native Windows PARENA emitter — this is the existing, working path, not a new one to build. |
| **The real, named risk** | `STDLIB.md` states plainly that most of the ~57 stdlib packages (including several named above) have **parseable source but no confirmed execution** — the current emitter "only understands one program shape" in places. `IDUNA/internal/hsdeck/hsdeck.go` is plain Go specifically *because* "BURROW's Go emitter is scalar-only (no loops/Vec)" for a varint codec — a materially *simpler* parsing problem than a Unity AssetBundle + LZ4 + XML pipeline. **Expect this program to be the thing that finds the emitter's real limits**, not a program that fits neatly inside them. |

Per IDUNA/`CLAUDE.md`'s standing **Core Deps Are PARENA-First** rule: AssetBundle parsing, LZ4
decompression, and XML parsing are core deps this app stands on. If PARENA's compiler/emitter can't
actually execute the stdlib source that claims to cover them, **fixing that gap is itself the first
real task**, not a reason to reach for a third-party library as the shipped path. This is also why
phase 1 below is a PARENA-only spike, not a full-feature build.

## 3. What does NOT exist yet (named honestly)

- No Unity AssetBundle reader, anywhere, in any language, in this monorepo.
- No Hearthstone XML (`CardDefs.xml`-shaped) parser.
- No Windows crash-handler precedent (`SetUnhandledExceptionFilter`) in this monorepo — the real,
  checked precedent is POSIX-only: `REDGARDEN/apps/arena_server/src/main.c:924-974`, a `signal()`-
  registered handler that writes a JSON line to an already-open log file handle on SIGSEGV/SIGABRT/
  SIGFPE, then re-raises with `SIG_DFL`. Same idea, Windows API, new code.
- No "log streaming with projection" pattern exists anywhere to copy (checked `IDUNA/internal/
  http/handlers/logs.go` — Splunk-HEC-shaped ingest/search over NDJSON, not a live-tail/projection
  UI concept). This gets invented fresh for HRIP: a live tail of the on-disk log, with a "projection"
  view that parses structured log lines into a filtered/columned summary (e.g. "cards found so far",
  "bundle currently being read", "errors"), not just a raw scrolling pane.
- No generic admin upsert endpoint exists on the WOTAN/IDUNA side for `hs_cards` — HRIP's own output
  needs a real new IDUNA route to land in, which is Go work, not PARENA (IDUNA itself is a Go
  backend; this is consistent with `hsdeck.go` already being Go for the same reason).
- No `run.bat`-wrapped release zip precedent exists anywhere in this monorepo (SHANKPIT bundles
  `ffmpeg.exe` alongside its game exe, which is the closest shape, but ships no `.bat`). Novel, but
  small: a `run.bat` that execs the exe, doesn't close the window on a nonzero exit, and prints the
  log file path, so a non-CLI user can read a failure.

## 4. V0 cut — what actually gets built first

Mirroring DEADWEIGHT/LO's own "name the full ask, cut a real buildable V0" discipline:

- **V0 goal: prove the pipeline on one real bundle, text fields only, no art.** Locate install →
  read one AssetBundle → decompress → extract the card-defs payload → parse enough of it to get
  `dbf_id`, `name`, `cost`, `class`, `rarity`, `card_set`, `text` → emit JSON shaped exactly like
  `hs_cards` (see §1 schema below) → upsert into IDUNA.
- **Deferred, named, not dropped**: card art (`image_url` — a separate texture-bundle extraction,
  matches WOTAN's own "art is the last step" framing, so it's last here too), incremental/patch-diff
  re-extraction, a packaged installer beyond zip+run.bat, any auto-detection of game patches.

### Target output shape (matches `hs_cards` exactly — IDUNA/migrations/truestore/202610090001_hs_social.sql)
```json
{"dbf_id": 123, "name": "...", "cost": 0, "class": "...", "rarity": "...", "card_set": "...", "text": "...", "image_url": ""}
```

## 5. Phased plan → kanban (SECTION 600 in `EMILY/BACKLOG.md`)

1. **HRIP-01 — RE spike: Hearthstone's real on-disk bundle format.** Throwaway, any-tooling
   investigation (not shipped code) to nail down: which file(s) under `Hearthstone\Data\Win\` hold
   card defs, compression scheme actually in use, exact payload shape (XML vs. a Unity serialized
   object). Output is a written format doc, not a build.
2. **HRIP-02 — PARENA core dep: AssetBundle reader + decompression.** Extend/prove
   `stdlib/compress/lz4.prn` (or the real decompression scheme HRIP-01 finds) actually executes
   end-to-end against a real captured bundle header; write the bundle-container parser. This is the
   step most likely to hit real PARENA emitter limits per §2 — if it does, the gap becomes a PARENA
   compiler/stdlib fix, logged as its own sub-item, not routed around.
3. **HRIP-03 — PARENA core dep: card-defs parser.** Parse the extracted payload into `(defstruct
   Card ...)` records, indexed in a `map` by `dbf_id`.
4. **HRIP-04 — JSON emission matching `hs_cards`.** Prove `stdlib/json.prn` executing end-to-end for
   this shape; write the output file via `stdlib/io.prn`.
5. **HRIP-05 — Verification feedback loop.** Diff HRIP's own JSON output against a community
   reference (e.g. HearthstoneJSON) per the founder's explicit "verification only" framing — a report
   of mismatches, not a merge of the two data sources.
6. **HRIP-06 — Windows install discovery.** Standard install-path probing (Program Files, Program
   Files (x86), registry uninstall-key lookup as a fallback) via `stdlib/io.prn`.
7. **HRIP-07 — Crash/error file logging.** Port REDGARDEN's signal-handler-writes-JSON-line idiom to
   the Windows API (`SetUnhandledExceptionFilter` via FFI); always-on log file next to the exe.
8. **HRIP-08 — SDL2 GUI: scan/extract/progress screen.** Batteries-included main window: a button to
   locate + extract, visible progress, the on-disk log path shown on screen (so a crash is always
   diagnosable without a terminal).
9. **HRIP-09 — Upsert page + log streaming with projection.** A second screen: review extracted cards,
   push to a new IDUNA endpoint; a live tail of the log file with a parsed/projected summary view
   (counts, current bundle, errors) alongside the raw tail.
10. **HRIP-10 — IDUNA `hs_cards` upsert endpoint (Go, IDUNA repo, not HRIP).** New route
    (`POST /api/v1/hs/cards/upsert` or similar), gated the same way other admin-ish WOTAN data entry
    points are; this is IDUNA-side work HRIP depends on, tracked as its own card so it isn't silently
    assumed.
11. **HRIP-11 — CI/CD auto-release.** Mirror `SHANKPIT/.github/workflows/release.yml`'s shape
    (tag-triggered, build, zip, `gh release create`): cross-compile via mingw, package `hrip.exe` +
    `run.bat` + the (empty until first run) log directory into a zip, auto-tag and publish a GitHub
    Release the same way SHANKPIT/GOLDEN_DOCS already do.

## 6. Open question for the founder (named, not guessed at)

If HRIP-02 (PARENA core-dep spike) finds the emitter genuinely cannot execute a program this shape
yet, the real choice is: (a) invest in fixing BURROW/`parena-c`'s emitter to cover it (the
"PARENA-first" policy's own intended outcome), or (b) accept a narrower V0 (e.g. a hand-written C
host with PARENA-native rule/parsing *modules* FFI'd in, the same "C host + PARENA mod islands"
shape this monorepo already uses for REDGARDEN/ECOWAR/PAPERCRAFT) rather than a 100%-PARENA binary.
Not resolved here — flagged so it doesn't get silently decided mid-implementation.
