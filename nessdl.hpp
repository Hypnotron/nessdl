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
        SDL_Event event;
        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_Texture* texture;
        u32* pixels;
        int pitch;

        std::array<SDL_Event, 16> buttonMap {};

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
        enum class Field : u16_fast {
            AUDIO_BUFFER_MIN_SIZE,
            FRAMES_REMAINING,
            PAUSED,
        };

        template <typename DataType>
        inline DataType& getField(const char* const name) {
            return *(reinterpret_cast<DataType*>(fields[name]));
        }
        template <typename DataType>
        inline DataType& getField(const Field field) {
            return getField<DataType>(
                    fieldNames[static_cast<u16_fast>(field)].c_str());
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
        std::vector<std::string> fieldNames {
            "audio_buffer_min_size",
            "frames_remaining",
            "paused",
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
                     << "map <controller index> <button>: maps an SDL event"
                         << "to an NES controller button\n"
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
                getField<int>(Field::PAUSED) = !getField<int>(Field::PAUSED);
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
            {"map", [&] (std::vector<std::string>& args) {
                if (args[1] != "1" && args[1] != "2") {
                    std::cerr << "invalid value " << args[1] << "\n> ";
                    return;
                }

                while (SDL_PollEvent(&event));
                std::cerr << "press any button...\n";
                do {
                    SDL_WaitEvent(&event);
                } while (
                        event.type != SDL_CONTROLLERBUTTONDOWN
                     && event.type != SDL_KEYDOWN);

                u8_fast index = (args[2] == "a"
                      ? 0
                      : args[2] == "b"
                      ? 1
                      : args[2] == "select"
                      ? 2
                      : args[2] == "start"
                      ? 3
                      : args[2] == "up"
                      ? 4
                      : args[2] == "down"
                      ? 5
                      : args[2] == "left"
                      ? 6
                      : 7) + (args[1] == "2" ? 8 : 0);
                if ((index & 0x07) == 0x07 && args[2] != "right") {
                    std::cerr << "assuming " << args[2] << " means 'right'";
                }
                buttonMap[index] = event;

                std::cerr << "\n> ";
            }},
            {"exit", [&] (std::vector<std::string>& args) {
                getField<int>(Field::FRAMES_REMAINING) = 0;
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
            {"redo", 2},
            {"map", 3},
            {"exit", 1},
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
            SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
            
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
            window = SDL_CreateWindow(
                    "nessdl", 
                    SDL_WINDOWPOS_UNDEFINED,
                    SDL_WINDOWPOS_UNDEFINED,
                    //width:
                    768,
                    //height:
                    720,
                    SDL_WINDOW_RESIZABLE);
            //                                driver flags
            renderer = SDL_CreateRenderer(window, -1,    0);
            texture = SDL_CreateTexture(
                    renderer, 
                    SDL_PIXELFORMAT_ARGB8888,
                    SDL_TEXTUREACCESS_STREAMING,
                    256,
                    240);

            nes.audioOutputFunction = [&] (u8 sample) {
                if (
                        SDL_GetQueuedAudioSize(audioDevice) 
                      < getField<int>(Field::AUDIO_BUFFER_MIN_SIZE)) {
                    SDL_QueueAudio(
                            audioDevice, 
                            std::begin(emptyArray), 
                            getField<int>(Field::AUDIO_BUFFER_MIN_SIZE));
                }
                SDL_QueueAudio(audioDevice, &sample, 1);
            };
            nes.videoOutputFunction = [&] (u8_fast x, u8_fast y, u32 pixel) {
                pixels[y * (pitch / sizeof(u32)) + x] = pixel;
            };

            SDL_PauseAudioDevice(audioDevice, 0);
            std::cerr << "Nessdl CLI: type help for information\n> ";

            //Main loop:
            auto targetTime {std::chrono::steady_clock::now()};
            for (
                    ;
                    getField<int>(Field::FRAMES_REMAINING) > 0; 
                    --getField<int>(Field::FRAMES_REMAINING)) {
                targetTime += std::chrono::nanoseconds(16666666);

                if (!getField<int>(Field::PAUSED)) {
                    SDL_LockTexture(
                            texture, 
                            //region:
                            nullptr, 
                            reinterpret_cast<void**>(&pixels),  
                            &pitch);

                    for (u32_fast cycles {357366}; cycles > 0; --cycles) { 
                        nes.tick();
                    }

                    SDL_UnlockTexture(texture);
                    //                             src region  dst region
                    SDL_RenderCopy(renderer, texture, nullptr,   nullptr);
                    SDL_RenderPresent(renderer);
                }

                while (SDL_PollEvent(&event)) {
                    std::function<bool(
                            const SDL_Event& left, 
                            const SDL_Event& right)> match {[] (
                            const SDL_Event& left,
                            const SDL_Event& right) {
                        return false;
                    }};
                    bool pressed {false};
                    switch (event.type) {
                        //TODO: handle more events
                        case SDL_QUIT:
                            runCommand("exit");
                        break;
                        case SDL_KEYUP:
                        case SDL_KEYDOWN:
                            match = [] (
                                    const SDL_Event& left, 
                                    const SDL_Event& right) {
                                return 
                                        left.key.keysym.sym 
                                     == right.key.keysym.sym; 
                            };
                            pressed = event.key.state == SDL_PRESSED;
                        break;
                        case SDL_CONTROLLERBUTTONDOWN:
                        case SDL_CONTROLLERBUTTONUP:
                            match = [] (
                                    const SDL_Event& left, 
                                    const SDL_Event& right) {
                                return 
                                        left.cbutton.which 
                                     == right.cbutton.which
                                     && left.cbutton.button 
                                     == right.cbutton.button;
                            };
                            pressed = event.cbutton.state == SDL_PRESSED;
                        break;
                    }
                    for (u8_fast i {0}; i < 8; ++i) {
                        if (match(event, buttonMap[i])) {
                            setBit(nes.controller1, i, pressed); 
                        }
                    }
                    for (u8_fast i {8}; i < 16; ++i) {
                        if (match(event, buttonMap[i])) {
                            setBit(nes.controller2, i & 0x07, pressed); 
                        }
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
            for (auto& field : fields) {
                if (fieldTypes[field.first] == Type::INT) {
                    delete reinterpret_cast<int*>(field.second);
                }
                else if (fieldTypes[field.first] == Type::FLOAT) {
                    delete reinterpret_cast<float*>(field.second);
                }
                else if (fieldTypes[field.first] == Type::STRING) {
                    delete reinterpret_cast<std::string*>(field.second);
                }
            }
            std::cerr << "\n(finished, press enter to quit): ";
            SDL_Quit();
        }
};
