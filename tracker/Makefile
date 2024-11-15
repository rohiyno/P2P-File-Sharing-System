# main:
# 	g++ -std=c++11 -g tracker.cpp utility.cpp handle_request.cpp logger.cpp
	

CXX = g++
CXXFLAGS = -std=c++11 -g
SOURCES = tracker.cpp tracker_utility.cpp handle_request.cpp logger.cpp
EXECUTABLE = tracker

all: $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES)

clean:
	rm -f $(EXECUTABLE)
