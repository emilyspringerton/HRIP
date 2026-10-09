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
chk "revealed card w/ owner+zone" "$out" '"id":28,"player":1,"card":"CORE_WON_141","zone0":"DECK"'
chk "no raw CR in JSON"           "$(printf '%s' "$out" | tr -d '\n' | grep -c "$(printf '\r')")" '^0$'
exit $fail
