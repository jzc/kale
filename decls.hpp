#include <string>
#include <cstdint>
#include <iostream>
#include <list>
#include <unordered_map>

struct Cell;
using Cons = Cell*;
using Symbol = const std::string*;

class Object {
private:
  enum Tag {
    tag_number,
    tag_symbol,
    tag_cons
  };
  
  std::uint64_t tag;
  std::uint64_t data;  
public:
  explicit Object(double d);
  explicit Object(Symbol s);
  explicit Object(Cons c);
  bool is_number() const;
  bool is_symbol() const;
  bool is_cons() const;
  double as_number() const;
  Symbol as_symbol() const;
  Cons as_cons() const;
  Object& car() const;
  Object& cdr() const;
  bool is_nil() const;
  friend bool operator==(const Object& o1, const Object& o2);
  friend std::ostream& operator<<(std::ostream& os, Object o); 
};

struct Cell {
  Object car;
  Object cdr;
  Cell(Object car, Object cdr)
    : car{car}, cdr{cdr}
  {}
};

struct Memory {
  std::list<Cell> conses {};
  std::list<std::string> symbol_storage {};
  std::unordered_map<std::string, const std::string*>
  symbol_lookup {};

  Cons cons(Object car, Object cdr);
  Symbol symbol(const std::string& s); 
};

Memory memory {};

namespace Constants {
  const Object nil = Object{memory.symbol("nil")};
}

extern "C" { 
  void _add(Object* out, Object* o1, Object* o2);
  void _sub(Object* out, Object* o1, Object* o2);
  void _mult(Object* out, Object* o1, Object* o2);
  void __div(Object* out, Object* o1, Object* o2);
  void _cons(Object* out, Object* o1, Object* o2);
  void _make_number(Object* out, double d);
  bool is_nil(Object* o1);
  void _print(Object* out, Object* o1);
}

enum class Token {
  lparen, rparen, dot, number, symbol, eof
};

class Tokenizer {
private:
  std::istream& is;
  Token last_token {};
  bool did_peek {false};
public:
  double number_data {};
  std::string symbol_data {};
  
  Tokenizer(std::istream& is); 
  Token peek();  
  Token read_token();
};

class Parser {
private:
  Tokenizer t;
public:
  Parser(std::istream& is);
  Object parse();
};
