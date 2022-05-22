LLVM_BIN:=~/ws/llvm-project/build/bin
LLVM_CONFIG:=$(LLVM_BIN)/llvm-config
LLC:=$(LLVM_BIN)/llc
LLVM_CXX_FLAGS!=$(LLVM_CONFIG) --cxxflags | sed 's/c++14/c++17/ ; s/-fno-exceptions//'
LLVM_LD_FLAGS!=$(LLVM_CONFIG) --ldflags --system-libs --libs core
COMPILE_FLAGS:=-g $(LLVM_CXX_FLAGS)

# all: kale object.o

test:
	clang++ $(COMPILE_FLAGS) test.cpp -o test

interface.o: interface.ll
	$(LLC) -filetype=obj interface.ll

object.o: decls.hpp object.cpp
	clang++ $(COMPILE_FLAGS) -c object.cpp

parser.o: decls.hpp parser.cpp
	clang++ $(COMPILE_FLAGS) -c parser.cpp

# kale: kale.cpp
# note: put the compiled file before the linker, otherwise a linker error occurs
# clang++ kale.cpp -g `$(LLVM_FLAGS)` -o kale

.PHONY: clean
clean:
	rm *.o kale
