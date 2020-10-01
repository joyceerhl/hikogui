// Copyright 2019, 2020 Pokitec
// All rights reserved.

#pragma once

#include "../formula/formula.hpp"
#include "../strings.hpp"
#include "../algorithm.hpp"
#include "../ResourceView.hpp"
#include <memory>
#include <string_view>
#include <optional>

namespace tt {

struct stencil_node;

struct stencil_parse_context {
    using statement_stack_type = std::vector<std::unique_ptr<stencil_node>>;
    using const_iterator = typename std::string_view::const_iterator;

    statement_stack_type statement_stack;

    parse_location location;
    const_iterator index;
    const_iterator last;

    std::optional<const_iterator> text_segment_start;

    /** Post process context is used to record functions that are defined
    * in the template being parsed.
    */
    formula_post_process_context post_process_context;

    stencil_parse_context() = delete;
    stencil_parse_context(stencil_parse_context const &other) = delete;
    stencil_parse_context &operator=(stencil_parse_context const &other) = delete;
    stencil_parse_context(stencil_parse_context &&other) = delete;
    stencil_parse_context &operator=(stencil_parse_context &&other) = delete;
    ~stencil_parse_context() = default;

    stencil_parse_context(URL const &url, const_iterator first, const_iterator last);

    [[nodiscard]] char const& operator*() const noexcept {
        return *index;
    }

    [[nodiscard]] bool atEOF() const noexcept {
        return index == last;
    }

    stencil_parse_context& operator++() noexcept {
        tt_assume(!atEOF());
        location += *index;
        ++index;
        return *this;
    }

    stencil_parse_context& operator+=(ssize_t x) noexcept {
        for (ssize_t i = 0; i != x; ++i) {
            ++(*this);
        }
        return *this;
    }

    bool starts_with(std::string_view text) const noexcept {
        return ::tt::starts_with(index, last, text.begin(), text.end());
    }

    bool starts_with_and_advance_over(std::string_view text) noexcept {
        if (starts_with(text)) {
            *this += std::ssize(text);
            return true;
        } else {
            return false;
        }
    }

    bool advance_to(std::string_view text) noexcept {
        while (!atEOF()) {
            if (starts_with(text)) {
                return true;
            }
            ++(*this);
        }
        return false;
    }

    bool advance_over(std::string_view text) noexcept {
        if (advance_to(text)) {
            *this += std::ssize(text);
            return true;
        } else {
            return false;
        }
    }

    std::unique_ptr<formula_node> parse_expression(std::string_view end_text);

    std::unique_ptr<formula_node> parse_expression_and_advance_over(std::string_view end_text);

    template<typename T, typename... Args>
    void push(Args &&... args) {
        statement_stack.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    [[nodiscard]] bool append(std::unique_ptr<stencil_node> x) noexcept;

    template<typename T, typename... Args>
    [[nodiscard]] bool append(Args &&... args) noexcept {
        if (statement_stack.size() > 0) {
            return append(std::make_unique<T>(std::forward<Args>(args)...));
        } else {
            return false;
        }
    }

    /** Handle \#end statement.
    * This will pop the current statement of the stack and append it
    * to the statement that is now at the top of the stack.
    */
    [[nodiscard]] bool pop() noexcept;

    void start_of_text_segment(int back_track = 0) noexcept;
    void end_of_text_segment();

    [[nodiscard]] bool top_statement_is_do() const noexcept;

    [[nodiscard]] bool found_elif(parse_location location, std::unique_ptr<formula_node> expression) noexcept;

    [[nodiscard]] bool found_else(parse_location location) noexcept;

    [[nodiscard]] bool found_while(parse_location location, std::unique_ptr<formula_node> expression) noexcept;

    void include(parse_location location, std::unique_ptr<formula_node> expression);
};

}