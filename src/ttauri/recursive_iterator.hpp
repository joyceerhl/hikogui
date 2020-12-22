// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include <type_traits>
#include <compare>

namespace tt {

/** An iterator which recursively iterates through nested containers.
 * Currently only recurses through two levels of containers.
 * 
 */
template<typename ParentIt>
class recursive_iterator {
    using parent_iterator = ParentIt;
    using child_iterator = std::conditional_t<
        std::is_const_v<std::remove_reference_t<typename std::iterator_traits<ParentIt>::reference>>,
        typename std::iterator_traits<ParentIt>::value_type::const_iterator,
        typename std::iterator_traits<ParentIt>::value_type::iterator>;

    using value_type = typename std::iterator_traits<child_iterator>::value_type;
    using difference_type = typename std::iterator_traits<child_iterator>::difference_type;
    using reference = typename std::iterator_traits<child_iterator>::reference;
    using pointer = typename std::iterator_traits<child_iterator>::pointer;

public:
    recursive_iterator() noexcept = delete;
    recursive_iterator(recursive_iterator const &other) noexcept = default;
    recursive_iterator(recursive_iterator &&other) noexcept = default;
    recursive_iterator &operator=(recursive_iterator const &other) noexcept = default;
    recursive_iterator &operator=(recursive_iterator &&other) noexcept = default;
    ~recursive_iterator() = default;

    /** Create an iterator at an element inside a child container.
     */
    recursive_iterator(parent_iterator parent_it, parent_iterator parent_it_end, child_iterator child_it) noexcept :
        _parent_it(parent_it), _parent_it_end(parent_it_end), _child_it(child_it)
    {
    }

    /** Create a begin iterator at the first child's first element.
     */
    recursive_iterator(parent_iterator parent_it, parent_iterator parent_it_end) noexcept :
        _parent_it(parent_it),
        _parent_it_end(parent_it_end),
        _child_it(parent_it != parent_it_end ? std::begin(*parent_it) : child_iterator{})
    {
    }

    /** Create an end iterator one beyond the last child.
     */
    recursive_iterator(parent_iterator parent_it_end) noexcept :
        _parent_it(parent_it_end), _parent_it_end(parent_it_end), _child_it()
    {
    }

    [[nodiscard]] parent_iterator parent() const noexcept
    {
        return _parent_it;
    }

    /** Don't need to check the child_it at end.
     */
    [[nodiscard]] bool at_end() const noexcept
    {
        return _parent_it == _parent_it_end;
    }

    [[nodiscard]] reference operator*() const noexcept
    {
        return *_child_it;
    }

    [[nodiscard]] pointer operator->() const noexcept
    {
        return &(*_child_it);
    }

    [[nodiscard]] reference operator[](size_t i) const noexcept
    {
        return *(*this + i);
    }

    recursive_iterator &operator++() noexcept
    {
        ++_child_it;
        if (_child_it == std::end(*_parent_it)) {
            ++_parent_it;
            if (!at_end()) {
                _child_it = _parent_it->begin();
            }
        }
        return *this;
    }

    recursive_iterator &operator--() noexcept
    {
        if (_child_it == std::begin(*_parent_it)) {
            --_parent_it;
            _child_it = std::end(*_parent_it);
        }
        --_child_it;
        return *this;
    }

    recursive_iterator operator++(int) noexcept
    {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }
    recursive_iterator operator--(int) noexcept
    {
        auto tmp = *this;
        --(*this);
        return tmp;
    }

    recursive_iterator &operator+=(difference_type rhs) noexcept
    {
        if (rhs < 0) {
            return (*this) -= -rhs;
        }

        do {
            ttlet left_in_child = std::distance(_child_it, std::end(*_parent_it));

            if (left_in_child < rhs) {
                ++_parent_it;
                if (!at_end()) {
                    _child_it = std::begin(*_parent_it);
                }
                rhs -= left_in_child;

            } else {
                _child_it += rhs;
                rhs -= rhs;
            }

        } while (rhs);

        return *this;
    }

    recursive_iterator &operator-=(difference_type rhs) noexcept
    {
        if (rhs < 0) {
            return (*this) += -rhs;
        }

        do {
            ttlet left_in_child = std::distance(std::begin(*_parent_it), _child_it) + 1;

            if (left_in_child < rhs) {
                --_parent_it;
                _child_it = std::end(*_parent_it) - 1;
                rhs -= left_in_child;

            } else {
                _child_it -= rhs;
                rhs -= rhs;
            }

        } while (rhs);

        return *this;
    }

    [[nodiscard]] friend bool operator==(recursive_iterator const &lhs, recursive_iterator const &rhs) noexcept
    {
        if (lhs._parent_it != rhs._parent_it) {
            return false;
        } else if (lhs.at_end()) {
            tt_axiom(rhs.at_end());
            return true;
        } else {
            return lhs._child_it == rhs._child_it;
        }
    }

    [[nodiscard]] friend std::strong_ordering
    operator<=>(recursive_iterator const &lhs, recursive_iterator const &rhs) noexcept
    {
        if (lhs._parent_it != rhs._parent_it) {
            return lhs._parent_it <=> rhs._parent_it;
        } else if (lhs.at_end()) {
            tt_axiom(rhs.at_end());
            return std::strong_ordering::equal;
        } else {
            return lhs._child_it <=> rhs._child_it;
        }
    }

    [[nodiscard]] friend recursive_iterator operator+(recursive_iterator lhs, difference_type rhs) noexcept
    {
        return lhs += rhs;
    }
    [[nodiscard]] friend recursive_iterator operator-(recursive_iterator lhs, difference_type rhs) noexcept
    {
        return lhs -= rhs;
    }
    [[nodiscard]] friend recursive_iterator operator+(difference_type lhs, recursive_iterator rhs) noexcept
    {
        return rhs += lhs;
    }

    [[nodiscard]] friend difference_type operator-(recursive_iterator const &lhs, recursive_iterator const &rhs) noexcept
    {
        if (rhs < lhs) {
            return -(rhs - lhs);
        } else {
            auto lhs_ = lhs;
            difference_type count = 0;
            while (lhs_._parent_it < rhs._parent_it) {
                count += std::distance(lhs_.child_it, std::end(*lhs_._parent_it));
                ++(lhs_._parent_it);
                lhs_._child_it = std::begin(*lhs_._parent_it);
            }
            return count + std::distance(lhs_._child_it, rhs._child_it);
        }
    }

private:
    parent_iterator _parent_it;
    parent_iterator _parent_it_end;
    child_iterator _child_it;
};

template<typename Container>
[[nodiscard]] auto recursive_iterator_begin(Container &rhs) noexcept
{
    return recursive_iterator(std::begin(rhs), std::end(rhs));
}

template<typename Container>
[[nodiscard]] auto recursive_iterator_end(Container &rhs) noexcept
{
    return recursive_iterator(std::end(rhs), std::end(rhs));
}

template<typename Container>
[[nodiscard]] auto recursive_iterator_begin(Container const &rhs) noexcept
{
    return recursive_iterator(std::begin(rhs), std::end(rhs));
}

template<typename Container>
[[nodiscard]] auto recursive_iterator_end(Container const &rhs) noexcept
{
    return recursive_iterator(std::end(rhs), std::end(rhs));
}

} // namespace tt
