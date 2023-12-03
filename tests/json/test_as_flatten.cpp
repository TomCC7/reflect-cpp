#include "test_as_flatten.hpp"

#include <cassert>
#include <iostream>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <source_location>
#include <string>
#include <vector>

#include "write_and_read.hpp"

namespace test_as_flatten {

struct A {
  rfl::Field<"f1", std::string> f1;
  rfl::Field<"f2", rfl::Box<std::string>> f2;
};

struct B {
  rfl::Field<"f3", std::string> f3;
  rfl::Field<"f4", rfl::Box<std::string>> f4;
};

struct C {
  rfl::Flatten<A> a;
  rfl::Flatten<B> b;
  rfl::Field<"f5", int> f5;
};

void test() {
  std::cout << std::source_location::current().function_name() << std::endl;

  auto a = A{.f1 = "Hello", .f2 = rfl::make_box<std::string>("World")};

  auto b = B{.f3 = "Hello", .f4 = rfl::make_box<std::string>("World")};

  const auto c =
      rfl::as<C>(std::move(a), std::move(b), rfl::make_field<"f5">(5));

  write_and_read(
      c, R"({"f1":"Hello","f2":"World","f3":"Hello","f4":"World","f5":5})");
}
}  // namespace test_as_flatten
