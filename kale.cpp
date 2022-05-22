#include <iostream>
#include <variant>
#include <optional>
#include <vector>
#include <array>
#include <limits>
#include <cstdint>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <list>
#include <type_traits>
#include <cstring>
#include <optional>
#include <functional>

// template <typename T>



struct Compiler {
  Code code;

  // compiles the expression
  // initial op a_1 op a_2 op a_3 ... op a_n
  // associated to the left
  void compile_fold(std::uint8_t opcode,
		    const Object& initial,
		    const Object& lst) {
    compile_constant(initial);
    for (auto p = lst; p != SC::nil; p = p.cdr()) {
      compile_expression(p.car());
      code.push_instruction(opcode);
    }
  }

  void compile_constant(const Object& o) {
    code.push_instruction(n(OpCode::constant));
    code.push_instruction(code.push_constant(o));
  }

  void compile_bin_op(std::uint8_t opcode,const Object& form) {
    auto x = form.cdr().car();
    auto y = form.cdr().cdr().car();
    compile_expression(x);
    compile_expression(y);
    code.push_instruction(opcode);
  }

  void compile_if(const Object& form) {
    auto bool_form = form.cdr().car();
    auto then_form = form.cdr().cdr().car();
    auto else_form = form.cdr().cdr().cdr().car();
	
    compile_expression(bool_form);
    code.push_instruction(n(OpCode::jump_if_nil));
    code.push_instruction(0);
    // if bool_form was not nil, then then_location
    // is the location where the then_form code starts
    // after executing the jump_if_nil instruction
    auto then_location = code.instructions.size();
    compile_expression(then_form);
    code.push_instruction(n(OpCode::jump));
    code.push_instruction(0);
    // if bool_form_was nil, then else_location is the
    // location of the else_form code, and is where
    // the jump_if_nil instruction should jump us to
    auto else_location = code.instructions.size();
    compile_expression(else_form);
    auto end_location = code.instructions.size();

    // Compute the distance to jump for the jump_if_nil instruction
    auto jump1_distance = else_location-then_location;
    if (jump1_distance > std::numeric_limits<uint8_t>::max()) {
      throw std::runtime_error("jump is too big");
    }
    code.instructions[then_location-1] = jump1_distance;

    // Compute the distance to jump for the jump instruction
    auto jump2_distance = end_location-else_location;
    if (jump2_distance > std::numeric_limits<uint8_t>::max()) {
      throw std::runtime_error("jump is too big");
    }
    code.instructions[else_location-1] = jump2_distance;
  }

  std::unordered_map<std::string, std::uint8_t>
  local_map;

  int let_depth {0};

  void compile_let(const Object& form) {
    // if (let_depth == 0) {
    //   code.push_instruction(n(OpCode::alloc));
    //   auto curr_local_count = local_map.size();
    //   auto alloc_size_location = code.instructions.size();
    //   code.push_instruction(0);
    // }
    // ++let_depth;

    // auto bindings = form.cdr().car();
    // auto body = form.cdr().cdr().car();
    // for (auto p = bindings; p != SC::nil; p = p.cdr()) {
    //   auto binder = p.car();
    //   auto body = p.cdr().car();
    //   compile_expression(body);
    // }
    
    // --let_depth;
    // if (let_depth == 0) {
    //   auto local_count = local_map.size()-curr_local_count;
    //   code.push_instruction(n(OpCode::unalloc));
      
    // }
  }
			   
  void compile_expression(const Object& form) {
    if (form.is_number()) {
      compile_constant(form);
    } else if (form.is_symbol()) {
	
    } else if (form.is_cons()) {
      const auto& car = form.car(); 
      if (car == Object{SC::quote}) {
	compile_constant(form.cdr().car());
      } else if (car == Object{SC::add}) {
	compile_fold(n(OpCode::add), Object{0.0}, form.cdr());
      } else if (car == Object{SC::sub}) {
	compile_fold(n(OpCode::sub), Object{0.0}, form.cdr());
      } else if (car == Object{SC::mult}) {
	compile_fold(n(OpCode::mult), Object{1.0}, form.cdr());
      } else if (car == Object{SC::div}) {
	compile_fold(n(OpCode::div), Object{1.0}, form.cdr());
      } else if (car == SC::cons) {
	compile_bin_op(n(OpCode::cons), form);
      } else if (car == Object{SC::if_}) {
	compile_if(form);
      } else if (car == Object{SC::let}) {
	compile_let(form);	
      } else if (car == Object{SC::letrec}) {

      }
    }
  }
};

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;

