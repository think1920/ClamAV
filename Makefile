CXX = g++
CXXFLAGS = -std=c++17 -Wall -static-libstdc++

SERVER_SRC = server.cpp
SERVER_EXE = server.exe

PKG_OPENCV = `pkg-config --cflags --libs opencv4`

all: $(SERVER_EXE)

$(SERVER_EXE): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lws2_32 -lgdi32 $(PKG_OPENCV) -liphlpapi
	clear

server: $(SERVER_EXE)

clr:
	rm -f $(SERVER_EXE) 
	clear

run: $(SERVER_EXE)
	./$(SERVER_EXE)
