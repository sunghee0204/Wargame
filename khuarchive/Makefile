CXX ?= g++
CXXFLAGS ?= -fsanitize=address -o

all: 
	$(CXX) $(CXXFLAGS) KhuArchive main.cpp

clean:
	rm -f KhuArchive Archive.kzip
	
.PHONY: all clean submit