template <typename T>
struct ScopeStack {
  std::vector<std::unordered_map<std::string,T>>
  scope_stack {};

  // ScopeStack() {
  //   push_scope();
  // }

  void push_scope() {
    scope_stack.emplace_back();
  }

  void pop_scope() {
    scope_stack.pop_back();
  }

  const T* get(const std::string& s) {
    if (scope_stack.size() == 0) { return nullptr; }
    
    for (auto it = scope_stack.rbegin();
	 it != scope_stack.rend();
	 ++it) {
      auto&& map = *it;
      auto&& res = map.find(s);
      if (res != map.end()) {
	return &res->second;
      }
    }
    return nullptr;
  }

  void set(const std::string& s, const T& v) {
    scope_stack.back()[s] = v;
  }
};

struct LLVMCompiler {

  using VariableEntry = std::variant<Function*, Value*>;
  ScopeStack<VariableEntry> locals {};
  std::unordered_map<std::string, VariableEntry> globals {};

  const VariableEntry* lookup(const std::string& s) {
    auto res1 = locals.get(s);
    if (res1) {
      return res1;
    }
    auto res2 = globals.find(s);
    return res2 == globals.end() ? nullptr : &res2->second;
  }
  
  LLVMContext context {};
  Module module {"test", context};
  IRBuilder<> builder {context};
  Function* main;
  Type* object_type;
  Function* make_number_function;
  Function* make_symbol_function;
  Function* is_nil_function;

  LLVMCompiler()  {
    auto&& declare_function =
      [&](auto&& type,
	  const char* llvm_name,
	  const char* var_name) {
	Function* f =
	  Function::Create(type, Function::ExternalLinkage,
			   llvm_name, module);
	if (var_name)
	  globals[var_name] = f;
	  // variables.set(var_name, f);

	return f;
      };

    auto void_type =
      Type::getVoidTy(context);      
    auto bool_type =
      Type::getInt1Ty(context);    
    auto i64_type =
      Type::getInt64Ty(context);
    auto double_type=
      Type::getDoubleTy(context);    
    auto char_ptr_type =
      Type::getInt8PtrTy(context);

    object_type = StructType::create({i64_type, i64_type}, "Object");
    
    auto binary_op =
      FunctionType::get(object_type, {object_type, object_type}, false);    
    auto unary_op =
      FunctionType::get(object_type, {object_type}, false);

    main =
      declare_function(FunctionType::get(void_type, {}, false),
		       "main",
		       nullptr);
    make_symbol_function = 
      declare_function(FunctionType::get(object_type,{char_ptr_type, i64_type}, false),
		       "make_symbol",nullptr);
    make_number_function =
      declare_function(FunctionType::get(object_type,{double_type}, false),
		       "make_number", nullptr);
    is_nil_function =
      declare_function(FunctionType::get(bool_type, {object_type}, false),
		     "is_nil", "nil?");

    declare_function(binary_op, "add", "add");
    declare_function(binary_op, "sub", "sub");
    declare_function(binary_op, "mult", "mult");
    declare_function(binary_op, "_div", "div");
    declare_function(binary_op, "cons", "cons");
    declare_function(unary_op, "car", "car");
    declare_function(unary_op, "cdr", "cdr");
    declare_function(unary_op, "print", "print");    
    
    auto block = BasicBlock::Create(context, "entry", main);
    builder.SetInsertPoint(block);    
  }

  Value* compile_let(const Object& form) {
      locals.push_scope();
      auto&& bindings = form.cdr().car();
      auto&& body = form.cdr().cdr().car();
      for (auto p = bindings; p != SC::nil; p = p.cdr()) {
	auto&& binding_form = p.car();
	auto&& binder = binding_form.car();
	auto&& definition = binding_form.cdr().car();
	locals.set(*binder.as_symbol(), compile(definition));
      }
      auto&& res = compile(body);
      locals.pop_scope();
      return res;
  }

  // static collect_free_vars(Object& o) {
    
  // }

