PRJNAME = lltool
SRCDIR = ../source
BLDDIR = ./bin
VPATH := $(SRCDIR)
HEADERS = $(wildcard $(SRCDIR)/*.hpp)
MAIN = $(SRCDIR)/main.cpp
TARGET = $(BLDDIR)/$(PRJNAME)
TEST_MAIN = ../tests/test.cpp
TEST_TARGET = $(BLDDIR)/$(PRJNAME)-test

CXX = g++
CXXFLAGS = -std=c++2b -fno-rtti -O3 -I$(SRCDIR)
CXXFLAGS += -march=native
CXXFLAGS += -Wall -Wextra -Wpedantic -Wfatal-errors -Werror
CXXFLAGS += -Wshadow -Wconversion -Wsign-conversion
CXXFLAGS += -Wsign-promo -Wstrict-overflow=5 -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wstrict-null-sentinel -Wundef -Wno-unused
CXXFLAGS += -I/usr/local/include -L/usr/local/lib -lfmt

#MSVCFLAGS = cl.exe /std:c++latest /utf-8 /W4 /WX /O2

default: executable

all: executable test

debug: CXXFLAGS += -DDEBUG -D_DEBUG -g
debug: executable

executable: $(MAIN) $(HEADERS) makefile
	$(info [executable])
	mkdir -p ${BLDDIR}
	$(CXX) -o $(TARGET) $(CXXFLAGS) $(MAIN)

test: $(TEST_MAIN) $(MAIN) $(HEADERS) makefile
	$(info [test])
	mkdir -p ${BLDDIR}
	$(CXX) -o $(TEST_TARGET) $(CXXFLAGS) $(TEST_MAIN)

clean:
	$(info [clean])
	#rm $(BLDDIR)/*.o
	rm $(TARGET)
	rm $(TEST_TARGET)
