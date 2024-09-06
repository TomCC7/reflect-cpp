#ifndef RFL_YAML_READER_HPP_
#define RFL_YAML_READER_HPP_

#include <array>
#include <exception>
#include <map>
#include <memory>
#include <ryml.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "../Result.hpp"
#include "../always_false.hpp"

namespace rfl {
namespace yaml {

struct Reader {
  struct YAMLInputArray {
    YAMLInputArray(const ryml::ConstNodeRef& _node) : node_(_node) {}
    ryml::ConstNodeRef node_;
  };

  struct YAMLInputObject {
    YAMLInputObject(const ryml::ConstNodeRef& _node) : node_(_node) {}
    ryml::ConstNodeRef node_;
  };

  struct YAMLInputVar {
    YAMLInputVar(const ryml::ConstNodeRef& _node) : node_(_node) {}
    ryml::ConstNodeRef node_;
  };

  using InputArrayType = YAMLInputArray;
  using InputObjectType = YAMLInputObject;
  using InputVarType = YAMLInputVar;

  template <class T>
  static constexpr bool has_custom_constructor = (requires(InputVarType var) {
    T::from_yaml_obj(var);
  });

  rfl::Result<InputVarType> get_field_from_array(
      const size_t _idx, const InputArrayType& _arr) const noexcept {
    if (_idx >= _arr.node_.num_children()) {
      return rfl::Error("Index " + std::to_string(_idx) + " of of bounds.");
    }
    return InputVarType(_arr.node_[_idx]);
  }

  rfl::Result<InputVarType> get_field_from_object(
      const std::string& _name, const InputObjectType& _obj) const noexcept {
    auto var = InputVarType(_obj.node_.at(_name));
    if (var.node_.is_seed()) {
      return rfl::Error("Object contains no field named '" + _name + "'.");
    }
    return var;
  }

  bool is_empty(const InputVarType& _var) const noexcept {
    return _var.node_.has_val() && _var.node_.val_is_null();
  }

  template <class T>
  rfl::Result<T> to_basic_type(const InputVarType& _var) const noexcept {
    try {
      if constexpr (std::is_same<std::remove_cvref_t<T>, std::string>() ||
                    std::is_same<std::remove_cvref_t<T>, bool>() ||
                    std::is_floating_point<std::remove_cvref_t<T>>() ||
                    std::is_integral<std::remove_cvref_t<T>>()) {
        T val;
        _var.node_ >> val;
        return val;
      } else {
        static_assert(rfl::always_false_v<T>, "Unsupported type.");
      }
    } catch (std::exception& e) {
      return rfl::Error(e.what());
    }
  }

  template <class ArrayReader>
  std::optional<Error> read_array(const ArrayReader& _array_reader,
                                  const InputArrayType& _arr) const noexcept {
    for (const ryml::ConstNodeRef& node : _arr.node_.children()) {
      const auto err = _array_reader.read(YAMLInputVar(node));
      if (err) {
        return err;
      }
    }
    return std::nullopt;
  }

  template <class ObjectReader>
  std::optional<Error> read_object(const ObjectReader& _object_reader,
                                   const InputObjectType& _obj) const noexcept {
    for (const ryml::ConstNodeRef& node : _obj.node_.children()) {
      _object_reader.read(std::string_view(node.key()), InputVarType(node));
    }
    return std::nullopt;
  }

  rfl::Result<InputArrayType> to_array(
      const InputVarType& _var) const noexcept {
    if (!_var.node_.is_seq()) {
      return rfl::Error("Could not cast to sequence!");
    }
    return InputArrayType(_var.node_);
  }

  rfl::Result<InputObjectType> to_object(
      const InputVarType& _var) const noexcept {
    if (!_var.node_.is_map()) {
      return rfl::Error("Could not cast to map!");
    }
    return InputObjectType(_var.node_);
  }

  template <class T>
  rfl::Result<T> use_custom_constructor(
      const InputVarType _var) const noexcept {
    try {
      return T::from_yaml_obj(_var);
    } catch (std::exception& e) {
      return rfl::Error(e.what());
    }
  }
};

}  // namespace yaml
}  // namespace rfl

#endif
