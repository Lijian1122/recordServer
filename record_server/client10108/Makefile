# PROG = simplest_web_server
# MODULE_CFLAGS=-DMG_DISABLE_DAV_AUTH -DMG_ENABLE_FAKE_DAVLOCK
# include ../examples.mk

all: client

client: client.c mongoose.c mongoose.h
	g++ -W -Wall -std=c++11 -g -o client client.c  mongoose.c -lpthread -ldl

clean:
	rm client

