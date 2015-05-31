CFLAGS=-std=c99 -O3 -g -Werror -Wall -pedantic
NAME=libsimple-sparsehash.so
TESTNAME=sparsehash_test
OBJS=simple_sparsehash.o
INCLUDES=-I./include/
LIBINCLUDES=-L.

all: $(NAME) $(TESTNAME)

clean:
	rm *.o
	rm $(TESTNAME)
	rm $(NAME)

$(TESTNAME): test.o $(NAME)
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBINCLUDES) -o $(TESTNAME) $< -lsimple-sparsehash

%.o: ./src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -fPIC -c $<

$(NAME): $(OBJS)
	$(CC) -shared -fPIC $(CFLAGS) $(INCLUDES) -o $(NAME) $^
