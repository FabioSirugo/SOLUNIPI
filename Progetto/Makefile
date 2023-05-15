#FABIO SIRUGO
#MAT: 533099
#CORSO B
#MAKEFILE PER OS_STORE



POSFLAGS=-I. -D_POSIX_C_SOURCE=200809L
UTILSC=utils_client
CFLAGS=-std=c99 -Wall -g
CC=gcc

.PHONY: all clean test_base bigtest someErrors nometroppolungo

all : server client 

server: server.c
	$(CC) $(CFLAGS) $(POSFLAGS) -O3 $< -o $@ -lpthread 
	@echo "server compilato"
lib$(UTILSC).a: $(UTILSC).o $(UTILSC).h
	$(AR) rcs $@ $<
client: client.c lib$(UTILSC).a
	$(CC) $(CFLAGS) -L. $(POSFLAGS) -O3 $< -o $@ -l$(UTILSC)
	@echo "Client compilato"
clean :
	rm -f client client.o server.o server lib$(UTILSC).a lib$(UTILSC).o utils_client.o objstore.sock testout.log
	rm -rf data/
cleanData :
	rm -rf data/
cleanLog : 
	rm -f testout.log

test:
	make clean
	make
	make test_base

test_base:
	./server &
	@./testscr.sh
	@./testsum.sh

bigtest:
	./server &
	@./bigtest.sh
	@./testsum.sh

someErrors:
	./server &
	@./allwrong.sh
	@./testsum.sh

nometroppolungo:
	./server &
	@./nometroppolungo.sh
	@./testsum.sh