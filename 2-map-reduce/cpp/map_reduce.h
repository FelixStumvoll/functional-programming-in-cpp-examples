#pragma once

#include <future>
#include <map>
#include <ranges>
#include <vector>

using namespace std::ranges;

template<typename Fn, typename Input>
using map_result_t = std::invoke_result_t<Fn, const range_value_t<Input>&>;

template<
  typename MapFn, typename ReduceFn, sized_range Input,
  typename Key = range_value_t<map_result_t<MapFn, Input>>::first_type,
  typename MValue = range_value_t<map_result_t<MapFn, Input>>::second_type,
  typename RValue = std::invoke_result_t<ReduceFn, std::vector<MValue>&&>
>
std::map<Key, RValue> map_reduce_parallel(
  const MapFn& map_fn,
  const ReduceFn& reduce_fn,
  int map_workers, int reduce_workers,
  const Input& input
) {
  std::vector<std::future<std::vector<std::pair<Key, MValue>>>> map_futures = input
    | views::chunk(std::max((int) (size(input) / map_workers), 1))
    | views::transform([&map_fn](range auto chunk) {
        return std::async([chunk, &map_fn]() {
          return chunk
                | views::transform([&](const auto& chunk) { return map_fn(chunk); })
                | views::join
                | views::transform([](auto& pair) { return std::move(pair); })
                | to<std::vector>();
        });
      })
    | to<std::vector>();

  std::map<Key, std::vector<MValue>> grouped;
  for (auto& [key, value] : map_futures
                            | views::transform([](auto& f) { return f.get(); })
                            | views::join) {
    grouped.try_emplace(std::move(key)).first->second.push_back(std::move(value));
  }

  std::vector<std::future<std::vector<std::pair<Key, RValue>>>> reduce_futures =
    grouped
    | views::keys
    | views::chunk(std::max((int) size(grouped) / reduce_workers, 1))
    | views::transform([&reduce_fn, &grouped](auto keys) {
        return std::async([&reduce_fn, &grouped, keys]() {
          return keys
                  | views::transform([&reduce_fn, &grouped](const Key& key) {
                      return std::pair{key, reduce_fn(std::move(grouped.at(key)))};
                    })
                  | to<std::vector>();
        });
      }) 
    | to<std::vector>();

  return reduce_futures
          | views::transform([](auto& f) { return f.get(); }) 
          | views::join
          | views::transform([](auto& pair) { return std::move(pair); })
          | to<std::map>();
}