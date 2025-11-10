VERSION  = $(shell git describe --tags --always)

PREFIX   = /usr/local

CC       = gcc
LD       = $(CC)

CFLAGS    = -std=c89 -pedantic -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700 -D_FILE_OFFSET_BITS=64
CFLAGS   += -O3 -ffast-math
CFLAGS   += -Wall -Wextra -Wpedantic -Werror
CFLAGS   += -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-missing-field-initializers

CPPFLAGS  = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700 \
           -D_FILE_OFFSET_BITS=64 -DVERSION="$(VERSION)"
LDFLAGS   = -s
