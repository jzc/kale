#include "compiler.hpp"
#include <unordered_set>
#include <iterator>
#include <algorithm>
#include "llvm/Pass.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
// #include "llvm/Transforms/Scalar/TailRecursionElimination.h"

#include <type_traits>

template <typename T>
struct Discriminator : public FormVisitor {
  T* ref {nullptr};

  static T* as(Form& f) {
    Discriminator<T> d;
    f.accept(d);
    return d.ref;
  }

  void operator()(NumberForm& f) override {
    if constexpr (std::is_same<T, NumberForm>()) {
      ref = &f;
    }
  }
  void operator()(SymbolForm& f) override {
    if constexpr (std::is_same<T, SymbolForm>()) {
      ref = &f;
    }
  }
  void operator()(IfForm& f) override {
    if constexpr (std::is_same<T, IfForm>()) {
      ref = &f;
    }
  }
  void operator()(LetForm& f) override {
    if constexpr (std::is_same<T, LetForm>()) {
      ref = &f;
    }
  }
  void operator()(LetrecForm& f) override {
    if constexpr (std::is_same<T, LetrecForm>()) {
      ref = &f;
    }
  }
  void operator()(QuoteForm& f) override {
    if constexpr (std::is_same<T, QuoteForm>()) {
      ref = &f;
    }
  }
  void operator()(ApplicationForm& f) override {
    if constexpr (std::is_same<T, ApplicationForm>()) {
      ref = &f;
    }
  }
  void operator()(LambdaForm& f) override {
    if constexpr (std::is_same<T, LambdaForm>()) {
      ref = &f;
    }
  }
};

auto Compiler::lookup(Symbol s) -> const VariableEntry* {
  auto res1 = locals.get(s);
  if (res1) {
    return res1;
  }
  auto res2 = globals.find(s);
  return res2 == globals.end() ? nullptr : &res2->second;
}

Compiler::Compiler(bool optimize)
  : optimize{optimize}
{
  auto&& declare_function =
    [&](auto&& type,
	const char* llvm_name,
	const char* var_name) {
      Function* f =
	Function::Create(type, Function::ExternalLinkage,
			 llvm_name, module);
      if (var_name)
	globals[memory.symbol(var_name)] = f;
      // variables.set(var_name, f);

      return f;
    };
 
  auto void_type =
    Type::getVoidTy(context);      
  auto bool_type =
    Type::getInt1Ty(context);    
  auto i64_type =
    Type::getInt64Ty(context);
  auto i32_type =
    Type::getInt32Ty(context);
  auto double_type=
    Type::getDoubleTy(context);    
  auto char_ptr_type =
    Type::getInt8PtrTy(context);

  object_type = StructType::create({i64_type, i64_type}, "Object");
  auto object_ptr_type = PointerType::getUnqual(object_type);
    
  auto binary_op =
    FunctionType::get(object_type, {object_type, object_type}, false);    
  auto unary_op =
    FunctionType::get(object_type, {object_type}, false);

  main =
    declare_function(FunctionType::get(void_type, {}, false),
		     "main",
		     nullptr);
  make_symbol_function = 
    declare_function(FunctionType::get(object_type,{char_ptr_type}, false),
		     "make_symbol",nullptr);
  make_number_function =
    declare_function(FunctionType::get(object_type,{double_type}, false),
		     "make_number", nullptr);
  is_nil_function =
    declare_function(FunctionType::get(bool_type, {object_type}, false),
		     "is_nil", nullptr);
  get_code_function =
    declare_function(FunctionType::get(char_ptr_type, {object_type, i32_type}, false),
		     "get_code", nullptr);
  get_fvs_function =
    declare_function(FunctionType::get(object_ptr_type, {object_type}, false),
		     "get_fvs", nullptr);
  get_fv_function =
    declare_function(FunctionType::get(object_type, {object_ptr_type, i32_type}, false),
		     "get_fv", nullptr);
  create_closure_function =
    declare_function(FunctionType::get(object_type, {char_ptr_type, object_ptr_type}, false),
		     "create_closure", nullptr);

  declare_function(binary_op, "add", "add");
  declare_function(binary_op, "sub", "sub");
  declare_function(binary_op, "mult", "mult");
  declare_function(binary_op, "_div", "div");
  cons_function = declare_function(binary_op, "cons", "cons");
  declare_function(unary_op, "car", "car");
  declare_function(unary_op, "cdr", "cdr");
  declare_function(unary_op, "print", "print");    
    
  auto block = BasicBlock::Create(context, "entry", main);
  builder.SetInsertPoint(block);
  // manager.add(createTailCallEliminationPass());
  // manager.doInitialization();
}

