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

exit $fail
