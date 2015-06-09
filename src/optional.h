/*
 * Copyright (©) 2015 Nate Rosenblum
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef SRC_OPTIONAL_H_
#define SRC_OPTIONAL_H_

#include <stdexcept>
#include <utility>

namespace wte {

/** Dummy type for use in `optional_storage`. */
struct dummy_t { };

/** Storage for non-default-constructible types. */
template<typename T>
union optional_storage {
    dummy_t dummy;
    T value;

    optional_storage() : dummy() { }
    optional_storage(T const& v) : value(v) { }
    optional_storage(T && v) : value(std::move(v)) { }

    ~optional_storage() { }
};

/**
 * An implementation of portions of Optional from library fundamentals
 * TS N3848 (http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3848.html).
 *
 * See also http://en.cppreference.com/w/cpp/experimental/optional/optional.
 * However, only the good parts :) and won't save you from silly things like
 * trying to use it with a reference type.
 *
 * Doesn't try to implement the constexpr constructors, which are severely
 * complicated by the lack of a constexpr std::forward in C++11.
 *
 * Oh, the lengths you'll go to not take a dependency on boost :/
 */
template<typename T>
class Optional {
public:
    Optional();
    Optional(Optional &&);
    explicit Optional(T const& value);
    explicit Optional(T && value);
    // No in_place_t in-place constructors; use the generator below

    /** Generate an optional in-place. */
    template<typename... U>
    static Optional<T> make(U && ...u) {
        return Optional<T>(T(std::forward<U>(u)...));
    }

    Optional& operator=(Optional const& o) {
        if (!engaged_ && !o.engaged_) {
            return *this;
        } else if (engaged_ && !o.engaged_) {
            clear();
            return *this;
        }
        value_.value = o.value_.value;
        engaged_ = true;
        return *this;
    }

    ~Optional() {
        clear();
    }

    /** @return the value, or throws std::logic_error if not engaged. */
    T & value();

    /** @return the value, or throws std::logic_error if not engaged. */
    T const& value() const;

    /** @return whether the optional is engaged. */
    operator bool() const;
private:
    void clear() noexcept;

    template<typename... U>
    void construct(U && ...u) {
        void *ptr = &value_.value;
        ::new(static_cast<T*>(ptr)) T(std::forward<U>(u)...);
        engaged_ = true;
    }

    bool engaged_;
    optional_storage<T> value_;
};

template<typename T>
Optional<T>::Optional() : engaged_(false) { }

template<typename T>
Optional<T>::operator bool() const {
    return engaged_;
}

template<typename T>
Optional<T>::Optional(Optional<T> && o) {
    if (o.engaged_) {
        construct(std::move(o.value_.value));
        o.clear();
    } else {
        engaged_ = false;
    }
}

template<typename T>
Optional<T>::Optional(T const& v) {
    construct(v);
}

template<typename T>
Optional<T>::Optional(T && v) {
    construct(std::move(v));
}

template<typename T>
void Optional<T>::clear() noexcept {
    if (engaged_) {
        // Explicitly destroy value in the union storage type
        value_.value.T::~T();
        engaged_ = false;
    }
}

template<typename T>
T& Optional<T>::value() {
    if (!engaged_) {
        throw std::logic_error("Optional has no value");
    }
    return value_.value;
}

template<typename T>
T const& Optional<T>::value() const {
    if (!engaged_) {
        throw std::logic_error("Optional has no value");
    }
    return value_.value;
}

} // wte namespace

#endif // SRC_OPTIONAL_H_
