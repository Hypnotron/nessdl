#pragma once
#include <functional>
#include "byte.hpp"

//TODO: Big tick
template <typename CounterType> 
struct Counter {
    CounterType counter;
    CounterType reload;
    std::function<void()> function;

    Counter(
            const CounterType reload,
            const std::function<void()>& function)
          : reload{reload}, function{function}, counter{reload} {
    }

    void tick(CounterType ticks = 1) {
        if((counter -= ticks) < 0) {
            counter += reload + 1;
            function();
        }
    }
};         

