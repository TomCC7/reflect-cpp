#ifndef RFL_YAML_WRITER_HPP_
#define RFL_YAML_WRITER_HPP_

#include <exception>
#include <map>
#include <ryml/ryml.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "../Ref.hpp"
#include "../Result.hpp"
#include "../always_false.hpp"

namespace rfl {
namespace yaml {

class Writer {
 public:
  struct YAMLArray {
    YAMLArray(const ryml::NodeRef& _node) : node_(_node) {}
    ryml::NodeRef node_;
  };

  struct YAMLObject {
    YAMLObject(const ryml::NodeRef& _node) : node_(_node) {}
    ryml::NodeRef node_;
  };

  struct YAMLVar {
    YAMLVar(const ryml::NodeRef& _node) : node_(_node) {}
    ryml::NodeRef node_;
  };

  using OutputArrayType = YAMLArray;
  using OutputObjectType = YAMLObject;
  using OutputVarType = YAMLVar;

  Writer(const Ref<ryml::Tree>& _tree) : tree_(_tree) {}

  ~Writer() = default;

  OutputArrayType array_as_root(const size_t _size) const noexcept {
    return OutputArrayType(tree_->rootref());
  }

  OutputObjectType object_as_root(const size_t _size) const noexcept {
    return OutputObjectType(tree_->rootref());
  }

  OutputVarType null_as_root() const noexcept {
    return OutputVarType(tree_->rootref());
  }

  template <class T>
  OutputVarType value_as_root(const T& _var) const noexcept {
    return OutputVarType(tree_->rootref());
  }

  OutputArrayType add_array_to_array(const size_t _size,
                                     OutputArrayType* _parent) const noexcept {
    _parent->node_.append_child() |= ryml::SEQ;
    return OutputArrayType(_parent->node_.last_child());
  }

  OutputArrayType add_array_to_object(
      const std::string_view& _name, const size_t _size,
      OutputObjectType* _parent) const noexcept {
    auto buf = std::vector<char>(_name.begin(), _name.end());
    auto substr = c4::substr(buf.data(), buf.size());
    _parent->node_.append_child() << ryml::key(substr) |= ryml::SEQ;
    return OutputArrayType(_parent->node_.last_child());
  }

  OutputObjectType add_object_to_array(
      const size_t _size, OutputArrayType* _parent) const noexcept {
    _parent->node_.append_child() |= ryml::MAP;
    return OutputObjectType(_parent->node_.last_child());
  }

  OutputObjectType add_object_to_object(
      const std::string_view& _name, const size_t _size,
      OutputObjectType* _parent) const noexcept {
    auto buf = std::vector<char>(_name.begin(), _name.end());
    auto substr = c4::substr(buf.data(), buf.size());
    _parent->node_.append_child() << ryml::key(substr) |= ryml::MAP;
    return OutputObjectType(_parent->node_.last_child());
  }

  template <class T>
  OutputVarType add_value_to_array(const T& _var,
                                   OutputArrayType* _parent) const noexcept {
    _parent->node_[_parent->node_.num_children()] << _var;
    return OutputVarType(_parent->node_.last_child());
  }

  template <class T>
  OutputVarType add_value_to_object(const std::string_view& _name,
                                    const T& _var,
                                    OutputObjectType* _parent) const noexcept {
    _parent->node_[c4::csubstr(_name.data(), _name.size())] << _var;
    return OutputVarType(_parent->node_.last_child());
  }

  OutputVarType add_null_to_array(OutputArrayType* _parent) const noexcept {
    // TODO
  }

  OutputVarType add_null_to_object(const std::string_view& _name,
                                   OutputObjectType* _parent) const noexcept {
    // TODO
  }

  void end_array(OutputArrayType* _arr) const noexcept {}

  void end_object(OutputObjectType* _obj) const noexcept {}

 public:
  Ref<ryml::Tree> tree_;
};

}  // namespace yaml
}  // namespace rfl

#endif  // JSON_PARSER_HPP_
