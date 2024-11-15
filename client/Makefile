# main:
# 	g++ -std=c++11 -g client.cpp logger.cpp server.cpp utility.cpp sha.cpp download.cpp make_request.cpp -lssl -lcrypto


CXX = g++
CXXFLAGS = -std=c++11 -g
LIBS = -lssl -lcrypto
SOURCES = client.cpp logger.cpp server.cpp utility.cpp sha.cpp download.cpp make_request.cpp
EXECUTABLE = client

all: $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) $(LIBS)

clean:
	rm -f $(EXECUTABLE)
