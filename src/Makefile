IDIR =../include
CC=gcc
CFLAGS=-I$(IDIR) -I/usr/include -ggdb

ODIR=obj
LDIR =../lib

LIBS=-lm -lcurl

_DEPS = types.h utils.h command.h mail_ru_cloud.h http_api.h jsmn.h jsmn_utils.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = claud.o utils.o command.o command_list.o command_get.o command_upload.o \
command_handlers.o client.o get_shard_info.o jsmn.o jsmn_utils.o http_api.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

claud: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
