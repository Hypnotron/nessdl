#pragma once
#include <fstream>
#include <iomanip>

namespace debug {
    struct NullStream {
        template <typename GarbageType>
        NullStream& operator<< (const GarbageType&) {
            return *this;
        }

        template <typename GarbageType1, typename GarbageType2>
        void write(const GarbageType1&, const GarbageType2&) {
        }

        template <typename GarbageType1, typename GarbageType2>
        void read(const GarbageType1&, const GarbageType2&) {
        }
    };
    #ifdef BUILD_DEBUG
        std::ofstream log("nessdl-log.txt", 
                std::ios_base::trunc | std::ios_base::out);
    #else
        NullStream log;
    #endif
}


