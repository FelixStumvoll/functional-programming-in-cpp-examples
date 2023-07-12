#include <iostream>
#include <numeric>
#include <string>

#include "parser.h"

using namespace std::literals;
using namespace std::ranges;

is_parser auto whitespaces() {
  return parse_char(' ').many();
}

is_parser auto digit_parser() {
  return parse_if(&isdigit).transform([](char c) { return c - '0'; });
}

is_parser auto digit_parser_1() {
  return parse_if([](char c) { return c == '1'; }).transform([](char c) { return c - '0'; });
}

is_parser auto number_parser() {
  return digit_parser().some().transform([](std::vector<int>&& v) {
    return std::accumulate(v.begin(), v.end(), 0, [](int acc, int c) { return acc * 10 + c; });
  });
}

is_parser auto integer_parser() {
  return parse_char('-').ignore_and(number_parser().transform(std::negate<>()))
    .or_else(number_parser());
}

template<is_parser P>
is_parser auto token(P&& p) { return whitespaces().ignore_and(std::forward<P>(p)); }

is_parser auto op() {
  return parse_char('+').or_else(parse_char('-')).or_else(parse_char('*'))
    .transform([](char c) {
      return [c](int a) {
        return [c, a](int b) {
          switch (c) {
            case '+':
              return a + b;
            case '-':
              return a - b;
            case '*':
              return a * b;
            default:
              return 0;
          }
        };
      };
    });
}

is_parser auto calc_parser() {
  return token(integer_parser()).and_then([](int a) {
    return token(op()).ap(pure_parser(a)).ap(token(integer_parser()));
  });
}