void Compiler::operator()(LetForm& f) {
  locals.push_scope();
  for (auto&& binding : f.bindings) {
    locals.set(binding.binder, compile(*binding.definition));
  }
  res = compile(*f.body);
  locals.pop_scope();
}

template <typename T>
void remove(const T& t, std::unordered_set<T>& s) {
  auto it = s.find(t);
  if (it != s.end())
      s.erase(it);
}

template <typename G>
struct FreeVarCollector : public FormVisitor {
  G global_lookup;
  std::unordered_set<Symbol> res;
  FreeVarCollector(G global_lookup)
    : global_lookup{global_lookup}
  {}

  void collect(Form& f) {
    f.accept(*this);
  }
  void operator()(NumberForm& f) override {
    // no free variables
  }
  void operator()(SymbolForm& f) override {
    if (!global_lookup(f.symbol))
      res.insert(f.symbol);
  }
  void operator()(IfForm& f) override {
    collect(*f.cond_form);
    collect(*f.then_form);
    collect(*f.else_form);
  }
  void operator()(LetForm& f) override {
    // the semantics of let allow one to refer
    // to variables in previous bindings, thus,
    // (let ((binder definition) . rest) body)
    // is essentially equivalent to
    // ((lambda (binder) (let rest body)) definition)
    collect(*f.body);
    for (auto it = f.bindings.rbegin();
	 it != f.bindings.rend();
	 ++it) {
      auto& binding = *it;
      remove(binding.binder, res);
      collect(*binding.definition);
    }
  }
  void operator()(LetrecForm& f) override {
    collect(*f.body);
    for (auto&& binding : f.bindings) {
      collect(*binding.definition);
      for (auto&& parameter : binding.parameters) {
	remove(parameter, res);
      }
    }
    for (auto&& binding : f.bindings) {
      remove(binding.binder, res);
    }
  }
  void operator()(QuoteForm& f) override {
    //no free variables
  }
  void operator()(ApplicationForm& f) override {
    collect(*f.function_form);
    for (auto&& arg : f.arg_forms) {
      collect(*arg);
    }
  }
  void operator()(LambdaForm& f) override {
    collect(*f.body);
    for (auto&& parameter : f.parameters) {
      remove(parameter, res);
    }
  }
};

template <typename T>
struct ApplicationInjector : public FormVisitor {
  Symbol fn_symbol;
  T injectee_creator;

  ApplicationInjector(Symbol fn_symbol,
		      T injectee_creator)
    : fn_symbol{fn_symbol},
      injectee_creator{injectee_creator}
  {}
  
  void inject(Form& f) {
    f.accept(*this);
  }

  void operator()(NumberForm& f) override {}
  void operator()(SymbolForm& f) override {}
  void operator()(IfForm& f) override {
    inject(*f.cond_form);
    inject(*f.then_form);
    inject(*f.else_form);
  };
  void operator()(LetForm& f) override {
    for (auto&& binding : f.bindings) {
      inject(*binding.definition);
      if (binding.binder == fn_symbol) {
	return;
      }
    }
    inject(*f.body);
  }
  void operator()(LetrecForm& f) override {
    // check if fn_symbol is bound by
    // any of the bindings
    auto&& pred = [&](auto&& binding) {
      return binding.binder == fn_symbol;
    };
    if (std::any_of(f.bindings.begin(),
		    f.bindings.end(),
		    pred)) {
      return;
    }
    // not bound by any of the bindings,
    // inject into the definitions
    for (auto&& binding : f.bindings) {
      inject(*binding.definition);
    }    
  }
  void operator()(QuoteForm& f) override {}
  void operator()(ApplicationForm& f) override {
    for (auto&& arg_form : f.arg_forms) {
      inject(*arg_form);
    }
    auto symbol_form = Discriminator<SymbolForm>::as(*f.function_form);
    if (symbol_form) {
      if (symbol_form->symbol == fn_symbol) {
	for (auto&& injectee : injectee_creator()) {
	  f.arg_forms.emplace_back(std::move(injectee));
	}
      }
    } else {
      inject(*f.function_form);
    }    
  };
  void operator()(LambdaForm& f) override {
    // if (std::find(f.parameters.begin(),
    // 		  f.parameters.end(),
    // 		  fn_symbol)
    // 	!= f.parameters.end()) {
    //   return;
    // }
    // inject(*body);
  }
};

