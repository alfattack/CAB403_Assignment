
all: serv cli

server_target = server_program
client_target = client_program

flags = -Wall -Werror
libs = -lpthread

serv:
	gcc $(wildcard server/*.c) coms/coms_wrapper.c  -o $(server_target) -Wall -Werror -lpthread

cli:
	gcc $(wildcard client/*.c) coms/coms_wrapper.c -o $(client_target) -Wall -Werror -lpthread


clean:
	rm $(server_target)
	rm $(client_target)

