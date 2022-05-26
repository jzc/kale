#include "decls.hpp"
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

// idea: give RAII interface for push/popping scope
template <typename T>
struct ScopeStack {
  std::vector<std::unordered_map<Symbol,T>>
  scope_stack {};
  
  void push_scope() {
    scope_stack.emplace_back();
  }

  void pop_scope() {
    scope_stack.pop_back();
  }

  const T* get(Symbol s) {
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

  void set(Symbol s, const T& v) {
    scope_stack.back()[s] = v;
  }
};

struct Compiler : public FormVisitor {

  using VariableEntry = std::variant<Value*, Function*>;
  ScopeStack<VariableEntry> locals {};
  std::unordered_map<Symbol, VariableEntry> globals {};

  const VariableEntry* lookup(Symbol s);
  
  std::unique_ptr<LLVMContext> context_ptr
    {std::make_unique<LLVMContext>()};
  LLVMContext& context {*context_ptr};
  std::unique_ptr<Module> module_ptr
    {std::make_unique<Module>("test", context)};
  Module& module {*module_ptr};
  IRBuilder<> builder {context};

  // PassBuilder pass_builder {};
  // ModulePassManager module_pm;
  
  Function* main;
  Type* object_type;
  Function* make_number_function;
  Function* make_symbol_function;
  Function* is_nil_function;
  Function* cons_function;
  Function* get_code_function;
  Function* get_fvs_function;
  Function* get_fv_function;
  Function* create_closure_function;
  Value* res;
  bool optimize;

  Compiler(bool optimize);
  Value* compile(Form& f) {
    f.accept(*this);
    return res;
  }
  void operator()(NumberForm& f) override;
  void operator()(SymbolForm& f) override;
  void operator()(IfForm& f) override; 
  void operator()(LetForm& f) override; 
  void operator()(LetrecForm& f) override; 
  void operator()(QuoteForm& f) override;
  void operator()(ApplicationForm& f) override;
  void operator()(LambdaForm& f) override;
  void print_code();

  std::unordered_map<int, Function*>
  call_closure_cache;
  Function* call_closure_function(int n);
};
