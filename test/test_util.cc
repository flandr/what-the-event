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

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "test_util.h"

namespace wte {

int connectOrThrow(ConnectionListener *listener) {
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    int rc = inet_pton(AF_INET, "127.0.0.1", &saddr.sin_addr);
    if (-1 == rc) {
        throw std::runtime_error("fail");
    }
    saddr.sin_port = htons(listener->port());

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == fd) {
        throw std::runtime_error("fail");
    }

    rc = connect(fd, reinterpret_cast<sockaddr*>(&saddr), sizeof(saddr));
    if (-1 == rc) {
        throw std::runtime_error("fail");
    }

    return fd;
}

} // wte namespace
