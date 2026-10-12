#!/bin/sh
# end-to-end: real Power.log excerpt (CRLF, build 253216) + a tiny synthetic log -> tracker JSON.
BIN="$1"; DIR="$(dirname "$0")/fixtures"; fail=0
chk() { if printf '%s' "$2" | grep -q "$3"; then echo "PASS: $1"; else echo "FAIL: $1"; fail=1; fi; }
out="$("$BIN" "$DIR/real_excerpt.log" --once)"
chk "one game emitted"            "$(printf '%s\n' "$out" | wc -l)" '^1$'
chk "game type (CRLF trimmed)"    "$out" '"game_type":"GT_RANKED"'
chk "format"                      "$out" '"format":"FT_STANDARD"'
chk "local player name"           "$out" '"player1":"Hero#0001"'
chk "hidden opponent name"        "$out" '"player2":"UNKNOWN HUMAN PLAYER"'
chk "result won/lost (slot by hidden name)" "$out" '"result1":1,"result2":2'
chk "complete"                    "$out" '"complete":1'
chk "revealed card w/ owner+zone" "$out" '"id":28,"player":1,"card":"CORE_WON_141","card0":"CORE_WON_141","zone0":"DECK"'
chk "no raw CR in JSON"           "$(printf '%s' "$out" | tr -d '\n' | grep -c "$(printf '\r')")" '^0$'

# Regression (2026-10-12): a real Hearthstone build logs every CREATE_GAME twice (GameState, then an
# indented PowerTaskList duplicate of the same instant). This fixture is a verbatim capture of
# exactly that, still in progress (no STATE=COMPLETE) -- --live must emit ONE trailing record for
# the whole match, not fracture into two at the duplicate line, and the player's own opening-hand
# cards (declared before the duplicate) must still carry a real card id and zone0=DECK.
out2="$("$BIN" "$DIR/real_duplicate_create_game.log" --live)"
last2="$(printf '%s\n' "$out2" | tail -1)" # --live's trailing record is the only one the live page ever reads
chk "duplicate CREATE_GAME: one record emitted"   "$(printf '%s\n' "$out2" | wc -l)" '^1$'
chk "duplicate CREATE_GAME: not aborted"          "$last2" '"complete":0'
chk "duplicate CREATE_GAME: own card survives"    "$last2" '"id":21,"player":1,"card":"RLK_708","card0":"RLK_708","zone0":"DECK"'
chk "duplicate CREATE_GAME: own draw recorded"    "$last2" '\["z",1,21,1,"RLK_708","HAND"\]'

# Regression (2026-10-12): Hearthstone also logs STATE=COMPLETE twice (GameState, then an indented
# PowerTaskList duplicate preceded by its own short re-statement of a few end-of-game tags), the exact
# same shape as the CREATE_GAME duplicate above. host.c folds whatever text trails the real game's own
# end into a FRESH game state; before this fix that fresh state's STATE=COMPLETE (from the trailing
# duplicate) printed a second, near-empty phantom record, which the live page's `last record wins`
# read showed instead of the real result -- full deck, nothing drawn, "waiting for your opening hand."
# This fixture is a real, verbatim two-part excerpt of the same founder game as the fixture above (a
# later point in it): the first 900 lines are CREATE_GAME through the opening mulligan and real turn-1
# draws; the last 60 are the real end of that same game, both STATE=COMPLETE copies included. The
# ~15,000 lines of ordinary mid-game play between those two windows were cut only to keep the fixture
# small -- completion is driven by the STATE=COMPLETE lines, not turn count, so the cut does not affect
# whether this regression reproduces.
out3="$("$BIN" "$DIR/real_duplicate_state_complete.log" --live)"
chk "duplicate STATE=COMPLETE: one record emitted"  "$(printf '%s\n' "$out3" | wc -l)" '^1$'
chk "duplicate STATE=COMPLETE: real result kept"    "$out3" '"complete":1,"me":1'
chk "duplicate STATE=COMPLETE: own entities kept"   "$out3" '"player":1,"card":"'

exit $fail
