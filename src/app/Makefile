IDIR := ../../include
CC := gcc
CFLAGS := -I$(IDIR) -I/usr/include -ggdb

ODIR := ../../build
LDIR := ../../lib
BDIR := ../../bin
APPNAME := claud
TARGET := $(BDIR)/$(APPNAME)
LIBS :=-lm -lcurl -L$(LDIR) -lclaud

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

_DEPS = types.h utils.h command.h mail_ru_cloud.h http_api.h jsmn.h jsmn_utils.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o command.o command_handlers.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

all: ${TARGET}

$(TARGET): $(OBJ)
	mkdir -p $(BDIR)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(IDIR)/*~

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/
	
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(APPNAME)
