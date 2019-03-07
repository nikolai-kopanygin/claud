IDIR := ../../include
CC := gcc
CFLAGS := -I$(IDIR) -I/usr/local/include -I/usr/include -ggdb -fPIC
#CFLAGS = -fPIC -Wall -Wextra -O2 -g
LDFLAGS = -shared  # linking flags
RM = rm -f  # rm command

ODIR := ../../build
LDIR := ../../lib
LIBNAME := libclaud.so
TARGET_LIB := $(LDIR)/$(LIBNAME)

ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif

_DEPS = types.h utils.h cld.h http_api.h jsmn.h jsmn_utils.h
DEPS = $(patsubst %,$(IDIR)/claud/%,$(_DEPS))

_OBJ = utils.o cld_commands.o cld_list.o cld_get.o cld_share.o cld_upload.o \
cld.o cld_get_shard_info.o jsmn.o jsmn_utils.o http_api.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: all

all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJ)
	mkdir -p $(LDIR)
	$(CC) ${LDFLAGS} -o $@ $^

.PHONY: clean
clean:
	rm -f $(ODIR)/*.o *~ core $(IDIR)/*~

install: $(TARGET_LIB)
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(TARGET_LIB) $(DESTDIR)$(PREFIX)/lib/
	ldconfig
	install -d $(DESTDIR)$(PREFIX)/include/claud/
	install -m 644 $(IDIR)/claud/types.h $(DESTDIR)$(PREFIX)/include/claud/
	install -m 644 $(IDIR)/claud/utils.h $(DESTDIR)$(PREFIX)/include/claud/
	install -m 644 $(IDIR)/claud/cld.h $(DESTDIR)$(PREFIX)/include/claud/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/$(LIBNAME)
	rm -rf $(DESTDIR)$(PREFIX)/include/claud/