  Value* compile_letrec(const Object& form) {
    auto&& bindings = form.cdr().car();
    auto&& body = form.cdr().cdr().car();
    std::unordered_set<std::string_view> free_vars;
    std::unordered_map<std::string_view, std::pair<Function*, Object>>
      binder_body_map;
    for (auto p = bindings; p != SC::nil; p = p.cdr()) {
      auto&& binding_form = p.car();
      auto&& binder = binding_form.car();
      auto&& args = binding_form.cdr().car();
      auto&& definition = binding_form.cdr().cdr().car();

      // populate arg_set
      std::unordered_set<std::string_view> arg_set;
      for (auto q = args; args != SC::nil; q = q.cdr()) {
	arg_set.emplace(*q.car().as_symbol());
      }

      // first, collect all the free variables occurring
      // in every binding.

      // then, inside the bindings, append the free variables as
      // parameters to each function

      // then, every time one of the new functions is called,
      // pass the free variable arguments back in as
      // parameters.
    }
  }

  Value* compile_if(const Object& form) {
    auto condition_form = form.cdr().car();
    auto then_form = form.cdr().cdr().car();
    auto else_form = form.cdr().cdr().cdr().car();

    // compile the condition, then check if it is nil
    auto condition_code =
      builder.CreateCall(is_nil_function,
			 {compile(condition_form)},
			 "cond");

    // create blocks
    auto then_block = BasicBlock::Create(context, "then-block");
    auto else_block = BasicBlock::Create(context, "else-block");
    auto after_block = BasicBlock::Create(context, "after-block");

    // note order: first block is for true, second is for false
    // is-nil = true => goto else; is-nil = false => goto then;
    builder.CreateCondBr(condition_code, else_block, then_block);

    // get the current function we are emitting code to
    auto curr_fn = builder.GetInsertBlock()->getParent();

    // compile then branch
    curr_fn->getBasicBlockList().push_back(then_block);
    builder.SetInsertPoint(then_block);
    auto then_code = compile(then_form);
    builder.CreateBr(after_block);
    then_block = builder.GetInsertBlock();

    // compile else branch
    curr_fn->getBasicBlockList().push_back(else_block);
    builder.SetInsertPoint(else_block);
    auto else_code = compile(else_form);
    builder.CreateBr(after_block);
    else_block = builder.GetInsertBlock();

    // compile after branch
    curr_fn->getBasicBlockList().push_back(after_block);
    builder.SetInsertPoint(after_block);
    auto phi = builder.CreatePHI(object_type, 2);
    phi->addIncoming(then_code, then_block);
    phi->addIncoming(else_code, else_block);
    return phi;    
  }

  Value* compile(const Object& form) {
    if (form.is_symbol()) {
      auto&& res = lookup(*form.as_symbol());
      if (!res) { throw std::runtime_error("couldn't find variable");}
      
      if (std::holds_alternative<Value*>(*res)) {
	return std::get<Value*>(*res);
      } else {
	throw std::runtime_error("can handle this");
      }
    }
    
    if (form.is_number()) {
      auto n = ConstantFP::get(context, APFloat(form.as_number()));
      return builder.CreateCall(make_number_function, {n});
    }

    if (form.is_cons() && form.car().is_symbol()) {
      auto& car = form.car();

      // special forms
      if (car == SC::let) { return compile_let(form); }
      if (car == SC::letrec) { return compile_letrec(form); }
      if (car == SC::if_) { return compile_if(form); }

      auto&& res = lookup(*car.as_symbol());
      if (!res) { throw std::runtime_error("variable not found"); }
      if (std::holds_alternative<Function*>(*res)) {
	auto&& callee = std::get<Function*>(*res);
	auto n_args = callee->getFunctionType()->getNumParams();
	std::vector<Value*> args; args.reserve(n_args);
	for (auto p = form.cdr(); p != SC::nil; p = p.cdr()) {
	  args.push_back(compile(p.car()));
	}
	if (args.size() != n_args) { throw std::runtime_error("invalid number of args"); }
	return builder.CreateCall(callee, args);
      } else {
	throw std::runtime_error("cant handle this");
      }
    }
  }

  void print_code() {
    module.print(outs(), nullptr);
  }
};

int main() {
  Parser p {std::cin};
  // Compiler c;
  auto x = p.parse();
  // std::cout << "parsed: " << x << "\n";
  LLVMCompiler c{};
  auto s = "hello world";
  // auto z = ConstantDataArray::getRaw(s, sizeof s, Type::getInt8Ty(c.context));
  c.compile(x);
  c.builder.CreateRetVoid();
  c.print_code();
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
