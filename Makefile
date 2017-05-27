LIBS = -lsporth -lsoundpipe -lsndfile -lm -ldl -ljack -lrunt -lpthread
CFLAGS = -fPIC -O3 -Wall -ansi -g

# Local Runt
LIBS += -L$(HOME)/.runt/lib
CFLAGS += -I$(HOME)/.runt/include

OBJ = stream.o plumber.o

NAME = plumber

default: librunt_$(NAME).a rntplumber

librunt_$(NAME).a: $(OBJ)
	$(AR) rcs $@ $(OBJ)

rntplumber: parse.c $(OBJ)
	$(CC) $(CFLAGS) $< $(OBJ) -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

ugen.so: ugen.c $(OBJ)
	$(CC) $(CFLAGS) -fPIC -shared ugen.c $(OBJ) -o $@ $(LIBS) 

install: librunt_$(NAME).a rntplumber
	mkdir -p ~/.runt/lib
	mkdir -p ~/.runt/bin
	mkdir -p ~/.runt/include
	cp librunt_$(NAME).a ~/.runt/lib
	cp runt_plumber.h ~/.runt/include
	cp rntplumber ~/.runt/bin

clean: 
	rm -rf librunt_$(NAME).a $(OBJ) 
	rm -rf rntplumber 
