#include <gtest/gtest.h>

#include <cassert>
#include <iostream>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <source_location>
#include <string>
#include <vector>

#include "write_and_read.hpp"

namespace test_enum8 {

struct SomeClass {
  enum class hass : uint8_t { j1 = 0, j2 = 1, j3 = 2, j4 = 3 };
  std::string name{"foo"};
  std::string last_name{"bar"};
  uint64_t age{0};
  hass has{hass::j1};
  bool valid{false};
};

TEST(json, test_enum8) {
  const auto some_class = SomeClass{};
  write_and_read(
      some_class,
      R"({"name":"foo","last_name":"bar","age":0,"has":"j1","valid":false})");
}
}  // namespace test_enum8
