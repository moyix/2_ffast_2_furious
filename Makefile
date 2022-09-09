CC ?= gcc
CFLAGS = -g -fsanitize=address
LIBS = -lm -ldl

all: gofast.so 2_ffast_2_furious

gofast.so: gofast.c
	$(CC) -shared -fPIC -ffast-math -o gofast.so gofast.c

gofast.c:
	touch gofast.c

2_ffast_2_furious: 2_ffast_2_furious.c
	$(CC) -o 2_ffast_2_furious 2_ffast_2_furious.c $(CFLAGS) $(LIBS)

clean:
	rm -f gofast.so gofast.c 2_ffast_2_furious
