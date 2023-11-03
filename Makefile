all: build

build:
	g++ main.cpp -std=c++20 -Iinclude -lpthread -lpqxx -lcpr -o auth_server

run: build
	./auth_server
