EXECUTABLE_SERVER	:= tcp_server
EXECUTABLE_CLIENT := tcp_client

SERVER_UTIL := server_util
CLIENT_UTIL := client_util

SERVER_OBJECTS := $(EXECUTABLE_SERVER).o $(SERVER_UTIL).o
CLIENT_OBJECTS := $(EXECUTABLE_CLIENT).o $(CLIENT_UTIL).o

main: $(SERVER_OBJECTS) $(CLIENT_OBJECTS)
	g++ -g -Wall -std=c++17 -o $(EXECUTABLE_SERVER) $(SERVER_OBJECTS)
	g++ -g -Wall -std=c++17 -o $(EXECUTABLE_CLIENT) $(CLIENT_OBJECTS)

$(EXECUTABLE_SERVER).o: $(SERVER_UTIL).h 

$(EXECUTABLE_CLIENT).o: $(CLIENT_UTIL).h

clean:
	-rm $(SERVER_OBJECTS) $(EXECUTABLE_SERVER)
	-rm $(CLIENT_OBJECTS) $(EXECUTABLE_CLIENT)