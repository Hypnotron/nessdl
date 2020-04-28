#pragma once
#include <cstdlib>
#include <string>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>
#include <functional>
#include <unordered_map> 
#include <iostream>
#include <SDL2/SDL.h>
#include "byte.hpp"
#include "async-input.hpp"
#include "memory.hpp"
#include "nes-system.hpp"

class Nessdl {
    private:
        Nes nes;
        AsyncInput asyncInput{std::cin, 10};
        SDL_AudioDeviceID audioDevice;

        struct RwWrapper {
            SDL_RWops* data;

            RwWrapper(SDL_RWops* const data)
                  : data{data} {
            }

            void read(void* ptr, size_t size) {
                SDL_RWread(data, ptr, size, 1);
            }
            void write(void* ptr, size_t size) {
                SDL_RWwrite(data, ptr, size, 1);
            }
            void seekg(size_t offset, std::ios::seekdir way) {
                static const std::unordered_map<
                        std::ios::seekdir, 
                        int> wayToWhence {
                    {std::ios::beg, RW_SEEK_SET},
                    {std::ios::cur, RW_SEEK_CUR},
                    {std::ios::end, RW_SEEK_END},
                };
                SDL_RWseek(data, offset, wayToWhence.at(way)); 
            }

            ~RwWrapper() {
                if (data) {
                    SDL_RWclose(data);
                }
            }
        };

        enum class Type : u8_fast {
            INT, FLOAT, STRING,
        };

        template <typename DataType>
        inline DataType& getField(const char* const name) {
            return *(reinterpret_cast<DataType*>(fields[name]));
        }
        std::unordered_map<std::string, void*> fields {
            {"audio_buffer_min_size", new int(512)},
            {"frames_remaining", new int(0x7FFFFFFF)},
            {"paused", new int(1)},
        };
        std::unordered_map<std::string, Type> fieldTypes {
            {"audio_buffer_min_size", Type::INT}, 
            {"frames_remaining", Type::INT},
            {"paused", Type::INT},
        };
        std::unordered_map<std::string, std::function<
                bool(const void* const)>> constraints {
            {"audio_buffer_min_size", [] (const void* const data) {
                return  
                        *(reinterpret_cast<const int* const>(data)) >= 0
                     && *(reinterpret_cast<const int* const>(data)) <= 0xFFFF;
            }},
            {"frames_remaining", [] (const void* const data) {
                return
                        *(reinterpret_cast<const int* const>(data)) >= 0
                     && *(reinterpret_cast<const int* const>(data)) <= 0x7FFFFFFF;
            }},
            {"paused", [] (const void* const data) {
                return 
                        *(reinterpret_cast<const int* const>(data)) == 0
                     || *(reinterpret_cast<const int* const>(data)) == 1;
            }},
        };

