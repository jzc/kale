#include <iostream>
#include <memory>
// #include <utility>
int main () {
  auto x = std::make_unique<int>(1);
  auto y = std::make_unique<int>(2);
  std::swap(x, y);
}
