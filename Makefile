TARGET = server client
DEPS = message.h database.h
CFLAGS = -lpthread

all:  ${TARGET}

server: server.c ${DEPS}
	gcc -o server server.c ${CFLAGS}

client: client.c ${DEPS}
	gcc -o client client.c ${CFLAGS}

clean:
	rm -f ${TARGET}
