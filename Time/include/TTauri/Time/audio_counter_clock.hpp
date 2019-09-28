// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "TTauri/Required/os_detect.hpp"
#include <chrono>
#if COMPILER == CC_MSVC
#include <intrin.h>
#elif COMPILER == CC_CLANG
#include <x86intrin.h>
#elif COMPILER == CC_GCC
#include <x86intrin.h>
#endif

namespace TTauri {

struct audio_counter_clock {
    using rep = uint64_t;
    // Technically not nano-seconds, this just a counter.
    using period = std::nano;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<audio_counter_clock>;
    static const bool is_steady = true;

    static audio_counter_clock::time_point from_audio_api(uint64_t value) noexcept;

    static audio_counter_clock::time_point now() noexcept;
};

}