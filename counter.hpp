#pragma once
#include <functional>
#include "byte.hpp"

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

    void tick(const CounterType ticks = 1) {
        counter -= ticks;
        while (counter < 0) { 
            counter += reload + 1;
            function();
        }
    }
};         

