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
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef SRC_WTE_PORTING_H_
#define SRC_WTE_PORTING_H_

#if defined(_WIN32)
#define NOMINMAX
// struct timeval, among other things
#include <winsock2.h>
#else
#include <sys/time.h>
#endif

#if defined(_WIN32)
#if defined(EXPORTING)
#define WTE_SYM __declspec(dllexport)
#else
#define WTE_SYM __declspec(dllimport)
#endif
#else
#define WTE_SYM
#endif


// MSVC does not implement the noexcept keyword
#if defined(_WIN32)
#define NOEXCEPT
#else
#define NOEXCEPT noexcept
#endif

#endif // SRC_WTE_PORTING_H_
