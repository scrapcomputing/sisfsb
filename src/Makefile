OBJ=main.o sisfsb.o chips.o pci.o smbus.o utils.o args.o freqentry.o
ifeq ($(OS), LINUX)
	CXX=g++
	RM=rm
	EXTRA=-DLINUX $(EXTRA_FLAGS)
	BLD=build_linux
	MKDIR=mkdir
else
  CXX=gxx
  RM=del
	BLD=build
	MKDIR=mkdir
endif
OBJS=$(addprefix $(BLD)/, $(OBJ))
CXXFLAGS= -Os -fno-rtti -std=c++17 -Wall $(EXTRA)
TARGET=$(BLD)/sisfsb.exe

.phony: all
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $(TARGET)

$(BLD)/%.o: %.cpp $(BLD)
	$(CXX) $< $(CXXFLAGS) -c -o $@

$(BLD):
	$(MKDIR) $@

clean:
	$(RM) $(OBJS)
