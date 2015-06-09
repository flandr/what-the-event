/*
 * Copyright © 2015 Nathan Rosenblum <flander@gmail.com>
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

#include <gtest/gtest.h>

#include "optional.h"

namespace wte {

TEST(OptionalTest, Basic) {
    auto opt = Optional<int>::make(1);
    ASSERT_EQ(1, opt.value());
}

TEST(OptionalTest, BooleanContext) {
    Optional<int> empty;
    Optional<int> full(1);
    ASSERT_TRUE(full);
    ASSERT_FALSE(empty);
}

TEST(OptionalTest, ConstantValue) {
    auto const copt = Optional<int>::make(1);
    ASSERT_EQ(1, copt.value());
}

TEST(OptionalTest, EmptyOptionalThrows) {
    Optional<int> empty;
    ASSERT_THROW({empty.value();}, std::logic_error);
}

TEST(OptionalTest, SourceEmptyAfterMoveConstruction) {
    Optional<int> source(1);
    Optional<int> target(std::move(source));
    ASSERT_FALSE(source);
    ASSERT_TRUE(target);
    ASSERT_EQ(1, target.value());
}

TEST(OptionalTest, AssignmentOperator) {
    Optional<int> empty_target;
    Optional<int> source(1);

    ASSERT_FALSE(empty_target);
    empty_target = source;
    ASSERT_TRUE(empty_target);
    ASSERT_EQ(1, empty_target.value());

    Optional<int> full_target(2);
    full_target = source;
    ASSERT_EQ(1, full_target.value());
}

} // wte namespace
