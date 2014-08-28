ODIR=./obj
BDIR=./bin
SRC=./src

CC = gcc
CFLAGS := -Wall -Wextra -g -std=c11

_HEADERS := sdl.h chip8.h
_OBJECTS := $(_HEADERS:.h=.o)
OBJECTS = $(patsubst %,$(ODIR)/%,$(_OBJECTS))
HEADERS = $(patsubst %,$(SRC)/%,$(_HEADERS))

ifdef COMSPEC
	LIBS := -lmingw32 -lSDL2main -lSDL2 -luser32 -lgdi32 -lwinmm -ldxguid
else
	LIBS := `sdl2-config --cflags --libs`
endif

#NODEBUG = -O2 -std=c11 -mwindows


default: chip8

build:
	@test -d $(ODIR) || mkdir $(ODIR)
	@test -d $(BDIR) || mkdir $(BDIR)

clean:
	rm -rf ../obj/
	rm -rf ../bin/
	rm -f ./*~
	rm -f ./*.swp

rebuild: clean default

chip8: ${OBJECTS}
	${CC} $^ $(SRC)/main.c $(LIBS) $(CFLAGS) -o $(BDIR)/$@

$(ODIR)/%.o: $(SRC)/%.c $(SRC)/%.h build
	${CC} $< -c $(CFLAGS) -o $@

.PHONY: default clean check dist distcheck install rebuild uninstall