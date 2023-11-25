CFLAGS=-std=c++20
LDFLAGS=-lpthread -lpqxx -lcpr -lcrypto

all: main user_auth perm_control user_info admin utils config
	g++ $(CFLAGS) $(LDFLAGS) -o auth_server main.o endpoints/user_auth.o endpoints/perm_control.o endpoints/user_info.o endpoints/admin.o utils.o config.o

bin:
	g++ $(CFLAGS) $(LDFLAGS) -o auth_server main.o endpoints/user_auth.o endpoints/perm_control.o endpoints/user_info.o endpoints/admin.o utils.o config.o

main: main.cpp
	g++ $(CFLAGS) -c main.cpp

user_auth: endpoints/user_auth.cpp
	g++ $(CFLAGS) -c -I. -o endpoints/user_auth.o endpoints/user_auth.cpp

perm_control: endpoints/perm_control.cpp
	g++ $(CFLAGS) -c -I. -o endpoints/perm_control.o endpoints/perm_control.cpp

user_info: endpoints/user_info.cpp
	g++ $(CFLAGS) -c -I. -o endpoints/user_info.o endpoints/user_info.cpp

admin: endpoints/admin.cpp
	g++ $(CFLAGS) -c -I. -o endpoints/admin.o endpoints/admin.cpp

utils: utils.cpp
	g++ $(CFLAGS) -c utils.cpp

config: config.cpp
	g++ $(CFLAGS) -c config.cpp

run: all
	./auth_server

clean:
	rm -f *.o */*.o auth_server
