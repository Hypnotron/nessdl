#include <thread>
#include <mutex>
#include <atomic>
#include <deque>
#include <istream>
#include "byte.hpp"

class AsyncInput {
    private:
        std::atomic<bool> enabled {true}; 
        std::mutex mutex;
        u8_fast historySize;
        std::deque<std::string> lines{historySize};
        std::istream& input;
        std::thread thread{[&] () {
            for (std::string line; enabled && std::getline(input, line); ) {
                mutex.lock();
                lines.push_back(line);
                mutex.unlock();
            }
        }};

    public:
        AsyncInput(std::istream& input, u8_fast historySize) 
              : input{input}, historySize{historySize} {
        }

        ~AsyncInput() {
            enabled = false;
            thread.join();
        }
    

        bool get(std::string& line) {
            mutex.lock();
            bool valid {lines.size() > historySize}; 
            if (valid) {
                line = lines[historySize]; 
                lines.pop_front();
            }
            mutex.unlock();
            return valid; 
        }

        void getHistory(std::string& line, const u8_fast depth) {
            mutex.lock();
            line = lines[historySize - depth];
            mutex.unlock();
        }

        void setHistory(const std::string& line, const u8_fast depth) {
            mutex.lock();
            lines[historySize - depth] = line;
            mutex.unlock();
        }
};
