#ifndef RFL_CBOR_WRITER_HPP_
#define RFL_CBOR_WRITER_HPP_

#include <cbor.h>

#include <exception>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include "../Box.hpp"
#include "../Ref.hpp"
#include "../Result.hpp"
#include "../always_false.hpp"

namespace rfl::cbor {

class Writer {
 public:
  struct CBOROutputArray {
    cbor_item_t* val_;
  };

  struct CBOROutputObject {
    cbor_item_t* val_;
  };

  struct CBOROutputVar {
    cbor_item_t* val_;
  };

  using OutputArrayType = CBOROutputArray;
  using OutputObjectType = CBOROutputObject;
  using OutputVarType = CBOROutputVar;

  Writer(cbor_item_t** _root) : root_(_root) {}

  ~Writer() = default;

  OutputArrayType array_as_root(const size_t _size) const noexcept {
    *root_ = new_array(_size);
    return OutputArrayType{*root_};
  }

  OutputObjectType object_as_root(const size_t _size) const noexcept {
    *root_ = new_object(_size).val_;
    return OutputObjectType{*root_};
  }

  OutputVarType null_as_root() const noexcept {
    *root_ = new_null().val_;
    return OutputVarType{*root_};
  }

  template <class T>
  OutputVarType value_as_root(const T& _var) const noexcept {
    *root_ = new_value(_var);
    return OutputVarType{*root_};
  }

  OutputArrayType add_array_to_array(const size_t _size,
                                     OutputArrayType* _parent) const noexcept {
    auto arr = new_array(_size);
    cbor_array_push(_parent->val_, cbor_move(arr.val_));
    return arr;
  }

  OutputArrayType add_array_to_object(
      const std::string_view& _name, const size_t _size,
      OutputObjectType* _parent) const noexcept {
    auto arr = new_array(_size);
    cbor_map_add(_parent->val_, cbor_pair{.key = cbor_move(cbor_build_string(
                                              std::string(_name).c_str())),
                                          .value = cbor_move(arr.val_)});
    return arr;
  }

  OutputObjectType add_object_to_array(
      const size_t _size, OutputArrayType* _parent) const noexcept {
    auto obj = new_object(_size);
    cbor_array_push(_parent->val_, cbor_move(obj.val_));
    return obj;
  }

  OutputObjectType add_object_to_object(
      const std::string_view& _name, const size_t _size,
      OutputObjectType* _parent) const noexcept {
    auto obj = new_object(_size);
    cbor_map_add(_parent->val_, cbor_pair{.key = cbor_move(cbor_build_string(
                                              std::string(_name).c_str())),
                                          .value = cbor_move(obj.val_)});
    return obj;
  }

  template <class T>
  OutputVarType add_value_to_array(const T& _var,
                                   OutputArrayType* _parent) const noexcept {
    auto val = new_value(_var);
    cbor_array_push(_parent->val_, cbor_move(val.val_));
    return val;
  }

  template <class T>
  OutputVarType add_value_to_object(const std::string_view& _name,
                                    const T& _var,
                                    OutputObjectType* _parent) const noexcept {
    auto val = new_value(_var);
    cbor_map_add(_parent->val_, cbor_pair{.key = cbor_move(cbor_build_string(
                                              std::string(_name).c_str())),
                                          .value = cbor_move(obj.val_)});
    return val;
  }

  OutputVarType add_null_to_array(OutputArrayType* _parent) const noexcept {
    auto val = new_null();
    cbor_array_push(_parent->val_, cbor_move(val.val_));
    return val;
  }

  OutputVarType add_null_to_object(const std::string_view& _name,
                                   OutputObjectType* _parent) const noexcept {
    auto val = new_null();
    cbor_map_add(_parent->val_, cbor_pair{.key = cbor_move(cbor_build_string(
                                              std::string(_name).c_str())),
                                          .value = cbor_move(obj.val_)});
    return val;
  }

  void end_array(OutputArrayType* _arr) const noexcept {}

  void end_object(OutputObjectType* _obj) const noexcept {}

 private:
  OutputArrayType new_array(const size_t _size) const noexcept {
    return OutputArrayType{cbor_new_definite_array(_size)};
  }

  OutputObjectType new_object(const size_t _size) const noexcept {
    return OutputObjectType{cbor_new_definite_map(_size)};
  }

  OutputVarType new_null() const noexcept {
    return OutputVarType{cbor_new_null()};
  }

  template <class T>
  OutputVarType new_value(const T& _var) const noexcept {
    if constexpr (std::is_same<std::remove_cvref_t<T>, std::string>()) {
      return OutputVarType{cbor_build_string(_var.c_str())};
    } else if constexpr (std::is_same<std::remove_cvref_t<T>, bool>()) {
      return OutputVarType{cbor_build_null(_var)};
    } else if constexpr (std::is_floating_point<std::remove_cvref_t<T>>()) {
      return OutputVarType{cbor_build_float8(_var)};
    } else if constexpr (std::is_unsigned<std::remove_cvref_t<T>>()) {
      return OutputVarType{cbor_build_uint32(static_cast<uint32_t>(_var)};
    } else if constexpr (std::is_integral<std::remove_cvref_t<T>>()) {
      // Refer to
      // https://libcbor.readthedocs.io/en/latest/api/type_0_1_integers.html#type-1-negative-integers
      // for an explanation.
      auto val = OutputVarType{cbor_new_int32()};
      if (_var < 0) {
        cbor_mark_negint(val.val_);
        cbor_set_uint32(val.val_, static_cast<uint32_t>(_var * (-1) - 1));
      } else {
        cbor_set_uint32(val.val_, static_cast<uint32_t>(_var));
      }
    } else {
      static_assert(rfl::always_false_v<T>, "Unsupported type.");
    }
  }

 private:
  cbor_item_t** root_;
};

}  // namespace rfl::cbor

#endif  // CBOR_PARSER_HPP_