        std::unordered_map<std::string, std::function<
                void(std::vector<std::string>&)
                >> commands {
            {"help", [] (std::vector<std::string>& args) {
                std::cerr 
                     << "help: prints this message\n"
                     << "set <variable> <value>: changes the value of " 
                         << " a variable\n"
                     << "get <variable>: prints the value of a variable\n" 
                     << "open <filename>: opens a ROM and resets the system\n"
                     << "pause: pauses/unpauses the system\n"
                     << "reset: resets the system\n"
                     << "ramdump <filename>: dumps the contents of memory"
                         << " to a file\n"
                     << "redo <depth>: reruns the command that was run"
                         << " <depth> line(s) ago\n"
                     << "exit: quits nessdl\n"
                     << "> ";
            }},
            {"set", [&] (std::vector<std::string>& args) {
                if (fields.find(args[1]) == fields.end()) {
                    std::cerr << "invalid field " << args[1] << "\n> ";
                    return;
                }
                if (fieldTypes[args[1]] == Type::INT) {
                    int value;
                    try {
                        value = std::stoi(args[2]);
                    }
                    catch (const std::invalid_argument& exception) {
                        std::cerr << args[2] << " is not an integer\n> ";
                        return;
                    }
                    if (!constraints[args[1]](&value)) {
                        std::cerr << "invalid value " << value << "\n> ";
                        return;
                    }
                    getField<int>(args[1].c_str()) = value;
                }
                else if (fieldTypes[args[1]] == Type::FLOAT) {
                    float value;
                    try {
                        value = std::stof(args[2]);
                    }
                    catch (const std::invalid_argument& exception) {
                        std::cerr << args[2] << " is not a float\n> ";
                        return;
                    }
                    if (!constraints[args[1]](&value)) {
                        std::cerr << "invalid value " << value << "\n> ";
                        return;
                    }
                    getField<float>(args[1].c_str()) = value;
                }
                else if (fieldTypes[args[1]] == Type::STRING) {
                    if (!constraints[args[1]](&args[2])) {
                        std::cerr << "invalid value " << args[2] << "\n> ";
                        return;
                    }
                    getField<std::string>(args[1].c_str()) = args[2];
                }
                std::cerr << "> ";
            }},
            {"get", [&] (std::vector<std::string>& args) {
                if (fields.find(args[1]) == fields.end()) {
                    std::cerr << "invalid field " << args[1] << "\n> ";
                    return;
                }
                if (fieldTypes[args[1]] == Type::INT) {
                    std::cerr << getField<int>(args[1].c_str());
                }
                else if (fieldTypes[args[1]] == Type::FLOAT) {
                    std::cerr << getField<float>(args[1].c_str());
                }
                else if (fieldTypes[args[1]] == Type::STRING) {
                    std::cerr << getField<std::string>(args[1].c_str());
                }
                std::cerr << "\n> ";
            }},
            {"open", [&] (std::vector<std::string>& args) {
                RwWrapper rom {SDL_RWFromFile(args[1].c_str(), "rb")};
                if (!rom.data) {
                    std::cerr << "invalid filename " << args[1] << "\n"; 
                }
                else if (!ines::isValid(rom)) {
                    std::cerr << "bad NES header\n";
                }
                else {
                    nes.load(rom);
                    nes.reset();
                }
                std::cerr << "> ";
            }},
            {"pause", [&] (std::vector<std::string>& args) {
                getField<int>("paused") = !getField<int>("paused");
                std::cerr << "> ";
            }},
            {"reset", [&] (std::vector<std::string>& args) {
                nes.reset();
                std::cerr << "> ";
            }},
            {"ramdump", [&] (std::vector<std::string>& args) {
                nes.ramdump(args[1].c_str());
                std::cerr << "> ";
            }},
            {"redo", [&] (std::vector<std::string>& args) {
                u16_fast depth;
                try {
                    depth = std::stoi(args[1]);
                }
                catch (const std::invalid_argument& exception) {
                    std::cerr << args[2] << " is mot an integer\n> ";
                    return;
                }
                if (depth < 1 || depth > 9) {
                    std::cerr << "invalid value " << depth << "\n> ";
                    return;
                }
                std::string command;
                asyncInput.getHistory(command, depth + 1);
                asyncInput.setHistory(command, 1);
                std::cerr << "running \"" << command << "\"\n"; 
                runCommand(command); 
            }},
            {"exit", [&] (std::vector<std::string>& args) {
                getField<int>("frames_remaining") = 0;
            }},
        };
        std::unordered_map<std::string, u8_fast> commandSizes {
            {"help", 1},
            {"set", 3},
            {"get", 2},
            {"open", 2},
            {"pause", 1},
            {"reset", 1},
            {"ramdump", 2},
            {"exit", 1},
            {"redo", 2},
        };
        void runCommand(const std::string& command) { 
            std::istringstream line {command};
            std::vector<std::string> args;
            for (std::string arg; std::getline(line, arg, ' '); ) {
                args.push_back(arg);
            }
            if (args.size() == 0) {
                std::cerr << "> ";
            }
            else {
                if (commands.find(args[0]) == commands.end()) {
                    std::cerr << "invalid command " << args[0] << "\n> ";
                }
                else if (args.size() < commandSizes[args[0]]) {
                    std::cerr << "missing parameters\n> ";
                }
                else {
                    commands[args[0]](args);
                }
            }
        }

    public:

        Nessdl() {
            //Setup:
            SDL_Init(SDL_INIT_AUDIO);
            
            SDL_AudioSpec desired, obtained;
            SDL_zero(desired);
            desired.freq = 59561;
            desired.format = AUDIO_U8;
            desired.channels = 1;
            desired.samples = 2048;
            desired.callback = nullptr;
            audioDevice = SDL_OpenAudioDevice(
                    //default audio device:
                    nullptr, 
                    //is capture device:
                    0, 
                    &desired, 
                    &obtained, 
                    //prohibit changes to specification:
                    0);

            nes.audioOutputFunction = [&] (u8 sample) {
                if (
                        SDL_GetQueuedAudioSize(audioDevice) 
                      < getField<int>("audio_buffer_min_size")) {
                    SDL_QueueAudio(
                            audioDevice, 
                            std::begin(emptyArray), 
                            getField<int>("audio_buffer_min_size"));
                }
                SDL_QueueAudio(audioDevice, &sample, 1);
            };

            SDL_PauseAudioDevice(audioDevice, 0);
            std::cerr << "Nessdl CLI: type help for information\n> ";

            //Main loop:
            auto targetTime {std::chrono::steady_clock::now()};
            for (
                    ;
                    getField<int>("frames_remaining") > 0; 
                    --getField<int>("frames_remaining")) {
                targetTime += std::chrono::nanoseconds(16666666);

                if (!getField<int>("paused")) {
                    for (u32_fast cycles {357366}; cycles > 0; --cycles) { 
                        nes.tick();
                    }
                }

                for (std::string line; asyncInput.get(line); ) {
                    runCommand(line);
                }

                auto currentTime {std::chrono::steady_clock::now()};
                if (currentTime < targetTime) {
                    std::this_thread::sleep_for(std::chrono::duration_cast<
                            std::chrono::nanoseconds>(
                                    targetTime - currentTime)); 
                }
            }

            //Cleanup:
            std::cerr << "\n(finished, press enter to quit): ";
            SDL_Quit();
        }
};
