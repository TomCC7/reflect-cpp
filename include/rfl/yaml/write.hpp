#ifndef RFL_YAML_WRITE_HPP_
#define RFL_YAML_WRITE_HPP_

#include <ostream>
#include <ryml/ryml.hpp>
#include <sstream>
#include <string>
#include <type_traits>

#include "../Processors.hpp"
#include "../parsing/Parent.hpp"
#include "Parser.hpp"

namespace rfl {
namespace yaml {

/// Writes a YAML into an ostream.
template <class... Ps>
std::ostream& write(const auto& _obj, std::ostream& _stream) {
  using T = std::remove_cvref_t<decltype(_obj)>;
  using ParentType = parsing::Parent<Writer>;
  const auto tree = Ref<ryml::Tree>::make();
  auto w = Writer(tree);
  using ProcessorsType = Processors<Ps...>;
  static_assert(!ProcessorsType::no_field_names_,
                "The NoFieldNames processor is not supported for BSON, XML, "
                "TOML, or YAML.");
  Parser<T, ProcessorsType>::write(w, _obj, typename ParentType::Root{});
  _stream << *tree;
  return _stream;
}

/// Returns a YAML string.
template <class... Ps>
std::string write(const auto& _obj) {
  using T = std::remove_cvref_t<decltype(_obj)>;
  using ParentType = parsing::Parent<Writer>;
  const auto tree = Ref<ryml::Tree>::make();
  auto w = Writer(tree);
  using ProcessorsType = Processors<Ps...>;
  static_assert(!ProcessorsType::no_field_names_,
                "The NoFieldNames processor is not supported for BSON, XML, "
                "TOML, or YAML.");
  Parser<T, ProcessorsType>::write(w, _obj, typename ParentType::Root{});
  return ryml::emitrs_yaml<std::string>(*tree);
}

}  // namespace yaml
}  // namespace rfl

#endif
