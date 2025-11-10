VERSION  = $(shell git describe --tags --always)

PREFIX   = /usr/local

CC       = gcc
LD       = $(CC)

CFLAGS    = -std=c89 -pedantic -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700 -D_FILE_OFFSET_BITS=64
CFLAGS   += -DBF_VERSION="\"$(VERSION)\""
CFLAGS   += -O3 -ffast-math
CFLAGS   += -Wall -Wextra -Wpedantic -Werror
CFLAGS   += -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-missing-field-initializers
CFLAGS   += -s