void Compiler::operator()(LetrecForm& f) {
  std::unique_ptr<Form> placeholder = std::make_unique<NumberForm>(0);    
  // we want to collect the free vars happening within the
  // bindings. by swapping out he body of the form with something with
  // no free variables we can collect the free variables with one
  // (toplevel) call to collect.  (alternative to swapping: create two
  // collectors, collect the free vars of the whole letrec form, then
  // the free vars of the body, then compute set difference)
  std::swap(f.body, placeholder);
  FreeVarCollector collector {
    [&](auto&& s) {
      return globals.find(s) != globals.end();
    }
  };
  collector.collect(f);
  std::swap(f.body, placeholder);
  std::vector<Symbol> fvs {collector.res.begin(), collector.res.end()};

 
  auto&& injectee_creator = [&](){
    std::vector<std::unique_ptr<Form>> injectees;
    for (auto&& fv : fvs) {
      injectees.emplace_back(std::make_unique<SymbolForm>(fv));
    }
    return injectees;
  };

  if (fvs.size() > 0) {
    // inject the free variables into each binding definition and body
    for (auto&& binding : f.bindings) {
      ApplicationInjector ai{binding.binder, injectee_creator};
      ai.inject(*binding.definition);
      ai.inject(*f.body);
      // also inject fvs into parameters
      for (auto&& fv : fvs) {
	binding.parameters.emplace_back(fv);
      }
    }
  }
  auto body_insert_block = builder.GetInsertBlock();
  locals.push_scope();

  // Create all the functions add them to the
  // current locals  
  std::vector<Function*> fns;
  for (auto&& binding : f.bindings) {
    std::vector<Type*> parameter_types {binding.parameters.size(), object_type};
    auto type = FunctionType::get(object_type, parameter_types, false);
    Function* fn = Function::Create(type, Function::ExternalLinkage,
				    *binding.binder, module);    
    locals.set(binding.binder, fn);
    fns.push_back(fn); 
  }

  // Recursively compile each function
  auto binding_it = f.bindings.begin();
  for (auto&& fn : fns) {
    auto&& binding = *binding_it++;
    auto it = binding.parameters.begin();
    locals.push_scope();
    for (auto&& arg_value : fn->args()) {
      locals.set(*it++, &arg_value);
    }
    auto block = BasicBlock::Create(context, "entry", fn);
    builder.SetInsertPoint(block);
    builder.CreateRet(compile(*binding.definition));
    locals.pop_scope();
  }

  builder.SetInsertPoint(body_insert_block);
  res = compile(*f.body);
  locals.pop_scope();
}

