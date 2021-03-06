SRCDIR := src
BUILDDIR := build
BINDIR := bin
TARGET := $(BINDIR)/a.out

SOURCES := $(shell find $(SRCDIR) -type f -name *.cpp)
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.cpp=.o))
KERNELS := $(shell find $(SRCDIR) -type f -name *.cl)

DEPENDS := $(OBJECTS:.o=.d)
CXXFLAGS := -Wall -Wextra -MMD -Iinclude -std=c++11 -g

# set V='' to enable verbose output
override V ?= @

all: $(TARGET)
	@echo "[make] success!"

$(TARGET): $(OBJECTS)
	@echo "[link] $@"
	$(V)mkdir -p $(dir $@)
	$(V)$(CXX) $^ -o $@ $(CXXFLAGS) -lOpenCL

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp $(KERNELS)
	@echo "[c++] $<"
	$(V)mkdir -p $(dir $@)
	$(V)$(CXX) $< -c -o $@ $(CXXFLAGS) 

clean:
	@echo "[make] cleaning..."
	$(V)rm -rf $(BUILDDIR) $(BINDIR)

test: all
	$(TARGET)

-include $(DEPENDS)

.PHONY: clean test all 
