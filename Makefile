CFLAGS=-O3 -g -Werror -Wall -pedantic
NAME=libsimple-sparsehash.so
TESTNAME=sparsehash_test
OBJS=simple_sparsehash.o
INCLUDES=-I./include/
LIBINCLUDES=-L.

all: $(NAME) test

clean:
	rm *.o
	rm $(TESTNAME)
	rm $(NAME)

test: test.o $(NAME)
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBINCLUDES) -o $(TESTNAME) $< -lsimple-sparsehash

%.o: ./src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

$(NAME): $(OBJS)
	$(CC) -shared -fPIC $(CFLAGS) $(INCLUDES) -o $(NAME) $^
