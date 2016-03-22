GIT_HASH = $(shell git describe --always --tags)

bin_PROGRAMS = battor

battor_SOURCES = battor.c params.h params.c uart.h uart.c samples.c samples.h control.c control.h

battor_CFLAGS = -DGIT_HASH=\"$(GIT_HASH)\"
