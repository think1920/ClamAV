CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall -Wextra
LDFLAGS = -pthread

all: ftp_client clamav_agent

ftp_client: ftp_client.cpp
	$(CXX) $(CXXFLAGS) -o ftp_client ftp_client.cpp $(LDFLAGS)

clamav_agent: clamav_agent.cpp
	$(CXX) $(CXXFLAGS) -o clamav_agent clamav_agent.cpp $(LDFLAGS)

clean:
	rm -f ftp_client clamav_agent

.PHONY: all clean
