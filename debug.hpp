#pragma once
#include <fstream>

namespace debug {
    struct NullStream {
        template <typename GarbageType>
        NullStream& operator<< (const GarbageType&) {
            return *this;
        }
    };
    #ifdef BUILD_DEBUG
        std::ofstream log("nessdl-log.txt", 
                std::ios_base::trunc | std::ios_base::out);
    #else
        NullStream log;
    #endif
}


