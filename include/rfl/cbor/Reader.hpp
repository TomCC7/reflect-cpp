#ifndef RFL_CBOR_READER_HPP_
#define RFL_CBOR_READER_HPP_

#include <cbor.h>

#include <array>
#include <concepts>
#include <exception>
#include <map>
#include <memory>
#include <source_location>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "../Box.hpp"
#include "../Result.hpp"
#include "../always_false.hpp"

namespace rfl ::cbor {

/// Please refer to https://intel.github.io/tinycbor/current/index.html
struct Reader {
  struct CBORInputArray {
    cbor_item_t* val_;
  };

  struct CBORInputObject {
    cbor_item_t* val_;
  };

  struct CBORInputVar {
    cbor_item_t* val_;
  };

  using InputArrayType = CBORInputArray;
  using InputObjectType = CBORInputObject;
  using InputVarType = CBORInputVar;

  template <class T>
  static constexpr bool has_custom_constructor = (requires(InputVarType var) {
    T::from_cbor_obj(var);
  });

  rfl::Result<InputVarType> get_field(
      const std::string& _name, const InputObjectType& _obj) const noexcept {
    const size_t length = cbor_map_size(_obj.val_);
    auto it = cbor_map_handle(_obj.val_);
    for (size_t i = 0; i < length; ++i, ++it) {
      if (!cbor_isa_string(it->key)) {
        return Error("Expected the key to be a string.");
      }
      const auto name = to_string_view(it->key);
      if (_name == name) {
        return InputVarType{it->value};
      }
    }
    return Error("Field named '" + _name + "' not found.");
  }

  bool is_empty(const InputVarType& _var) const noexcept {
    return cbor_is_null(_var.val_);
  }

  template <class T>
  rfl::Result<T> to_basic_type(const InputVarType& _var) const noexcept {
    if constexpr (std::is_same<std::remove_cvref_t<T>, std::string>()) {
      if (!cbor_isa_string(_var.val_)) {
        return Error("Could not cast to string.");
      }
      return std::string(to_string_view(_var.val_));
    } else if constexpr (std::is_same<std::remove_cvref_t<T>, bool>()) {
      if (!cbor_is_bool(_var.val_)) {
        return rfl::Error("Could not cast to boolean.");
      }
      return cbor_get_bool(_var.val_);
    } else if constexpr (std::is_floating_point<std::remove_cvref_t<T>>()) {
      if (!cbor_is_float(_var.val_)) {
        return rfl::Error("Could not cast to boolean.");
      }
      return static_cast<T>(cbor_float_get_float(_var.val_));
    } else if constexpr (std::is_unsigned<std::remove_cvref_t<T>>()) {
      const auto width = cbor_int_get_width(_var.val_);
      switch (width) {
        case CBOR_INT_8:
          return static_cast<T>(cbor_get_uint8(_var.val_));
        case CBOR_INT_16:
          return static_cast<T>(cbor_get_uint16(_var.val_));
        case CBOR_INT_32:
          return static_cast<T>(cbor_get_uint32(_var.val_));
        case CBOR_INT_64:
          return static_cast<T>(cbor_get_uint64(_var.val_));
        default:
          return rfl::Error("Unknown width.");
      }
    } else if constexpr (std::is_integral<std::remove_cvref_t<T>>()) {
      // Refer to
      // https://libcbor.readthedocs.io/en/latest/api/type_0_1_integers.html#type-1-negative-integers
      // for an explanation.
      const bool is_neg = cbor_isa_negint(_var.val_);
      const auto width = cbor_int_get_width(_var.val_);
      switch (width) {
        case CBOR_INT_8:
          return is_neg ? static_cast<T>(cbor_get_uint8(_var.val_)) * (-1) - 1
                        : static_cast<T>(cbor_get_uint8(_var.val_));
        case CBOR_INT_16:
          return is_neg ? static_cast<T>(cbor_get_uint16(_var.val_)) * (-1) - 1
                        : static_cast<T>(cbor_get_uint16(_var.val_));
        case CBOR_INT_32:
          return is_neg ? static_cast<T>(cbor_get_uint32(_var.val_)) * (-1) - 1
                        : static_cast<T>(cbor_get_uint32(_var.val_));
        case CBOR_INT_64:
          return is_neg ? static_cast<T>(cbor_get_uint64(_var.val_)) * (-1) - 1
                        : static_cast<T>(cbor_get_uint64(_var.val_));
        default:
          return rfl::Error("Unknown width.");
      }
    } else {
      static_assert(rfl::always_false_v<T>, "Unsupported type.");
    }
  }

  rfl::Result<InputArrayType> to_array(
      const InputVarType& _var) const noexcept {
    if (!cbor_isa_array(_var.val_)) {
      return Error("Could not cast to an array.");
    }
    return InputArrayType{_var.val_};
  }

  rfl::Result<InputObjectType> to_object(
      const InputVarType& _var) const noexcept {
    if (!cbor_isa_map(_var.val_)) {
      return Error("Could not cast to a map.");
    }
    return InputObjectType{_var.val_};
  }

  template <class ArrayReader>
  std::optional<Error> read_array(const ArrayReader& _array_reader,
                                  const InputArrayType& _arr) const noexcept {
    const size_t length = cbor_array_size(_arr.val_);
    auto it = cbor_array_handle(_arr.val_);
    for (size_t i = 0; i < length; ++i, ++it) {
      const auto err = _array_reader.read(InputVarType{*it});
      if (err) {
        return err;
      }
    }
    return std::nullopt;
  }

  template <class ObjectReader>
  std::optional<Error> read_object(const ObjectReader& _object_reader,
                                   const InputObjectType& _obj) const noexcept {
    const size_t length = cbor_map_size(_obj.val_);
    auto it = cbor_map_handle(_obj.val_);
    for (size_t i = 0; i < length; ++i, ++it) {
      if (!cbor_isa_string(it->key)) {
        return Error("Expected the key to be a string.");
      }
      const auto name = to_string_view(it->key);
      _object_reader.read(name, InputVarType{it->value});
    }
    return std::nullopt;
  }

  template <class T>
  rfl::Result<T> use_custom_constructor(
      const InputVarType& _var) const noexcept {
    try {
      return T::from_cbor_obj(_var);
    } catch (std::exception& e) {
      return rfl::Error(e.what());
    }
  }

 private:
  std::string_view to_string_view(const cbor_item_t* _item) const noexcept {
    const auto length = cbor_string_length(_item);
    return std::string_view(
        reinterpret_cast<const char*>(cbor_string_handle(_item)), length);
  }
};

}  // namespace rfl::cbor

#endif  // JSON_PARSER_HPP_
