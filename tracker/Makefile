# HRIP tracker: PARENA logic -> C -> native binary. PARENA = path to a built PARENA checkout.
PARENA ?= ../../PARENA
OUT    ?= build
CC     ?= gcc
CFLAGS ?= -std=c99 -DPARENA_NO_GRAPHICS -Wall -Wextra -Werror -Wframe-larger-than=256 -fstack-protector-strong
SRC     = power_log.prn game_state.prn
LIBS    = $(PARENA)/stdlib/string.prn

$(OUT)/tracker_gen.c: $(SRC)
	mkdir -p $(OUT)
	$(PARENA)/parena build $(LIBS) $(SRC) -o $@

tracker: $(OUT)/tracker_gen.c host.c
	$(CC) $(CFLAGS) -I $(PARENA)/runtime -I $(OUT) host.c $(PARENA)/runtime/parena_runtime.c -o $(OUT)/tracker -lm

test: $(OUT)/tracker_gen.c
	cp $(OUT)/tracker_gen.c $(OUT)/pl_gen.c; cp $(OUT)/tracker_gen.c $(OUT)/gs_gen.c
	$(CC) -std=c99 -DPARENA_NO_GRAPHICS -Wall -Wextra -I $(PARENA)/runtime -I $(OUT) tests/test_power_log.c $(PARENA)/runtime/parena_runtime.c -o $(OUT)/t_pl -lm && $(OUT)/t_pl
	$(CC) -std=c99 -DPARENA_NO_GRAPHICS -Wall -Wextra -I $(PARENA)/runtime -I $(OUT) tests/test_game_state.c $(PARENA)/runtime/parena_runtime.c -o $(OUT)/t_gs -lm && $(OUT)/t_gs

.PHONY: tracker test
