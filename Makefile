LIBS = -lsporth -lsoundpipe -lsndfile -lm -ldl -ljack -lrunt
CFLAGS = -fPIC -O3 -shared -Wall -ansi -g

plumber.so: plumber.c audio.o
	$(CC) $(CFLAGS) plumber.c audio.o -o $@ $(LIBS)

audio.o: audio.c
	$(CC) $(CFLAGS) -c $< -o $@

clean: 
	rm -rf plumber.so audio.o
