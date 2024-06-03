#ifndef RFL_CBOR_WRITE_HPP_
#define RFL_CBOR_WRITE_HPP_

#include <cbor.h>

#include <cstdint>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>

#include "../parsing/Parent.hpp"
#include "Parser.hpp"

namespace rfl {
namespace cbor {

/// Returns CBOR bytes.
template <class... Ps>
std::vector<char> write(const auto& _obj) noexcept {
  using T = std::remove_cvref_t<decltype(_obj)>;
  using ParentType = parsing::Parent<Writer>;
  cbor_item_t* root;
  const auto writer = Writer(&root);
  Parser<T, Processors<Ps...>>::write(writer, _obj,
                                      typename ParentType::Root{});
  unsigned char* buffer;
  size_t buffer_size;
  cbor_serialize_alloc(root, &buffer, &buffer_size);
  cbor_decref(&root);
  auto vec = std::vector<char>(reinterpret_cast<char*>(buffer),
                               reinterpret_cast<char*>(buffer) + buffer_size);
  free(buffer);
  return vec;
}

/// Writes a CBOR into an ostream.
template <class... Ps>
std::ostream& write(const auto& _obj, std::ostream& _stream) noexcept {
  auto buffer = write<Ps...>(_obj);
  _stream.write(buffer.data(), buffer.size());
  return _stream;
}

}  // namespace cbor
}  // namespace rfl

#endif
