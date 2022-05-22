#include "decls.hpp"

int main() {
  Compiler compiler;
  Reader reader {std::cin};
  auto parsed = Parser::parse(reader.read());
  compiler.compile(*parsed);
  compiler.print_code(); 
    
  // Parser p {std::cin};
  // // Compiler c;
  // auto x = p.parse();
  // // std::cout << "parsed: " << x << "\n";
  // LLVMCompiler c{};
  // auto s = "hello world";
  // // auto z = ConstantDataArray::getRaw(s, sizeof s, Type::getInt8Ty(c.context));
  // c.compile(x);
  // c.builder.CreateRetVoid();
  // c.print_code();
  // c.compile_expression(x);
  // c.code.push_instruction(n(OpCode::ret));
  // c.code.disassemble();
  
  // VM<Safe, NoDebug> vm {};
  // vm.initialize(c.code);
  // std::cout << (const void*)vm.stack_pointer << "\n";
  // vm.step();
  // std::cout << (const void*)vm.stack_pointer << "\n";
  // vm.step();
  // std::cout << (const void*)vm.stack_pointer << "\n";
  // std::cout << vm.evaluate(c.code) << "\n";
}
