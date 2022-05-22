#include <iostream>
#include <functional>

class Visitor;

class Abstract {
public:
  virtual void accept(Visitor& v) = 0;
  virtual ~Abstract() {}
};

class Foo;
class Bar;

class Visitor {
public:
  virtual void operator()(Foo& f) = 0;
  virtual void operator()(Bar& b) = 0;
  virtual ~Visitor() {}
};



class Foo : public Abstract {
  void accept(Visitor& v) {
    v(*this);
  }
};
class Bar : public Abstract {
  void accept(Visitor& v) {
    v(*this);
  }
};

class Baz : public Visitor {
  void operator()(Foo&) override {
    std::cout << "foo\n";
  }
  void operator()(Bar&) override {
    std::cout << "bar\n";
  }  
}; 

int main () {
  Foo f;
  Abstract& a = f;
  Baz b;
  a.accept(b);
}
