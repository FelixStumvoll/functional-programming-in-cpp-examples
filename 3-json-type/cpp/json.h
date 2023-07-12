#pragma once

#include <algorithm>
#include <format>
#include <map>
#include <ranges>
#include <string>
#include <variant>
#include <vector>

#include "visitor.h"

using namespace std::literals;
using namespace std::ranges;

class null {};
class object;
class array;
using json = std::variant<double, bool, null, std::string, object, array>;

class array {
  using array_elems = std::vector<json>;
  array_elems _elements;

public:
  array(array_elems&& elements) : _elements(std::move(elements)) {}

  const array_elems& elements() const { return _elements; }

  array add(json&& elem)&& {
    array arr{std::move(*this)};
    arr._elements.push_back(std::move(elem));
    return arr;
  }
};

class object {
  using object_elems = std::map<std::string, json>;
  object_elems _elements;

public:
  object(object_elems&& elements) : _elements(std::move(elements)) {}

  const object_elems& elements() const { return _elements; }

  object add(std::pair<std::string, json>&& elem)&& {
    object obj{std::move(*this)};
    obj._elements.insert(std::move(elem));
    return obj;
  }
};

std::string stringify(const json& json) {
  return std::visit(
    visitor(
      [](null) { return "null"s; },
      [](bool bool_val) { return bool_val ? "true"s : "false"s; },
      [](double double_val) { return std::to_string(double_val); },
      [](const std::string& s) { return std::format("\"{}\"", s); },
      [](const object& obj) {
        if (obj.elements().empty()) { return "{}"s; }
        auto rev_rng = obj.elements() | views::reverse;
        auto init = rev_rng | views::drop(1) | views::reverse | views::transform([](const auto& kv) {
          return std::format("\"{}\":{},", kv.first, stringify(kv.second));
        }) | views::join | to<std::string>();

        const auto& [key, value] = (*begin(rev_rng));

        auto last = std::format("\"{}\": {}", key, stringify(value));
        return std::format("{{{}{}}}", std::move(init), std::move(last));
      },
      [](const array& arr) {
        if (arr.elements().empty()) { return "[]"s; }
        auto rev_rng = arr.elements() | views::reverse;
        auto init = rev_rng | views::drop(1) | views::reverse | views::transform([](const auto& elem) {
          return std::format("{},", stringify(elem));
        }) | views::join | to<std::string>();

        return std::format("[{}{}]", std::move(init), stringify(*begin(rev_rng)));
      }
    ),
    json
  );
}