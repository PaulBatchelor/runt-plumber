LIBS = -lsporth -lsoundpipe -lsndfile -lm -ldl -ljack -lrunt
CFLAGS = -fPIC -O3 -shared -Wall -ansi -g

OBJ=audio.o stream.o

plumber.so: plumber.c $(OBJ)
	$(CC) $(CFLAGS) plumber.c $(OBJ) -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean: 
	rm -rf plumber.so $(OBJ)
