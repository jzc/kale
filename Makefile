LLVM_BIN:=~/ws/llvm-project/build/bin
LLVM_CONFIG:=$(LLVM_BIN)/llvm-config
LLC:=$(LLVM_BIN)/llc

# if LLVM was built with shared libraries but not installed in search
# path set this variable to the directory of the location of the .so's
LLVM_LIB:=~/ws/llvm-project/build/lib
ifdef LLVM_LIB
  LLVM_LIB!=realpath $(LLVM_LIB)
  RPATH:=-Wl,-rpath,$(LLVM_LIB)
endif

LLVM_CXX_FLAGS!=$(LLVM_CONFIG) --cxxflags | sed 's/-fno-exceptions//'
LLVM_LD_FLAGS!=$(LLVM_CONFIG) --ldflags --system-libs --libs core native passes orcjit
COMPILE_FLAGS:=-g $(LLVM_CXX_FLAGS)
CXX:=clang++

all: interface.o object.o parsing.o compiler.o kale

test: test.cpp
	$(CXX) $(COMPILE_FLAGS) test.cpp -o test

interface.o: interface.ll
	$(LLC) -filetype=obj interface.ll

object.o: decls.hpp object.cpp
	$(CXX) $(COMPILE_FLAGS) -c object.cpp

parsing.o: decls.hpp parsing.cpp
	$(CXX) $(COMPILE_FLAGS) -c parsing.cpp

compiler.o: decls.hpp compiler.hpp compiler.cpp
	$(CXX) $(COMPILE_FLAGS) -c compiler.cpp

kale: kale.cpp object.o parsing.o compiler.o
# note: put the compiled file before the linker flags, otherwise a
# linker error occurs
	$(CXX) $(COMPILE_FLAGS) $(RPATH) -rdynamic \
	kale.cpp object.o parsing.o compiler.o \
	$(LLVM_LD_FLAGS) -o kale

.PHONY: clean
clean:
	rm -f *.o kale test
