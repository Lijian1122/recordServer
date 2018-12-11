CC:= g++
LIBS:= -lmsgqueue -lcurl -lpthread -lglog -lcurClient 
CPPFLAGS:= -std=c++11 -g -Wall -D_REENTRANT

MONITOR = mergeMonitor
SERVER = mergeServer
BINDIR = /lib

TARGET = $(MONITOR) $(SERVER)

MOBJECTS :=mergeMonitor.o
OBJECTS :=MergeRunable.o CThreadPool.o mongoose.o webServer.o

all : $(TARGET)


# install:	$(TARGET)
# 	cp $(TARGET) $(BINDIR)

$(MONITOR): $(MOBJECTS)
	$(CC) -o $(MONITOR) $(MOBJECTS) $(LIBS)

$(SERVER): $(OBJECTS)
	$(CC) -o $(SERVER) $(OBJECTS) $(LIBS)

%.o:%.cpp   
	$(CC) -g3 -c $(CPPFLAGS) $(INCLUDE) $< -o $@

clean:
	rm -f $(OBJECTS) $(MOBJECTS) $(TARGET)

MergeRunable.o: MergeRunable.cpp  MergeRunable.h
CThreadPool.o: CThreadPool.cpp  CThreadPool.h
mongoose.o: mongoose.c mongoose.h
webServer.o: webServer.c
mergeMonitor.o: mergeMonitor.c
