CFLAGS=-std=c++20

all: user_auth utils
	g++ $(CFLAGS) -lpthread -lpqxx -lcpr -o auth_server main.cpp endpoints/user_auth.o utils.o

user_auth: endpoints/user_auth.cpp
	g++ $(CFLAGS) -c -I. -o endpoints/user_auth.o endpoints/user_auth.cpp

utils: utils.cpp
	g++ $(CFLAGS) -c utils.cpp

run: all
	./auth_server
