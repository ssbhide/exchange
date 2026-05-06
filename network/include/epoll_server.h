#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>

#if defined(__linux__)
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "lockfree_queue.h"
#include "protocol.h"

namespace exchange::network {

template <std::size_t QueueSize, std::size_t BufferSize = 4096>
class EpollServer {
public:
    using Queue = LockFreeQueue<Order, QueueSize>;

    EpollServer(std::uint16_t port, Queue& queue)
        : port_(port), queue_(queue) {}

    ~EpollServer() {
        stop();
    }

    bool start() {
#if defined(__linux__)
        if (running_) {
            return true;
        }

        listenFd_ = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (listenFd_ < 0) {
            return false;
        }

        int reuse = 1;
        (void)::setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons(port_);

        if (::bind(listenFd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
            stop();
            return false;
        }

        if (::listen(listenFd_, SOMAXCONN) < 0) {
            stop();
            return false;
        }

        epollFd_ = ::epoll_create1(EPOLL_CLOEXEC);
        if (epollFd_ < 0) {
            stop();
            return false;
        }

        epoll_event event{};
        event.events = EPOLLIN;
        event.data.fd = listenFd_;
        if (::epoll_ctl(epollFd_, EPOLL_CTL_ADD, listenFd_, &event) < 0) {
            stop();
            return false;
        }

        running_ = true;
        return true;
#else
        (void)port_;
        return false;
#endif
    }

    void stop() {
#if defined(__linux__)
        running_ = false;
        for (auto& connection : connections_) {
            if (connection.fd >= 0) {
                ::close(connection.fd);
                connection = {};
            }
        }

        if (epollFd_ >= 0) {
            ::close(epollFd_);
            epollFd_ = -1;
        }

        if (listenFd_ >= 0) {
            ::close(listenFd_);
            listenFd_ = -1;
        }
#endif
    }

    bool run_once(int timeoutMs = 0) {
#if defined(__linux__)
        if (!running_) {
            return false;
        }

        epoll_event events[16]{};
        const int ready = ::epoll_wait(epollFd_, events, static_cast<int>(std::size(events)), timeoutMs);
        if (ready < 0) {
            return false;
        }

        for (int i = 0; i < ready; ++i) {
            if (events[i].data.fd == listenFd_) {
                accept_connections();
            } else {
                handle_connection(events[i].data.fd);
            }
        }

        return true;
#else
        (void)timeoutMs;
        return false;
#endif
    }

    void serve_forever() {
#if defined(__linux__)
        while (running_) {
            (void)run_once(1000);
        }
#endif
    }

private:
#if defined(__linux__)
    struct ConnectionState {
        int fd{-1};
        std::array<std::byte, BufferSize> buffer{};
        std::size_t used{0};
    };

    void accept_connections() {
        for (;;) {
            const int clientFd = ::accept4(listenFd_, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
            if (clientFd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return;
                }
                return;
            }

            int slot = -1;
            for (std::size_t i = 0; i < connections_.size(); ++i) {
                if (connections_[i].fd < 0) {
                    slot = static_cast<int>(i);
                    break;
                }
            }

            if (slot < 0) {
                ::close(clientFd);
                continue;
            }

            connections_[static_cast<std::size_t>(slot)].fd = clientFd;
            connections_[static_cast<std::size_t>(slot)].used = 0;

            epoll_event event{};
            event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
            event.data.fd = clientFd;
            if (::epoll_ctl(epollFd_, EPOLL_CTL_ADD, clientFd, &event) < 0) {
                ::close(clientFd);
                connections_[static_cast<std::size_t>(slot)] = {};
            }
        }
    }

    void handle_connection(int fd) {
        ConnectionState* connection = find_connection(fd);
        if (connection == nullptr) {
            ::epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
            ::close(fd);
            return;
        }

        std::array<std::byte, BufferSize> temp{};
        for (;;) {
            const ssize_t received = ::recv(fd, temp.data(), temp.size(), 0);
            if (received == 0) {
                close_connection(fd);
                return;
            }

            if (received < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }

                close_connection(fd);
                return;
            }

            append_and_parse(*connection, temp.data(), static_cast<std::size_t>(received));
        }
    }

    void append_and_parse(ConnectionState& connection, const std::byte* data, std::size_t size) {
        const std::size_t available = BufferSize - connection.used;
        const std::size_t toCopy = size > available ? available : size;
        std::memcpy(connection.buffer.data() + connection.used, data, toCopy);
        connection.used += toCopy;

        std::size_t offset = 0;
        while (connection.used - offset >= sizeof(WireOrderMessage)) {
            Order order{};
            if (!decode_wire_order(connection.buffer.data() + offset, connection.used - offset, order)) {
                offset += sizeof(WireOrderMessage);
                continue;
            }

            while (!queue_.push(order)) {
            }

            offset += sizeof(WireOrderMessage);
        }

        if (offset > 0) {
            const std::size_t remaining = connection.used - offset;
            if (remaining > 0) {
                std::memmove(connection.buffer.data(), connection.buffer.data() + offset, remaining);
            }
            connection.used = remaining;
        }
    }

    ConnectionState* find_connection(int fd) {
        for (auto& connection : connections_) {
            if (connection.fd == fd) {
                return &connection;
            }
        }

        return nullptr;
    }

    void close_connection(int fd) {
        ::epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
        for (auto& connection : connections_) {
            if (connection.fd == fd) {
                ::close(connection.fd);
                connection = {};
                return;
            }
        }
    }

    int listenFd_{-1};
    int epollFd_{-1};
    std::array<ConnectionState, 64> connections_{};
#endif

    std::uint16_t port_{};
    Queue& queue_;
    bool running_{false};
};

} // namespace exchange::network