void Compiler::operator()(LambdaForm& f) {
  // Get the free vars of the body
  FreeVarCollector collector {
    [&](auto&& s) {
      return globals.find(s) != globals.end();
    }
  };
  collector.collect(f);
  // Now filter out all the free variables that are
  // mapped to function*'s
  std::vector<Symbol> fvs {};
  fvs.reserve(collector.res.size()); 
  std::copy_if(collector.res.begin(), collector.res.end(),
	       std::back_inserter(fvs),
	       [&](auto&& s) {
		 auto res = lookup(s);
		 if (!res) throw std::runtime_error("");
		 return std::holds_alternative<Value*>(*res);
	       });
  
  // Create a function for the body
  std::vector<Type*> parameter_types {1+f.parameters.size(), object_type};
  parameter_types[0] = PointerType::getUnqual(object_type);
  auto type = FunctionType::get(object_type, parameter_types, false);
  Function* fn = Function::Create(type, Function::ExternalLinkage,
				  "lambda", module);  
  auto before_insert_block = builder.GetInsertBlock();  
  auto lambda_insert_block = BasicBlock::Create(context, "entry", fn);
  builder.SetInsertPoint(lambda_insert_block);
  // Setting up the locals
  locals.push_scope();
  // Set up the free vars to fetch the value from the fv array
  for (int i = 0; i < fvs.size(); ++i) {
    auto idx = ConstantInt::get(Type::getInt32Ty(context), i);				
    auto fv_val = builder.CreateCall(get_fv_function, {fn->getArg(0), idx});
    locals.set(fvs[i], fv_val);
  }
  // Set up regular parameters
  auto pit = f.parameters.begin();
  auto vit = fn->arg_begin()+1;
  for (; vit != fn->arg_end(); ++vit, ++pit) {
    locals.set(*pit, vit);
  }
  // Now recursively compile the body
  builder.CreateRet(compile(*f.body));
  locals.pop_scope();
  builder.SetInsertPoint(before_insert_block);
  
  // Now that we've compiled the body, we need to create
  // an array of the free vars and then we can create a closure
  auto arr =
    builder.CreateAlloca(object_type,
			 ConstantInt::get(Type::getInt32Ty(context),
					  fvs.size()));
  for (int i = 0; i < fvs.size(); ++i) {
    Value* idx = ConstantInt::get(Type::getInt32Ty(context), i);
    auto ptr = builder.CreateGEP(object_type, arr, {idx});
    auto res = lookup(fvs[i]);
    if (!res) throw std::runtime_error("");
    Value* fv_val;
    if (std::holds_alternative<Value*>(*res)) {
      fv_val = std::get<Value*>(*res);
    } else {
      throw std::runtime_error("can't handle");
    }
    builder.CreateStore(fv_val, ptr);
  }

  auto fn_ptr = builder.CreateBitCast(fn, Type::getInt8PtrTy(context));
  res = builder.CreateCall(create_closure_function, {fn_ptr, arr});
}

