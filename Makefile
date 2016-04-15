VERSION=0.1
SOVERSION=0
CFLAGS=-std=c99 -Wextra -Wno-ignored-qualifiers -O3 -g -Werror -Wall
NAME=libsimple-sparsehash.so
TESTNAME=sparsehash_test
OBJS=simple_sparsehash.o
INCLUDES=-I./include/
LIBINCLUDES=-L.

PREFIX?=/usr/local
INSTALL_LIB=$(PREFIX)/lib/
INSTALL_INCLUDE=$(PREFIX)/include/

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
LDCONFIG=
ifeq ($(uname_S),Darwin)
	LDCONFIG=echo
else
	LDCONFIG=ldconfig
endif

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

uninstall:
	rm -rf $(INSTALL_LIB)$(NAME)*
	rm -rf $(INSTALL_INCLUDE)/simple_sparsehash.h

install:
	@mkdir -p $(INSTALL_LIB)
	@mkdir -p $(INSTALL_INCLUDE)
	@install $(NAME) $(INSTALL_LIB)$(NAME).$(VERSION)
	@ln -fs $(INSTALL_LIB)$(NAME).$(VERSION) $(INSTALL_LIB)$(NAME)
	@ln -fs $(INSTALL_LIB)$(NAME).$(VERSION) $(INSTALL_LIB)$(NAME).$(SOVERSION)
	@install ./include/*.h $(INSTALL_INCLUDE)
	@$(LDCONFIG) $(INSTALL_LIB)
	@echo "$(NAME) installed to $(PREFIX) :^)."
