# HRIP tracker — Hearthstone deck/game tracker (PARENA logic, thin C host)

Tails Hearthstone's `Power.log` and emits **one JSON record per finished game**: header, both
players' revealed cards (the substrate for opponent-deck inference and matchup stats), and an
ordered **replay timeline**. Logic is PARENA; C is only the file/poll/arena host.

```
make test PARENA=../PARENA      # build + end-to-end test on a real (scrubbed) Power.log excerpt
build/tracker <Power.log> [--once]
```

## Record shape
```
{"game_type":"GT_RANKED","format":"FT_STANDARD","player1":"…","player2":"…",
 "result1":1,"result2":2,"turns":25,"complete":1,"me":1,   # result: 1 won, 2 lost, 3 tied, 0 unknown; me = local player slot (0 undecidable)
 "entities":[{"id":28,"player":1,"card":"CORE_WON_141","zone0":"DECK","t":0}, …],
 "timeline":[["t",turn], ["z",turn,entity,player,card,zone], ["p",turn,entity,player,card], …]}
```
- `entities`: every card that was *revealed*, once, with its owner and the zone it first appeared in.
  `zone0` DECK/HAND = part of that player's deck; anything else = created by an effect. For the
  opponent this is the "cards seen" list = their deck as far as it was revealed.
