#pragma once

template<typename... Ts>
struct visitor : Ts ... { using Ts::operator()...; };

template<class... Ts>
visitor(Ts...) -> visitor<Ts...>;