LIBS = -lsporth -lsoundpipe -lsndfile -lm -ldl -ljack -lrunt
CFLAGS = -fPIC -O3 -shared -Wall -ansi -g

OBJ = stream.o plumber.o

NAME = plumber

default: librunt_$(NAME).a ugen.so

librunt_$(NAME).a: $(OBJ)
	$(AR) rcs $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

ugen.so: ugen.c $(OBJ)
	$(CC) $(CFLAGS) -fPIC -shared ugen.c $(OBJ) -o $@ $(LIBS) 

install: librunt_$(NAME).a
	mkdir -p ~/.runt/lib
	mkdir -p ~/.runt/include
	cp librunt_$(NAME).a ~/.runt/lib
	cp runt_plumber.h ~/.runt/include

clean: 
	rm -rf librunt_$(NAME).a $(OBJ) ugen.so
