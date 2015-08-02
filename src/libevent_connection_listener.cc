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

#include "libevent_connection_listener.h"

#include <assert.h>

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <string>

#include <event2/util.h>

#include "wte/porting.h"
#include "xplat-io.h"

namespace wte {

LibeventConnectionListener::LibeventConnectionListener(
        std::shared_ptr<EventBase> base,
        std::function<void(int)> const& acceptCallback,
        std::function<void(std::exception const&)> errorCallback)
    : base_(base), port_(0), acceptCallback_(acceptCallback),
        errorCallback_(errorCallback), handler_(this, /*fd=*/ -1) { }

LibeventConnectionListener::~LibeventConnectionListener() {
    handler_.unregister();
    if (handler_.fd() != -1) {
        xclose(handler_.fd());
    }
}

void LibeventConnectionListener::bind(uint16_t port) {
    bind("0.0.0.0", port);
}

void LibeventConnectionListener::bind(std::string const& ip_addr,
        uint16_t port) {
    const char *error = nullptr;
    int fd = -1;

    for (;;) {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (-1 == fd) {
            error = "Failed to allocate socket";
            break;
        }

        int rc = evutil_make_socket_nonblocking(fd);
        if (-1 == rc) {
            error = "Failed to set socket non-blocking";
            break;
        }

        rc = evutil_make_listen_socket_reuseable(fd);
        if (-1 == rc) {
            error = "Failed to set socket reusable";
            break;
        }

        struct sockaddr_in saddr;
        saddr.sin_family = AF_INET;
        rc = inet_pton(AF_INET, ip_addr.c_str(), &saddr.sin_addr);
        if (1 != rc) {
            error = "Failed to convert address";
            break;
        }

        saddr.sin_port = htons(port);
        socklen_t len = sizeof(saddr);
        rc = ::bind(fd, reinterpret_cast<struct sockaddr*>(&saddr), len);
        if (-1 == rc) {
            error = "Failed to bind socket";
            break;
        }

        // Extract the bound port, in case it is ephemeral
        rc = getsockname(fd, reinterpret_cast<struct sockaddr*>(&saddr), &len);
        if (-1 == rc) {
            error = "Failed to extract port number from socket";
            break;
        }
        port_ = ntohs(saddr.sin_port);

        // Update the file descriptor in our handler
        handler_.setFd(fd);

        // Success
        return;
    }

    assert(error);

    if (fd != -1) {
        xclose(fd);
        fd = -1;
    }

    throw std::runtime_error(error);
}

void LibeventConnectionListener::listen(int backlog) {
    int rc = ::listen(handler_.fd(), backlog);
    if (-1 == rc) {
        throw std::runtime_error("Failed to listen");
    }
}

void LibeventConnectionListener::AcceptHandler::ready(What event) NOEXCEPT {
    auto& handler = listener_->handler_;

    // TODO: we could loop this and accept multiple connections in a single
    // trip through the event handler
    sockaddr_storage ss;
    socklen_t len = sizeof(ss);
    int sock = accept(handler.fd(), reinterpret_cast<struct sockaddr*>(&ss),
        &len);
    if (sock < 0) {
        // TODO: add error-specific message
        std::runtime_error err("Failed to accept connection");
        listener_->errorCallback_(err);
        return;
    }

    int rc = evutil_make_socket_nonblocking(sock);
    if (-1 == rc) {
        xclose(sock);
        std::runtime_error err("Failed to make accepted socket non-blocking");
        listener_->errorCallback_(err);
        return;
    }

    listener_->acceptCallback_(sock);
}

void LibeventConnectionListener::startAccepting() {
    // XXX assert not currently accepting
    base_->registerHandler(&handler_, What::READ);
}

void LibeventConnectionListener::stopAccepting() {
    base_->registerHandler(&handler_, What::NONE);
}

std::shared_ptr<ConnectionListener> mkConnectionListener(
        std::shared_ptr<EventBase> base,
        std::function<void(int)> const& acceptCallback,
        std::function<void(std::exception const&)> errorCallback) {
    return std::shared_ptr<ConnectionListener>(
        new LibeventConnectionListener(base, acceptCallback, errorCallback),
        std::default_delete<ConnectionListener>());
}

} // wte namespace
