LIBS = -lsporth -lsoundpipe -lsndfile -lm -ldl -ljack -lrunt
CFLAGS = -fPIC -O3 -shared -Wall -ansi -g

OBJ = stream.o

plumber.so: plumber.c $(OBJ)
	$(CC) $(CFLAGS) plumber.c $(OBJ) -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: plumber.so
	mkdir -p /usr/local/share/runt/
	install plumber.so /usr/local/share/runt/
	install sporth.rnt /usr/local/share/runt/

clean: 
	rm -rf plumber.so $(OBJ)
