enum class OpCode : std::uint8_t {
  add, sub, mult, div,
  cons, car, cdr, consp,
  constant,
  jump, jump_if_nil,
  call, ret,
  alloc, unalloc,
  load, store,
};

constexpr std::uint8_t n(OpCode x) {
  return static_cast<std::uint8_t>(x);
  // return std::uint8_t{x};
}

OpCode as_opcode(std::uint8_t x) {
  return OpCode{x};
}

struct Code {
  std::vector<std::uint8_t> instructions {};
  std::vector<Object> constants {};

  // std::uint8_t begin() {
  //   return &*instructions.begin();
  // }
  
  // bool is_valid_code_pointer(std::uint8_t* p) {
  //   return p >= &*instructions.begin()
  //     && p < &*instructions.end();
  // }

  // std::uint8_t read_byte(std::uint8_t* p) {
  //   if (is_valid_code_pointer(p)) {
  //     return *p;
  //   } else {
  //     throw std::runtime_error("attempted to read code out of bounds");
  //   }
  // }

  // std::uint16_t read_short(std::uint8_t* p) {
  //   if (is_valid_code_pointer(p)
  // 	&& is_valid_code_pointer(p+1)) {
  //     std::uint16_t x;
  //     std::memcpy(p, &x, sizeof(x));
  //     return x;
  //   } else {
  //     throw std::runtime_error("attempted to read code out of bounds");
  //   }
  // }
  
  std::uint8_t push_constant(const Object& o) {
    if (constants.size() == std::numeric_limits<std::uint8_t>::max()) {
      throw std::runtime_error("out of constant space");
    }
    constants.push_back(o);
    return constants.size()-1;
  }

  void push_instruction(std::uint8_t i) {
    instructions.push_back(i);
  }

  void disassemble() {
    for (auto it = instructions.begin();
	 it != instructions.end();
	 ++it) {
      std::cout << it-instructions.begin() << ": ";
      switch (as_opcode(*it)) {
      case OpCode::add:
	std::cout << "add\n";
	break;
      case OpCode::sub:
	std::cout << "sub\n";
	break;
      case OpCode::mult:
	std::cout << "mult\n";
	break;
      case OpCode::div:
	std::cout << "div\n";
	break;
      case OpCode::cons:
	std::cout << "cons\n";
	break;
      case OpCode::car:
	std::cout << "car\n";
	break;
      case OpCode::cdr:
	std::cout << "cdr\n";
	break;
      case OpCode::consp:
	std::cout << "consp\n";
	break;
      case OpCode::constant:
	std::cout << "constant ";
	std::cout << constants[*(++it)];
	std::cout << "\n";
	break;
      case OpCode::load:
	std::cout << "load ";
	std::cout << int{*(++it)};
	std::cout << "\n";
	break;
      case OpCode::jump:
	std::cout << "jump ";
	std::cout << int{*(++it)};
	std::cout << "\n";
	break;
      case OpCode::jump_if_nil:
	std::cout << "jump_if_nil ";
	std::cout << int{*(++it)};
	std::cout << "\n";
	break;
      case OpCode::ret:
	std::cout << "ret\n";
	break;
      }
    }
  }
};

struct CallFrame {
  // base pointer, i.e. where variables start for current frame
  const Object* bp;
  // instruction pointer, i.e. what code to execute next
  const std::uint8_t* ip;
  // code we are executing (to read constants)
  const Code* code;
  
  CallFrame() = default;
  CallFrame(const Object* bp,
	    const std::uint8_t* ip,
	    const Code* code)
    : bp{bp}, ip{ip}, code{code} {}
};

struct Safe {};
struct Unsafe {};
struct Debug {};
struct NoDebug {};

template <typename S = Safe, typename D = Debug>
struct VM {
  // By default, the VM will check if the stack pointer
  // or instruction pointer go out of bounds. Constructing
  // VM<Unsafe> will disable these checks.
  static constexpr bool is_safe = std::is_same<S, Safe>();
  static constexpr bool is_debug = std::is_same<D, Debug>();
  
  
  bool halted {};

  const static int stack_size {4096};
  std::array<Object, stack_size> stack {};
  Object* sp {};
  
  const static int call_stack_size {1024};
  std::array<CallFrame, call_stack_size> call_stack {};
  CallFrame* call_frame {};

  // bool valid_stack_pointer(std::Object* p) {
  //   return p >= &*code->instructions.begin()
  //     && p >= &*code->instructions.end();
  // }

