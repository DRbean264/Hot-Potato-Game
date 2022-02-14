# TARGETS=client server

# all: $(TARGETS)
# clean:
# 	rm -f $(TARGETS)

# client: client.cpp
# 	g++ -g -o $@ $<

# server: server.cpp
# 	g++ -g -o $@ $<

TARGETS=ringmaster player

all: $(TARGETS)
clean:
	rm -f $(TARGETS) *~ *.o

ringmaster: ringmaster.o utils.o
	gcc -g -o ringmaster ringmaster.o utils.o

player: player.o utils.o
	gcc -g -o player player.o utils.o

ringmaster.o: ringmaster.c utils.h
	gcc -g -c ringmaster.c

player.o: player.c utils.h
	gcc -g -c player.c

utils.o: utils.c utils.h
	gcc -g -c utils.c