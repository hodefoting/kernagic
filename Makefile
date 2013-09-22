BIN_PKGMODULES=cairo gtk+-2.0
PROJECT_NAME = kernagic

CFLAGS += -O2 -g
LD_FLAGS += -lm

include .mm/magic
include .mm/bin
