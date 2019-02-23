TARGETS=ringmaster player

all: $(TARGETS)
clean:
	rm -f $(TARGETS)

client: client.cpp
	g++ -g -Wall -std=gnu++98 -o $@ $<

server: server.cpp
	g++ -g -Wall -std=gnu++98 -o $@ $<

