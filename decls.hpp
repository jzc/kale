#include <string>
#include <cstdint>
#include <iostream>
#include <list>
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>

struct Cell;
struct ClosureData;
using Cons = Cell*;
using Closure = ClosureData*;
using Symbol = const std::string*;

class Object {
private:
  enum Tag {
    tag_number,
    tag_symbol,
    tag_cons,
    tag_closure,
  };
  
  std::uint64_t tag;
  std::uint64_t data;  
public:
  explicit Object(double d);
  explicit Object(Symbol s);
  explicit Object(Cons c);
  explicit Object(Closure c);
  bool is_number() const;
  bool is_symbol() const;
  bool is_cons() const;
  bool is_closure() const;
  double as_number() const;
  Symbol as_symbol() const;
  Cons as_cons() const;
  Closure as_closure() const;
  Object& car() const;
  Object& cdr() const;
  bool is_nil() const;
  friend bool operator==(const Object& o1, const Object& o2);
  friend bool operator!=(const Object& o1, const Object& o2)
  { return !(o1 == o2); }
  friend std::ostream& operator<<(std::ostream& os, Object o);
  bool equal(const Object& rhs) const;
};

struct Cell {
  Object car;
  Object cdr;
  Cell(Object car, Object cdr)
    : car{car}, cdr{cdr}
  {}
};

struct ClosureData {
  std::vector<Object> fvs;
  void* code;
  int n_params;
};

struct Memory {
  std::list<Cell> conses {};
  std::list<std::string> symbol_storage {};
  std::unordered_map<std::string, const std::string*>
  symbol_lookup {};

  Cons cons(Object car, Object cdr);
  Symbol symbol(const std::string& s); 
};

extern Memory memory;

namespace Constants {
  extern const Object nil; 
  extern const Object if_; 
  extern const Object let; 
  extern const Object letrec;
  extern const Object quote; 
  extern const Object cons;
  extern const Object t;
  extern const Object lambda;
}

extern "C" { 
  void _add(Object* out, Object* o1, Object* o2);
  void _sub(Object* out, Object* o1, Object* o2);
  void _mult(Object* out, Object* o1, Object* o2);
  void __div(Object* out, Object* o1, Object* o2);
  void _cons(Object* out, Object* o1, Object* o2);
  void _make_number(Object* out, double d);
  void _make_symbol(Object* out, const char* data);
  bool _is_nil(Object* o1);
  void _print(Object* out, Object* o1);
  void _equal(Object* out, Object* o1, Object *o2);
  void* _get_code(Object* o1, int n);
  Object* _get_fvs(Object* o1);
}

enum class Token {
  lparen, rparen, dot, number, symbol, eof,
  quote
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

class Reader {
private:
  Tokenizer t;
public:
  Reader(std::istream& is);
  Object read();
};

class FormVisitor;
class Form {
public:
  virtual void accept(FormVisitor& visitor) = 0;
  virtual ~Form() {}
};

class NumberForm;
class SymbolForm;
class IfForm;
class LetForm;
class LetrecForm;
class QuoteForm;
class ApplicationForm;
class LambdaForm;

class FormVisitor {
public:
  virtual void operator()(NumberForm&) = 0;
  virtual void operator()(SymbolForm&) = 0;
  virtual void operator()(IfForm&) = 0;
  virtual void operator()(LetForm&) = 0;
  virtual void operator()(LetrecForm&) = 0;
  virtual void operator()(QuoteForm&) = 0;
  virtual void operator()(ApplicationForm&) = 0;
  virtual void operator()(LambdaForm&) = 0;
  virtual ~FormVisitor() {};
};

class NumberForm : public Form {
public:
  double number;
  NumberForm(double number) : number{number} {}
  void accept(FormVisitor& visitor) override {
    visitor(*this);
  }
};

class SymbolForm : public Form {
public:
  Symbol symbol;
  SymbolForm(Symbol symbol) : symbol{symbol} {}
  void accept(FormVisitor& visitor) override {
    visitor(*this);
  }
};

class IfForm: public Form {
public:
  std::unique_ptr<Form> cond_form;
  std::unique_ptr<Form> then_form;
  std::unique_ptr<Form> else_form;
  IfForm(std::unique_ptr<Form>&& cond_form,
	 std::unique_ptr<Form>&& then_form,
	 std::unique_ptr<Form>&& else_form)
    : cond_form{std::move(cond_form)},
      then_form{std::move(then_form)},
      else_form{std::move(else_form)}
  {}
  void accept(FormVisitor& visitor) override {
    visitor(*this);
  }
};

struct VariableBinding {
  Symbol binder;
  std::unique_ptr<Form> definition;
  VariableBinding(Symbol binder,
		  std::unique_ptr<Form>&& definition)
    : binder{binder},
      definition{std::move(definition)}
  {}
};

class LetForm : public Form {
public:
  std::vector<VariableBinding> bindings;
  std::unique_ptr<Form> body;
  LetForm(std::vector<VariableBinding>&& bindings,
	  std::unique_ptr<Form>&& body)
    : bindings{std::move(bindings)},
      body{std::move(body)}
  {}
  void accept(FormVisitor& visitor) override {
    visitor(*this);
  }
};

struct FunctionBinding {
  Symbol binder;
  std::vector<Symbol> parameters;
  std::unique_ptr<Form> definition;
  FunctionBinding(Symbol binder,
		  std::vector<Symbol>&& parameters,
		  std::unique_ptr<Form>&& definition)
    : binder{binder},
      parameters{std::move(parameters)},
      definition{std::move(definition)}
  {}
};

class LetrecForm : public Form {
public:
  std::vector<FunctionBinding> bindings;
  std::unique_ptr<Form> body;
  LetrecForm(std::vector<FunctionBinding>&& bindings,
	     std::unique_ptr<Form>&& body)
    : bindings{std::move(bindings)},
      body{std::move(body)}
  {}
  void accept(FormVisitor& visitor) override {
    visitor(*this);
  }      
};

class QuoteForm : public Form {
public:
  Object arg;
  QuoteForm(Object arg) : arg{arg} {};
  void accept(FormVisitor& visitor) override {
    visitor(*this);
  }
};

class ApplicationForm : public Form {
public:
  std::unique_ptr<Form> function_form;
  std::vector<std::unique_ptr<Form>> arg_forms;
  ApplicationForm(std::unique_ptr<Form>&& function_form,
		  std::vector<std::unique_ptr<Form>>&& arg_forms)
    : function_form{std::move(function_form)},
      arg_forms{std::move(arg_forms)}
  {}
  void accept(FormVisitor& visitor) override {
    visitor(*this);
  }
};

class LambdaForm : public Form {
public:
  std::vector<Symbol> parameters;
  std::unique_ptr<Form> body;
  LambdaForm(std::vector<Symbol>&& parameters,
	     std::unique_ptr<Form>&& body)
    : parameters{std::move(parameters)},
      body{std::move(body)}
  {}
  void accept(FormVisitor& visitor) override {
    visitor(*this);
  }
};

struct Parser {
  static std::unique_ptr<Form> parse(const Object& o);
  static std::unique_ptr<Form> parse_if(const Object& o);
  static std::unique_ptr<Form> parse_let(const Object& o);
  static std::unique_ptr<Form> parse_letrec(const Object& o);
  static std::unique_ptr<Form> parse_quote(const Object& o);
  static std::unique_ptr<Form> parse_application(const Object& o);
  static std::unique_ptr<Form> parse_lambda(const Object& o);
};
