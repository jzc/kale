#include <iostream>
#include <functional>

class Foo {
public:
  enum test {a};
};

int main () {
  
  Foo::a;
}

// struct Foo {
//   int n;
//   Foo() { std::cout << "foo default constructed\n"; }
//   Foo(int n) : n{n} { std::cout << "foo with " << n << " constructed\n"; }
//   Foo(const Foo& f) : n{f.n} { std::cout << "foo copy with " << f.n << "\n"; }
// };

// struct Bar {
//   Foo f1;
//   Foo f2{f1};
//   Bar() : f1{6} {}
// };

// int main() {
//   int i = 0;
//   long l = 1;
//   std::variant<int, long> x{i};
//   std::cout << std::holds_alternative<long>(x) << "\n";
// }
