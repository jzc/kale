#include "compiler.hpp"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/ExecutorProcessControl.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Host.h"

using namespace llvm::orc;

int main(int argc, char** argv) {
  auto optimize = false;
  auto end = argv+argc;
  const std::string opt_flag = "-O";
  if (std::find(argv, end, opt_flag) != end) {
    optimize = true;
  }

  ExitOnError ExitOnErr;
  
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();
  Triple triple {sys::getProcessTriple()};
  JITTargetMachineBuilder jtmb{triple};
  ConcurrentIRCompiler irc{jtmb};  
  auto epc = ExitOnErr(SelfExecutorProcessControl::Create());
  auto jit =
    ExitOnErr
    (LLJITBuilder()
     .setExecutorProcessControl(std::move(epc))
     .setJITTargetMachineBuilder(std::move(jtmb))
     .setObjectLinkingLayerCreator([](auto&& es, auto&& triple) {
       return std::make_unique<ObjectLinkingLayer>(es);
     })
     .setCompileFunctionCreator([](auto&& jtmb) {
       return std::make_unique<ConcurrentIRCompiler>(jtmb);
     })
     .create());
  
  Compiler compiler{optimize};
  Reader reader {std::cin};
  auto o = reader.read();
  auto parsed = Parser::parse(o);
  // std::cout << o << "\n";
  compiler.compile(*parsed);
  compiler.print_code();
  ThreadSafeModule tsm {
    std::move(compiler.module_ptr),
    std::move(compiler.context_ptr)
  };
  ExitOnErr(jit->addIRModule(std::move(tsm)));
 
  
  auto interface_buf = MemoryBuffer::getFile("interface.o");
  if (auto ec = interface_buf.getError()) {
    return 1;
  }
  ExitOnErr(jit->addObjectFile(std::move(*interface_buf)));

  // auto object_buf = MemoryBuffer::getFile("object.o");
  // if (auto ec = object_buf.getError()) {
  //   return 1;
  // }
  // ExitOnErr(jit->addObjectFile(std::move(*object_buf)));  

  auto process_dylib_generator =
    ExitOnErr(DynamicLibrarySearchGenerator
	      ::GetForCurrentProcess(jit->getDataLayout().getGlobalPrefix()));
  jit->getMainJITDylib().addGenerator(std::move(process_dylib_generator));
  
  
  auto main = ExitOnErr(jit->lookup("main"));
  main.toPtr<void(*)()>()();
}
