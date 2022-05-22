using Number = double;
using Symbol = const std::string*;
// struct Symbol;
struct Cons;
struct Code;

struct Object {
  std::variant<Number, Symbol, Cons*> data;
  Object() : data{} {}
  explicit Object(double d) : data{d} {}
  explicit Object(Cons* c) : data{c} {}
  explicit Object(Symbol s) : data{s} {}
  bool is_number() const { return std::holds_alternative<Number>(data); }
  Number as_number() const { return std::get<Number>(data); }
  bool is_symbol() const { return std::holds_alternative<Symbol>(data); }
  Symbol as_symbol() const { return std::get<Symbol>(data); } 
  bool is_cons() const { return std::holds_alternative<Cons*>(data); }
  const Cons* as_cons() const { return std::get<Cons*>(data); }

  bool is_nil() const;
  const Object& car() const;
  const Object& cdr() const;  

  friend bool operator==(const Object& o1, const Object& o2) {
    return o1.data == o2.data;
  }

  friend bool operator!=(const Object& o1, const Object& o2) {
    return !(o1 == o2);
  }

  friend Object operator+(const Object& o1, const Object& o2) {
    if (o1.is_number() && o2.is_number()) {
      return Object{o1.as_number() + o2.as_number()};
    }
    throw std::runtime_error("type error");
  }

  friend Object operator-(const Object& o1, const Object& o2) {
    if (o1.is_number() && o2.is_number()) {
      return Object{o1.as_number() - o2.as_number()};
    }
    throw std::runtime_error("type error");
  }

  friend Object operator*(const Object& o1, const Object& o2) {
    if (o1.is_number() && o2.is_number()) {
      return Object{o1.as_number() * o2.as_number()};
    }
    throw std::runtime_error("type error");
  }

  friend Object operator/(const Object& o1, const Object& o2) {
    if (o1.is_number() && o2.is_number()) {
      return Object{o1.as_number() / o2.as_number()};
    }
    throw std::runtime_error("type error");
  }
};

enum class Color {
  white, gray, black 
};

// Cons cell, each cell is equipped with a color used
// the marking phase in garbage collection
struct Cons {
  Color c;
  Object car;
  Object cdr;
  Cons() = default;
  Cons(Object car, Object cdr)
    : car{car}, cdr{cdr} {}
};


// Memory manager, in charge of allocating and freeing conses
// and interning symbols
struct Memory {
  std::list<Cons> conses {};
  std::list<std::string> symbol_storage {};
  std::unordered_map<std::string, const std::string*>
  symbol_lookup {};

  Cons* cons(Object car, Object cdr) {
    conses.emplace_back(car, cdr);
    return &conses.back();
  }

  Symbol symbol(const std::string& s) {
    auto [it, did_insert] = symbol_lookup.insert({s, nullptr});
    if (did_insert) {
      symbol_storage.push_back(s);
      it->second = &symbol_storage.back();
    }
    return it->second;
  }
};


Memory memory {};

// Symbol constants, used for quick lookup of these
// symbols during compilation
namespace SC {
  const auto nil = Object{memory.symbol("nil")};
  const auto let = Object{memory.symbol("let")};
  const auto letrec = Object{memory.symbol("letrec")};
  const auto def = Object{memory.symbol("def")};
  const auto if_ = Object{memory.symbol("if")};
  
  const auto add =  Object{memory.symbol("+")};
  const auto sub =  Object{memory.symbol("-")};
  const auto mult = Object{memory.symbol("*")};
  const auto div =  Object{memory.symbol("/")};
  const auto eq =   Object{memory.symbol("=")};

  const auto cons = Object{memory.symbol("cons")};
  const auto car = Object{memory.symbol("car")};
  const auto cdr = Object{memory.symbol("cdr")};
  
  const auto quote = Object{memory.symbol("quote")};
}

// struct Function {
//   Code* code;
// }

bool Object::is_nil() const {
  return *this == SC::nil;
}

const Object& Object::car() const {
  if (is_cons()) {
    return as_cons()->car;
  }
  throw std::runtime_error("type error");
}

const Object& Object::cdr() const {
  if (is_cons()) {
    return as_cons()->cdr;
  }
  throw std::runtime_error("type error");
}

std::ostream& operator<<(std::ostream& os, const Object& o) {
  if (o.is_number()) {
    os << o.as_number();
  } else if (o.is_symbol()) {
    os << *o.as_symbol();
  } else if (o.is_cons()) {
    os << "(";
    auto curr_cons = o.as_cons();
    for (;;) {
      os << curr_cons->car;
      auto& cdr = curr_cons->cdr;
      if (cdr.is_cons()) {
	curr_cons = cdr.as_cons();
	os << " ";
      }	else if (cdr.is_nil()) {
	break;
      } else {
	os << " . ";
	os << cdr;
	break;	
      }
    }
    os << ")";
  }
  return os;
}
