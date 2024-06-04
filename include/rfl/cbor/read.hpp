#ifndef RFL_CBOR_READ_HPP_
#define RFL_CBOR_READ_HPP_

#include <cbor.h>

#include <istream>
#include <string>

#include "../Processors.hpp"
#include "../internal/wrap_in_rfl_array_t.hpp"
#include "Parser.hpp"
#include "Reader.hpp"

namespace rfl {
namespace cbor {

using InputObjectType = typename Reader::InputObjectType;
using InputVarType = typename Reader::InputVarType;

/// Parses an object from a CBOR var.
template <class T, class... Ps>
auto read(const InputVarType& _obj) {
  const auto r = Reader();
  return Parser<T, Processors<Ps...>>::read(r, _obj);
}

/// Parses an object from CBOR using reflection.
template <class T, class... Ps>
Result<internal::wrap_in_rfl_array_t<T>> read(const char* _bytes,
                                              const size_t _size) {
  cbor_load_result load_result;
  cbor_item_t* item = cbor_load(reinterpret_cast<const unsigned char*>(_bytes),
                                _size, &load_result);
  // TODO: Error handling
  auto result = read<T, Ps...>(InputVarType{item});
  cbor_decref(&item);
  return result;
}

/// Parses an object from CBOR using reflection.
template <class T, class... Ps>
auto read(const std::vector<char>& _bytes) {
  return read<T, Ps...>(_bytes.data(), _bytes.size());
}

/// Parses an object from a stream.
template <class T, class... Ps>
auto read(std::istream& _stream) {
  std::istreambuf_iterator<char> begin(_stream), end;
  auto bytes = std::vector<char>(begin, end);
  return read<T, Ps...>(bytes.data(), bytes.size());
}

}  // namespace cbor
}  // namespace rfl

#endif
