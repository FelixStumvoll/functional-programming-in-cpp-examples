#pragma once

#include <functional>
#include <optional>
#include <ranges>
#include <string_view>
#include <utility>

using namespace std::ranges;
using input = std::string_view;

template<typename T>
concept is_parse_result = std::same_as<T,
  std::optional<std::pair<typename T::value_type::first_type, input>>>;

template<typename F>
concept is_parser = std::regular_invocable<F, input>
                    && is_parse_result<std::invoke_result_t<F, input>>;

template<typename T> using parse_result_t = std::optional<std::pair<T, input>>;

template<is_parser T> using parsed_type_t =
  std::invoke_result_t<T, input>::value_type::first_type;

template<is_parser F> is_parser auto mk_parser(F&& fn);

template<typename V> is_parser auto pure_parser(V&& value);

template<is_parser F>
class parser {
  F _fn;
public:
  using parsed_type = parsed_type_t<F>;
  using parse_result = parse_result_t<parsed_type>;

  template<std::same_as<F> F1> requires (!std::same_as<F1, parser>)
  explicit parser(F1&& fn) : _fn(std::forward<F1>(fn)) {}
  parse_result operator()(input s) const { return std::invoke(_fn, s); }

  template<std::regular_invocable<parsed_type&&> M>
  is_parser auto transform(M&& fn)&& {
    return pure_parser<M>(std::forward<M>(fn)).ap(std::move(*this));
  }

/*
  // transform implementation using and_then
  template<std::regular_invocable<parsed_type&&> MapFn>
  is_parser auto transform(MapFn&& fn) {
    return std::move(*this).and_then([fn = std::forward<MapFn>(fn)](parsed_type&& value) {
      return pure_parser(fn(std::move(value)));
    });
  }
*/

/*
  // manual transform implementation
  template<std::regular_invocable<parsed_type&&> M>
  is_parser auto transform(M&& fn)&& {
    return mk_parser([self{std::move(*this)}, fn = std::forward<M>(fn)](input s) {
      return self(s).transform([&fn](parse_result::value_type&& result) {
        return std::pair{fn(std::move(result.first)), result.second};
      });
    });
  }
*/

  template<std::regular_invocable<parsed_type&&> M> requires is_parser<std::invoke_result_t<M, parsed_type&&>>
  is_parser auto and_then(M&& fn)&& {
    return mk_parser([self{std::move(*this)}, fn = std::forward<M>(fn)](input s) {
      return self(s).and_then([&fn](parse_result::value_type&& result) {
        return fn(std::move(result.first))(result.second);
      });
    });
  }

  template<is_parser P> requires std::regular_invocable<parsed_type, parsed_type_t<P>>
  is_parser auto ap(P&& other)&& {
    return mk_parser(
      [self{std::move(*this)}, other = std::forward<P>(other)](input s) {
        return self(s).and_then([&other](parse_result::value_type&& result) {
          auto [fn, rest] = std::move(result);
          return other(rest)
            .transform([&fn](parse_result_t<parsed_type_t<P>>::value_type&& ap_result) {
              return std::pair{fn(std::move(ap_result.first)), ap_result.second};
            });
        });
      });
  }

  template<is_parser P> requires std::same_as<parsed_type_t<P>, parsed_type>
  auto or_else(P&& other)&& {
    return mk_parser([self{std::move(*this)}, other = std::forward<P>(other)](input s) -> is_parse_result auto {
      return self(s).or_else([&other, &s] { return other(s); });
    });
  }

  template<is_parser P>
  is_parser auto ignore_and(P&& other)&& {
    return std::move(*this)
      .and_then([other = std::forward<P>(other)](auto&&) -> const P& { return other; });
  }

  template<std::predicate<const parsed_type&> P>
  is_parser auto filter(P&& pred)&& {
    return mk_parser([self{std::move(*this)}, pred = std::forward<P>(pred)](input s) -> is_parse_result auto {
      return self(s).and_then([&pred](parse_result::value_type&& result) {
        if (!pred(result.first)) { return parse_result{}; }
        else { return parse_result{std::move(result)}; }
      });
    });
  }

  is_parser auto many()&& {
    return mk_parser([self{std::move(*this)}](input s) {
      std::vector<parsed_type> vec;
      while (auto result = self(s)) {
        vec.push_back(std::move(result->first));
        s = result->second;
      }
      return parse_result_t<decltype(vec)>{{std::move(vec), s}};
    });
  }

  is_parser auto some()&& {
    return mk_parser([self{std::move(*this).many()}](input s) {
      using many_result = parse_result_t<parsed_type_t<decltype(self)>>;
      return self(s).and_then([](many_result::value_type&& result) {
        if (result.first.empty()) { return many_result{}; }
        else { return many_result{std::move(result)}; }
      });
    });
  }

/*
  // some implementation using filter
  is_parser auto some()&& {
    return std::move(*this)
      .many()
      .filter(std::not_fn(&std::vector<parsed_type>::empty));
  }
*/
};

template<is_parser F>
is_parser auto mk_parser(F&& f) { return parser<F>(std::forward<F>(f)); }

template<typename V>
is_parser auto pure_parser(V&& value) {
  return mk_parser([value = std::forward<V>(value)](input s) {
    return parse_result_t<std::remove_cvref_t<V>>{{value, s}};
  });
}

template<std::predicate<char> P>
is_parser auto parse_if(P&& pred) {
  return mk_parser([pred = std::forward<P>(pred)](input s) -> parse_result_t<char> {
    if (!empty(s | views::take(1) | views::filter(pred))) {
      return parse_result_t<char>{{s[0], {s | views::drop(1)}}};
    } else { return parse_result_t<char>{}; }
  });
}

is_parser auto parse_char(char comp) { return parse_if(std::bind_front(std::equal_to<>(), comp)); }