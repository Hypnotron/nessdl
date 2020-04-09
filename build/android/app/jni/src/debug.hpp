#pragma once
#include <fstream>

namespace debug {
    #ifdef BUILD_DEBUG
        std::ofstream log("nessdl-log.txt", 
                std::ios_base::trunc | std::ios_base::out);
    #else
        struct NullStream {
            template <typename GarbageType>
            NullStream& operator<< (const GarbageType&) {
                return *this;
            }
        };
        NullStream log;
    #endif
}


