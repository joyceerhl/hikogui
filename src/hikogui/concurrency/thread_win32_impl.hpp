// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../win32_headers.hpp"

#include "thread_intf.hpp"
#include "unfair_mutex.hpp"
#include "../utility/utility.hpp"
#include "../char_maps/char_maps.hpp"
#include "../macros.hpp"
#include <intrin.h>
#include <mutex>
#include <string>
#include <unordered_map>

hi_export_module(hikogui.concurrency.thread : impl);

hi_export namespace hi::inline v1 {

[[nodiscard]] hi_inline thread_id current_thread_id() noexcept
{
    // Thread IDs on Win32 are guaranteed to be not zero.
    constexpr uint64_t NT_TIB_CurrentThreadID = 0x48;
    return __readgsdword(NT_TIB_CurrentThreadID);
}

hi_inline void set_thread_name(std::string_view name) noexcept
{
    auto const wname = hi::to_wstring(name);
    SetThreadDescription(GetCurrentThread(), wname.c_str());

    auto const lock = std::scoped_lock(detail::thread_names_mutex);
    detail::thread_names.emplace(current_thread_id(), std::string{name});
}

hi_inline  std::vector<bool> mask_int_to_vec(DWORD_PTR rhs) noexcept
{
    auto r = std::vector<bool>{};

    r.resize(64);
    for (std::size_t i = 0; i != r.size(); ++i) {
        r[i] = to_bool(rhs & (DWORD_PTR{1} << i));
    }

    return r;
}

hi_inline  DWORD_PTR mask_vec_to_int(std::vector<bool> const &rhs) noexcept
{
    DWORD r = 0;
    for (std::size_t i = 0; i != rhs.size(); ++i) {
        r |= rhs[i] ? (DWORD{1} << i) : 0;
    }
    return r;
}

[[nodiscard]] hi_inline std::vector<bool> process_affinity_mask()
{
    DWORD_PTR process_mask;
    DWORD_PTR system_mask;

    auto process_handle = GetCurrentProcess();

    if (not GetProcessAffinityMask(process_handle, &process_mask, &system_mask)) {
        throw os_error(std::format("Could not get process affinity mask.", get_last_error_message()));
    }

    return mask_int_to_vec(process_mask);
}

hi_inline std::vector<bool> set_thread_affinity_mask(std::vector<bool> const &mask)
{
    auto const mask_ = mask_vec_to_int(mask);

    auto const thread_handle = GetCurrentThread();

    auto const old_mask = SetThreadAffinityMask(thread_handle, mask_);
    if (old_mask == 0) {
        throw os_error(std::format("Could not set the thread affinity. '{}'", get_last_error_message()));
    }

    return mask_int_to_vec(old_mask);
}

[[nodiscard]] hi_inline std::size_t current_cpu_id() noexcept
{
    auto const index = GetCurrentProcessorNumber();
    hi_assert(index < 64);
    return index;
}

} // namespace hi::inline v1
