all: onaboat

onaboat:
	gcc -Wall -g -O2 -o onaboat onaboat.c -lbcm2835 

clean:
	rm -rf onaboat