- `timeline`: replay stream in log order (turn starts, zone changes, plays), each resolved to
  owner + card. Hearthstone has no local engine to re-run (unlike DEADWEIGHT's seed+inputs replay),
  so the log *is* the replay.

## Files
| File | Role |
|---|---|
| `power_log.prn` | scanning helpers (find, token, int, name) |
| `game_state.prn` | `defstruct Fact`/`GameState`, per-line handlers, `fold-text` loop, summary JSON |
| `host.c` | file offset, one static buffer, two native `Arena`s (scratch reset per fold, game reset per game), sleep loop. No stack buffers, no malloc |

## Validated against real logs
Two real `Power.log` files (build 253216, 5 complete games: ranked + friendly): plays, final turn
and results match an independent Python count on all 5. 82k-line / 10 MB log folds in ~0.7 s.
Real-log facts the parser handles: CRLF line endings; every event logged twice (only `GameState.*`
is read); opponent name `UNKNOWN HUMAN PLAYER` at game start (result assigned to the leftover
slot); abandoned games (next `CREATE_GAME` with no `STATE=COMPLETE`) emit `complete:2`.

## Not done yet
- Not yet validated for: Battlegrounds/Mercenaries/Arena formats, reconnects mid-game, `log.config`
  installer, Mac paths. A game still running at EOF is held, not emitted.
- Hero/class identification (hero entity → class) and deck-code reconstruction need the card DB
  (`hs_cards`) join, done server-side.
- WOTAN side: `hs_games` ingest + matchup/win-rate views + replay viewer (same pattern as the
  DEADWEIGHT match viewer). No native Windows build of this engine exists or is planned — the live
  path ships matched log lines to this same tracker running server-side instead (see "The WOTAN
  uplink" below); an in-game overlay UI is still untested/unbuilt.
- Generated PARENA code copies structs by value on the stack (largest frame ~370 B, build limit
  `-Wframe-larger-than=512`); the host itself keeps no stack buffers.

## `me` (local player)
Undrawn deck cards are never in `Power.log`, so `me` comes from what only the local client sees:
the **opening hand** (deck-origin cards revealed by turn 1; the opponent's hand stays hidden), with
known-id plays as the fallback. Correct on all 5 recorded games and on a live game (a pseudo-play,
`TIME_EVENT_*`, had fooled the old play-only signal). 0 when nothing is revealed yet.

## Where the deck comes from: `Decks.log` (not memory)
Hearthstone logs the queued deck itself to `Logs\Hearthstone_*\Decks.log` (enabled by the `[Decks]`
section already in `log.config`):

```
I 20:53:40 Finding Game With Deck:
I 20:53:40 ### A3VTR
I 20:53:40 # Deck ID: 3380012604
I 20:53:40 AAECAZICBpigBODAB4TEB4jdB7Hj...      <- standard deck code
```
Also, at login, every deck ("Deck Contents Received", name + id + deck code). So: decode the code
(IDUNA already has the decoder), map dbf ids to card ids via `hs_cards.card_id`, and
`cards left = deck - own deck-origin entities that left DECK`. Verified live on a game in progress.
(An earlier note here said the logs carry no decklist; that was wrong: `Power.log` doesn't, `Decks.log` does.)

## Hearthstone's 10 MB log cap (found 2026-10-10)
Hearthstone stops writing `Power.log` at ~10,000 KiB per session (about 2-3 games); after that every
log-based tracker silently freezes until the game is restarted. Three real sessions ended at
10,243,664 / 10,247,587 / 10,253,145 bytes. The fix is one line in `client.config` next to
`Hearthstone.exe`: `FileSizeLimit.Int=-1` (HDT and Firestone write the same key). Truncating the log
does not work (the game keeps its write offset and leaves a sparse file). The WOTAN uplink
(IDUNA `internal/http/handlers/hs_live.go`) applies this itself, reports it, and warns when the log is
near/at the cap; any other client built on this tracker must do the same.

## The WOTAN uplink — what it does on your PC

WOTAN's live tracker page does not run this repo's own `host.c`/PARENA parser on your machine —
there is no Windows build of it. Instead your PC runs a small PowerShell script (`hsUplinkScript`
in IDUNA's `internal/http/handlers/hs_live.go`, served at `GET /api/v1/hs/live/uplink.ps1`) that
tails your two Hearthstone log files and ships the matching lines to IDUNA, which runs *this*
tracker (the vendored copy at `IDUNA/internal/hstracker/src`) over them server-side, once per
upload. The script never executes this repo's code; it is a separate, from-scratch log shipper.
This section documents what that script actually does, end to end, so running it is an informed
choice rather than a leap of faith.

**Getting it.** The normal path is the one-time installer (`GET /api/v1/hs/live/install.ps1`),
which writes a tiny launcher to `%LOCALAPPDATA%\WOTAN\launch.ps1` plus a Desktop and Start Menu
shortcut, and changes nothing else on the system. Nothing is registered to auto-start — you
double-click the shortcut each time you want to play, and closing the window stops everything.
Every launch re-downloads the current uplink script fresh from WOTAN (falling back to the last
copy it saved if WOTAN is unreachable), so a server-side fix ships without reinstalling. That also
means every run executes whatever PowerShell WOTAN is currently serving at that URL — the same
trust extended to any other self-updating client. The script is plain text; it can be read at that
URL before trusting it.

**Signing in.** Unless an existing `-Token` is passed, the script opens the default browser to
IDUNA's own SSO page and starts a plain HTTP listener bound to `127.0.0.1` only (default port
51824 — not reachable from the network) just long enough to catch the token IDUNA's login
redirect hands back. The token it ends up holding is scoped to exactly one permission,
`hs.live.upload`, and expires in 12 hours: it can POST log lines to the caller's own tracker
session and nothing else — it cannot act as that IDUNA identity anywhere else.

**Upload key (optional, 2026-10-12, WOTAN identity federation).** Instead of the browser sign-in, the
uplink can use a long-lived **upload key** (`wt_...`, created under Account settings on WOTAN, shown once,
stored server-side only as a hash, scoped to `hs.live.upload`, revocable). At start it exchanges the key
(`POST /api/v1/hs/live/exchange {"key":...}` returning `{token, tags_linked}`) for the same 12-hour upload token
the browser flow produces, so the rest of the loop is unchanged; a 401 re-exchanges the key rather than opening the
browser. Parameters: `-Key '<key>'` (use it for this run), `-Key '<key>' -SaveKey` (also store it),
`-SaveKey` alone (prompts with a hidden input, so the key never lands in shell history, then stores it), and
`-ForgetKey` (delete the stored key). A stored key lives in `%APPDATA%\WOTAN\upload-key.dat`, encrypted with
Windows DPAPI (`ConvertFrom-SecureString`): only the same Windows user on the same PC can decrypt it. The Desktop
shortcut passes no key, so after a one-time `-SaveKey` it just works. A rejected key (revoked) falls back to the
browser sign-in with a message. Games are only attributed when the BattleTag in the log matches a BattleTag linked
to the account; others are counted per key as "rejected: BattleTag not linked" in Account settings. The script still
prints `Syncing to: ...` and never sends a club id (the server decides routing).

*Where the script lives:* the uplink and installer are served by IDUNA from string constants in
`IDUNA/internal/http/handlers/hs_live.go` (`hsUplinkScript`), not from this repo. The key-exchange change is kept
here as `uplink/uplink-key.patch` (unified diff against that file; `patch --dry-run -p0 hs_live.go < uplink-key.patch`
was clean when written). It is **not applied** and **PowerShell has not been run**: the change was checked by
structure only (balanced braces, expected parameters/URLs present, no backticks that would break the Go raw string).

**One file it edits: `client.config`.** Hearthstone stops writing `Power.log` at about 10 MB (2-3
games) unless `client.config` next to `Hearthstone.exe` has `FileSizeLimit.Int=-1` (the same key
HDT and Firestone set — see the cap section above). The script adds that one line if it is
missing (pass `-NoConfigFix` to disable this) and reports success/failure back to the WOTAN page.
If Program Files is not writable directly, Windows prompts for elevation (UAC) to copy in just
that one file; declining leaves tracking working, just capped at ~10 MB of log per session. It
never restarts or closes Hearthstone itself — if Hearthstone was already running when the fix
landed, the page reports that a restart is needed before the lifted cap takes effect.

**The loop, once a second.** It locates Hearthstone's `Logs` folder (standard install paths, or
`-LogsDir`), always follows whichever `Hearthstone_*` session folder is newest, and reads only the
bytes appended to `Decks.log`/`Power.log` since its last pass — opened with `FileShare.ReadWrite`
so it never locks or contends with Hearthstone's own write handle. It only ever reads those two
files; it does not touch chat logs, `Zone.log`, or anything else on disk. Lines that don't already
match Hearthstone's own structured log format (deck-code/"Deck ID" lines from `Decks.log`;
`GameState.*`/`PowerTaskList.*` DebugPrint lines from `Power.log`) are discarded before anything is
queued to send — there is no path for arbitrary file content or chat text to leave the PC through
this script.

**What actually leaves the PC.** The matched lines are LZ4-compressed in-process (a small
hand-rolled encoder, nothing beyond what is in this script) and POSTed, bearer-authenticated with
the upload-scoped token, to `<Base>/api/v1/hs/live/lines` — about once a second while a game is in
progress, or as a 10-second heartbeat otherwise. Concretely, this includes: both players'
BattleTags (Hearthstone logs both in plain text), every card either player reveals, zone changes
(draws/plays/deaths), turn numbers, and the final result. That is exactly what the live page needs
to render a deck tracker, and exactly what it costs to get one — nothing else about the PC is
collected or sent.

**If something goes wrong.** A batch stays queued until the server accepts it, so a dropped
connection or a 429 just retries — a game's lines are never silently lost. A 401 (expired or
revoked token) triggers a fresh sign-in automatically. If the server reports it lost its session
(a WOTAN deploy restarts its in-memory state), the script resends the current game from the start.

**Undoing it.** Closing the PowerShell window stops everything immediately. Deleting
`%LOCALAPPDATA%\WOTAN` and the two shortcuts removes everything the installer created. The one
change that outlives the window is the `FileSizeLimit.Int=-1` line in `client.config` — harmless
(and shared with other trackers) if left in place, or just delete that line to restore
Hearthstone's default cap.

**Not handled by this script.** Hearthstone must already be configured to write
`Power.log`/`Decks.log` at all (`log.config`'s `[Power]`/`[Decks]` sections — commonly already on
from a prior HDT/Firestone install, or set by hand). The uplink never creates or edits
`log.config`; see "Not done yet" above.
