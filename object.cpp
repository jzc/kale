#include <stdexcept>
#include <cstring>
#include "decls.hpp"

Memory memory {};

namespace Constants {
  const Object nil = Object{memory.symbol("nil")};
  const Object if_ = Object{memory.symbol("if")};
  const Object let = Object{memory.symbol("let")};
  const Object letrec = Object{memory.symbol("letrec")};
  const Object quote = Object{memory.symbol("quote")};
  const Object cons = Object{memory.symbol("cons")};
}

void type_error() {
  throw std::runtime_error("type error");
}

template <typename S, typename T>
S bitcast(T& t) {
  static_assert(sizeof(S) == sizeof(T));
  S s;
  std::memcpy(&s, &t, sizeof s);
  return s;
}

Object::Object(double d)
  : tag{tag_number},
    data{bitcast<std::uint64_t>(d)}
{}

Object::Object(Symbol s)
  : tag{tag_symbol},
    data{bitcast<std::uint64_t>(s)}
{}

Object::Object(Cons c)
  : tag{tag_cons},
    data{bitcast<std::uint64_t>(c)}
{}

bool Object::is_number() const { return tag == tag_number; }

bool Object::is_symbol() const { return tag == tag_symbol; }

bool Object::is_cons() const { return tag == tag_cons; }

double Object::as_number() const {
  if (!is_number()) type_error();
  return bitcast<double>(data);
}
Symbol Object::as_symbol() const {
  if (!is_symbol()) type_error();
  return bitcast<Symbol>(data);
}
Cons Object::as_cons() const {
  if (!is_cons()) type_error();
  return bitcast<Cons>(data);
}
Object& Object::car() const { return as_cons()->car; }

Object& Object::cdr() const { return as_cons()->cdr; }

bool Object::is_nil() const { return *this == Constants::nil; }

bool operator==(const Object& o1, const Object& o2) {
  return o1.tag == o2.tag && o1.data == o2.data;
}

bool Object::equal(const Object& rhs) const {
    if (tag != rhs.tag) {
      return false;
    }
    switch (tag) {
    case tag_number:
    case tag_symbol:
      return data == rhs.data;
    case tag_cons:
      return car().equal(rhs.car())
	&& cdr().equal(rhs.cdr());
    }
  }

Cons Memory::cons(Object car, Object cdr) {
  conses.emplace_back(car, cdr);
  return &conses.back();
}

Symbol Memory::symbol(const std::string& s) {
  auto [it, did_insert] = symbol_lookup.insert({s, nullptr});
  if (did_insert) {
    symbol_storage.push_back(s);
    it->second = &symbol_storage.back();
  }
  return it->second;
}

std::ostream& operator<<(std::ostream& os, Object o) {
  switch (o.tag) {
  case Object::tag_number:
    os << o.as_number();
    break;
  case Object::tag_symbol:
    os << *o.as_symbol();
    break;
  case Object::tag_cons: {
    os << "(";
    for (auto p = o; p.is_cons(); p = p.cdr()) {
      os << p.car();
      auto& cdr = p.cdr();
      if (cdr.is_nil()) {
	os << ")";
      } else if (!cdr.is_cons()) {
	os << " . " << cdr << ")";
      } else {
	os << " ";
      }
    }
    break;      
  }
  }
  return os;
}

extern "C" { 
  void _add(Object* out, Object* o1, Object* o2) {
    if (!o1->is_number() || !o2->is_number())
      type_error();

    *out = Object{o1->as_number() + o2->as_number()};
  }

  void _sub(Object* out, Object* o1, Object* o2) {
    if (!o1->is_number() || !o2->is_number())
      type_error();

    *out = Object{o1->as_number() - o2->as_number()};
  }

  void _mult(Object* out, Object* o1, Object* o2) {
    if (!o1->is_number() || !o2->is_number())
      type_error();

    *out = Object{o1->as_number() * o2->as_number()};
  }

  void __div(Object* out, Object* o1, Object* o2) {
    if (!o1->is_number() || !o2->is_number())
      type_error();

    *out = Object{o1->as_number() / o2->as_number()};
  }

  void _cons(Object* out, Object* o1, Object* o2) {
    *out = Object{memory.cons(*o1, *o2)};
  }

  void _make_number(Object* out, double d) {
    *out = Object{d};
  }

  void _make_symbol(Object* out, const char* data) {
    *out = Object{memory.symbol(data)};
  }

  bool is_nil(Object* o1) {
    return o1->is_nil();
  }

  void _print(Object* out, Object* o1) {
    std::cout << *o1 << "\n";
    *out = Constants::nil;
  }

  void _equal(Object* out, const Object* o1, const Object* o2) {
    *out = o1->equal(*o2) ? Constants::t : Constants::nil;
  }
}
