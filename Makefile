CFLAGS=-std=c++20
LDFLAGS=-lpthread -lpqxx -lcpr -lcrypto

all: main user_auth perm_control utils
	g++ $(CFLAGS) $(LDFLAGS) -o auth_server main.o endpoints/user_auth.o endpoints/perm_control.o utils.o

bin:
	g++ $(CFLAGS) $(LDFLAGS) -o auth_server main.o endpoints/user_auth.o endpoints/perm_control.o utils.o

main: main.cpp
	g++ $(CFLAGS) -c main.cpp

user_auth: endpoints/user_auth.cpp
	g++ $(CFLAGS) -c -I. -o endpoints/user_auth.o endpoints/user_auth.cpp

perm_control: endpoints/perm_control.cpp
	g++ $(CFLAGS) -c -I. -o endpoints/perm_control.o endpoints/perm_control.cpp

utils: utils.cpp
	g++ $(CFLAGS) -c utils.cpp

run: all
	./auth_server
