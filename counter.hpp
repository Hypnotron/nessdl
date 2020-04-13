#pragma once
#include <functional>
#include "byte.hpp"

template <typename CounterType = u8_fast>
struct Counter {
    CounterType counter;
    CounterType reload;
    std::function<void()> function;

    Counter(
            const CounterType reload,
            const std::function<void()>& function)
          : reload{reload}, function{function}, counter{reload} {
    }

    void tick() {
        if(counter-- <= 0) {
            counter = reload;
            function();
        }
    }
};         

