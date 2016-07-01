## Copyright 2016 University of Stuttgart
## Authors: Dennis Hoppe

CC = /usr/bin/gcc

COPT_SO = $(CFLAGS) -fpic

CFLAGS = -std=gnu99 -pedantic -Wall -fPIC -Wwrite-strings -Wpointer-arith \
-Wcast-align -O0 -ggdb $(CURL_INC) $(API_INC)

LFLAGS =  -lm $(CURL)

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CFLAGS += -DDEBUG -g
else
    CFLAGS += -DNDEBUG
endif

COMMON = .
EXTERN = $(COMMON)/bin

EXT_SRC = $(COMMON)/ext
CUTEST = $(EXT_SRC)/CuTest
CUTEST_INC = -I$(CUTEST)

SRC = $(COMMON)/src
API_INC = -I$(SRC)/
CONTRIB_SRC = $(SRC)/contrib
TEST_SRC = $(COMMON)/test

CURL = -L$(EXTERN)/curl/lib/ -lcurl
CURL_INC = -I$(EXTERN)/curl/include/

all: clean mf_api test_mf_api

mf_api: $(SRC)/mf_api.c $(CONTRIB_SRC)/mf_publisher.c
	$(CC) -shared $^ -o $@.so -lrt -ldl -Wl,--export-dynamic $(CFLAGS) $(LFLAGS)

test_mf_api: $(TEST_SRC)/test_mf_api.c $(SRC)/mf_api.c $(CONTRIB_SRC)/mf_publisher.c
	$(CC) $^ -o $@ $(CUTEST)/*.c $(CUTEST_INC) $(API_INC) -I. $(CFLAGS) $(LFLAGS)

install:
	@mkdir -p lib/
	mv -f mf_api.so lib/

clean:
	rm -rf *.o
	rm -rf *.so
	rm -rf test_mf_api
	rm -rf lib
	rm -rf html
	rm -rf latex
	rm -rf doc

clean-all: clean
	rm -rf bin

doc:
	doxygen Doxyfile
