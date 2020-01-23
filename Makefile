EXECUTABLE	:= network-communication

$(EXECUTABLE):
	g++ -g -Wall -std=c++17 -o $(EXECUTABLE) $(EXECUTABLE).cpp 

clean:
	-rm $(EXECUTABLE)
