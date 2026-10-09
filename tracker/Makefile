# HRIP tracker: PARENA logic -> C -> native binary. PARENA = path to a built PARENA checkout.
PARENA ?= ../../PARENA
OUT    ?= build
CC     ?= gcc
CFLAGS ?= -std=c99 -DPARENA_NO_GRAPHICS -Wall -Wextra -Werror -Wframe-larger-than=512 -fstack-protector-strong
SRC     = power_log.prn game_state.prn
LIBS    = $(PARENA)/stdlib/string.prn

$(OUT)/tracker_gen.c: $(SRC)
	mkdir -p $(OUT)
	$(PARENA)/parena build $(LIBS) $(SRC) -o $@

tracker: $(OUT)/tracker_gen.c host.c
	$(CC) $(CFLAGS) -I $(PARENA)/runtime -I $(OUT) host.c $(PARENA)/runtime/parena_runtime.c -o $(OUT)/tracker -lm

test: tracker
	sh tests/e2e.sh $(OUT)/tracker

.PHONY: tracker test