void Compiler::operator()(IfForm& f) {
  // compile the condition, then check if it is nil
  auto condition_code =
    builder.CreateCall(is_nil_function,
		       {compile(*f.cond_form)},
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
  auto then_code = compile(*f.then_form);
  builder.CreateBr(after_block);
  then_block = builder.GetInsertBlock();

  // compile else branch
  curr_fn->getBasicBlockList().push_back(else_block);
  builder.SetInsertPoint(else_block);
  auto else_code = compile(*f.else_form);
  builder.CreateBr(after_block);
  else_block = builder.GetInsertBlock();

  // compile after branch
  curr_fn->getBasicBlockList().push_back(after_block);
  builder.SetInsertPoint(after_block);
  auto phi = builder.CreatePHI(object_type, 2);
  phi->addIncoming(then_code, then_block);
  phi->addIncoming(else_code, else_block);
  res = phi;    
}

void Compiler::operator()(QuoteForm& f) {
  std::function<Value*(const Object&)>
    rec = [&](auto&& o) -> Value* {
      if (o.is_number()) {
	NumberForm f {o.as_number()};
	return compile(f);
      }
      if (o.is_symbol()) {
	auto c = ConstantDataArray::getString(context, *o.as_symbol());
	auto gv = new GlobalVariable{module,
				     c->getType(),
				     true,
				     GlobalValue::ExternalLinkage, c};
	auto gvc = builder.CreateBitCast(gv, Type::getInt8PtrTy(context));
	return builder.CreateCall(make_symbol_function, {gvc});
      }
      return builder.CreateCall(cons_function, {rec(o.car()), rec(o.cdr())});
    };
  res = rec(f.arg);
  // throw std::runtime_error("can't handle");
}

void Compiler::operator()(SymbolForm& f) {
  auto&& it = lookup(f.symbol);
  if (!it) { throw std::runtime_error("couldn't find variable");}
      
  if (std::holds_alternative<Value*>(*it)) {
    res = std::get<Value*>(*it);
  } else {
    throw std::runtime_error("can handle this");
  }
}

void Compiler::operator()(NumberForm& f) {
  auto n = ConstantFP::get(context, APFloat(f.number));
  res = builder.CreateCall(make_number_function, {n});
}

void Compiler::operator()(ApplicationForm& f) {
  SymbolForm* symbol_form = Discriminator<SymbolForm>::as(*f.function_form);
  Value* head;
  if (symbol_form) {
    auto&& it = lookup(symbol_form->symbol);
    if (!it) { throw std::runtime_error("variable not found"); }
    if (std::holds_alternative<Function*>(*it)) {
      auto&& callee = std::get<Function*>(*it);
      auto n_args = callee->getFunctionType()->getNumParams();
      if (f.arg_forms.size() != n_args) {
	throw std::runtime_error("invalid number of args");
      }
      std::vector<Value*> arg_values;
      arg_values.reserve(n_args);
      for (auto&& arg_form : f.arg_forms) {
	arg_values.push_back(compile(*arg_form));
      }
      res = builder.CreateCall(callee, arg_values);
      return;
    } else {
      head = std::get<Value*>(*it);
    }
  } else {
    head = compile(*f.function_form);
  }

  std::vector<Value*> arg_values;
  arg_values.reserve(f.arg_forms.size()+1);
  arg_values.push_back(head);
  for (auto&& arg : f.arg_forms) {
    arg_values.push_back(compile(*arg));
  }
  res = builder.CreateCall(call_closure_function(f.arg_forms.size()),
			   arg_values);		     
}

void Compiler::print_code() {
  builder.CreateRetVoid();

  if (optimize) {
    // Create the analysis managers.
    LoopAnalysisManager LAM;
    FunctionAnalysisManager FAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;

    // Create the new pass manager builder.
    // Take a look at the PassBuilder constructor parameters for more
    // customization, e.g. specifying a TargetMachine or various debugging
    // options.
    PassBuilder PB;

    // Register all the basic analyses with the managers.
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    // Create the pass manager.
    // This one corresponds to a typical -O2 optimization pipeline.
    ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(OptimizationLevel::O2);
    MPM.run(module, MAM);
  }
  module.print(outs(), nullptr);
}


/* Closures/lambdas work like this: first, similarly to the letrec
   case, close over all the free variables and add them as free
   variables to the parameters of the lambda. then, generate code for
   the body of the lambda like a normal function, giving us a function
   pointer. Now, we create a closure object (via a runtime function)
   by supplying a pointer, the number of parameters the function has
   (not counting the free variables), and the values of the free
   variables.

   To call a closure, we call a runtime function which takes an int,
   signifying the number of arguments we are about to call the closure with,
   and returns the code pointer if the number of arguments matches
   the number of parameters of the closure; otherwise, the runtime aborts.
   Thus, if we receive the pointer, we can then safely bitcast it
   to the correct number of arguments, and then call it like a
   regular function. Note that the code for the lambda will be compiled as
   a non varargs function - it probably is undefined behavior
   to cast the function we receive into a varags function and
   try to call it. Thus, we'll need to generate code for every
   closure call for a given number of arguments.
 */
Function* Compiler::call_closure_function(int n) {
  if (auto res = call_closure_cache.find(n);
      res != call_closure_cache.end()) {
    return res->second;
  }
  auto before_insert_block = builder.GetInsertBlock();

  // 1 parameter for the closure argument,
  // n parameters for the lambda arguments
  auto this_fn_type =
    FunctionType::get(object_type,
		      std::vector{static_cast<unsigned>(n+1), object_type},
		      false);
  auto fn =
    Function::Create(this_fn_type, Function::ExternalLinkage,
		     {"call_closure", std::to_string(n)}, module);
  auto block = BasicBlock::Create(context, "entry", fn);
  builder.SetInsertPoint(block);
  auto code =
    builder.CreateCall(get_code_function,
		       {fn->getArg(0), ConstantInt::get(Type::getInt32Ty(context), n)});
  auto fvs =
    builder.CreateCall(get_fvs_function, {fn->getArg(0)});

  auto fnptr_parameter_types = std::vector(static_cast<unsigned>(n+1), object_type);
  fnptr_parameter_types[0] = PointerType::getUnqual(object_type);

  auto fn_type = FunctionType::get(object_type, fnptr_parameter_types, false);
  auto fnptr_type = PointerType::getUnqual(fn_type);
  Value* fnptr = builder.CreateBitCast(code, fnptr_type);
  std::vector<Value*> arguments;
  arguments.push_back(fvs);
  for (auto it = fn->arg_begin()+1; it != fn->arg_end(); ++it) {
    arguments.push_back(it);
  }
    // Value* idx = ConstantInt::get(Type::getInt32Ty(context), i);
    // auto ptr = builder.CreateGEP(object_type, fvs, {idx});
    // auto fv = builder.CreateLoad(object_type, ptr);
    // arguments.push_back(fv);

  auto ret = builder.CreateCall(fn_type, fnptr, arguments);
  builder.CreateRet(ret);    
  builder.SetInsertPoint(before_insert_block);
  return fn;
}