  const Code*& code() {
    return call_frame->code;
  }

  const std::uint8_t*& ip() {
    return call_frame->ip;
  }

  void initialize(const Code& code) {
    halted = false;
    sp = &stack[0];
    call_frame = &call_stack[0];
    *call_frame = CallFrame{&stack[0], &code.instructions[0], &code};
  }

  Object evaluate(const Code& code) {
    initialize(code);
    execute();
    return pop();
  }

  bool is_valid_code_pointer(const std::uint8_t* p) {
    return p >= &code()->instructions[0]
      && p < (&code()->instructions[0] + code()->instructions.size());
  } 

  std::uint8_t read_instruction() {
    if constexpr (is_safe) {
      if (!is_valid_code_pointer(ip())) {
	throw std::runtime_error("instruction pointer out of bounds");	
      } 
    }
    return *(ip()++);
  }

  std::uint16_t read_short() {
    if constexpr (is_safe) {
      if (!(is_valid_code_pointer(ip())
	    && is_valid_code_pointer(ip()+1))) {
	throw std::runtime_error("instruction pointer out of bounds");
      }
    }
    std::uint16_t x;
    std::memcpy(ip(), &x, sizeof(x));
    ip() += 2;
    return x;
  }

  Object read_constant() {
    return call_frame->code->constants[read_instruction()];
  }

  Object pop() {
    if constexpr (is_safe) {
      if (sp < stack.begin()) {
	throw std::runtime_error("stack underflow");
      } else if (sp >= stack.end()) {
	throw std::runtime_error("stack overflow");
      }
    }
    return *(--sp);
  }

  void push(Object o) {
    if constexpr (is_safe) {
      if (sp < stack.begin()) {
	throw std::runtime_error("stack underflow");
      } else if (sp >= stack.end()) {
	throw std::runtime_error("stack overflow");
      }
    }
    *sp = o;
    sp++;
  }

  template <typename T>
  void do_bin_op(T op) {
    auto y = pop();
    push(op(pop(), y));
  }
  
  void step() {
    // note: if you do not return from the switch,
    // then an error is thrown to signal an unknown instruction.
    // compared to using break, this allows the compiler to
    // detect if all values from OpCode are being handled
    switch(as_opcode(read_instruction())) {
    case OpCode::constant:
      push(read_constant());
      return;
    case OpCode::add:
      do_bin_op([](auto&& x, auto&& y) {
	return x+y;
      });
      return;
    case OpCode::sub: 
      do_bin_op([](auto&& x, auto&& y) {
	return x-y;
      });
      return;
    case OpCode::mult: 
      do_bin_op([](auto&& x, auto&& y) {
	return x*y;
      });
      return;    
    case OpCode::div:
      do_bin_op([](auto&& x, auto&& y) {
	return x/y;
      });
      return;
    case OpCode::cons:
      do_bin_op([&](auto&& x, auto&& y) {
	return Object{memory.cons(x, y)};
      });
      return;
    case OpCode::consp: {
      auto x = pop();
      push(x.is_cons() ? x : SC::nil);
    }
    case OpCode::car:
      push(pop().car());
      return;
    case OpCode::cdr:
      push(pop().cdr());
      return;
    case OpCode::load:
      // read an instruction (the offset),
      // read the variable from the current base_pointer
      // plus the offset,
      // push on the stack.
      push(*(call_frame->bp + read_instruction()));
      return;
    case OpCode::jump:
      ip() += read_instruction();
      return;
    case OpCode::jump_if_nil: {
      auto jump_distance = read_instruction();
      if (pop().is_nil()) {
	call_frame->ip += jump_distance;
      }
      return;
    }
    case OpCode::call:      
      return;
    case OpCode::ret:
      // ret at bottom of call stack halts
      if (call_frame == &call_stack[0]) {
	halted = true;
      } else {

      }
      return;
    }
    throw std::runtime_error("invalid instruction");    
  }

  int ip_offset() {
    return call_frame->ip - &call_frame->code->instructions[0];
  }

  void print_stack() {
    std::cout << "stack: ";
    for (auto x = &stack[0]; x != sp; x++) {
      std::cout << *x;
      if (x+1 != sp)
	std::cout << ", ";
    }
    std::cout << "\n";
  }

  void execute() {
    while (!halted) {
      if constexpr (is_debug) {
	std::cout << "offset: "<< ip_offset() << "\n";
	print_stack();
      } 
      step();      
    }
  }
};

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
