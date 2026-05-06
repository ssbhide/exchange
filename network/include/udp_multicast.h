#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "protocol.h"

namespace exchange::network {

class UdpMulticastSocket {
public:
    UdpMulticastSocket(std::string multicastAddress, std::uint16_t port, std::string interfaceAddress = {})
        : multicastAddress_(std::move(multicastAddress)), interfaceAddress_(std::move(interfaceAddress)), port_(port) {
        open();
    }

    ~UdpMulticastSocket() {
        if (socketFd_ >= 0) {
            ::close(socketFd_);
        }
    }

    bool valid() const {
        return socketFd_ >= 0;
    }

    bool send_execution(const Execution& execution, Side side) {
        if (!valid()) {
            return false;
        }

        const WireExecutionMessage message = encode_wire_execution(execution, side);
        const sockaddr_in destination = multicast_endpoint();
        const ssize_t sent = ::sendto(socketFd_, &message, sizeof(message), 0,
            reinterpret_cast<const sockaddr*>(&destination), sizeof(destination));
        return sent == static_cast<ssize_t>(sizeof(message));
    }

    int native_handle() const {
        return socketFd_;
    }

private:
    void open() {
        socketFd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (socketFd_ < 0) {
            return;
        }

        int reuse = 1;
        (void)::setsockopt(socketFd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        unsigned char ttl = 1;
        (void)::setsockopt(socketFd_, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

        unsigned char loopback = 1;
        (void)::setsockopt(socketFd_, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback, sizeof(loopback));

        if (!interfaceAddress_.empty()) {
            in_addr interfaceAddr{};
            if (::inet_pton(AF_INET, interfaceAddress_.c_str(), &interfaceAddr) == 1) {
                (void)::setsockopt(socketFd_, IPPROTO_IP, IP_MULTICAST_IF, &interfaceAddr, sizeof(interfaceAddr));
            }
        }
    }

    sockaddr_in multicast_endpoint() const {
        sockaddr_in destination{};
        destination.sin_family = AF_INET;
        destination.sin_port = htons(port_);
        ::inet_pton(AF_INET, multicastAddress_.c_str(), &destination.sin_addr);
        return destination;
    }

    int socketFd_{-1};
    std::string multicastAddress_;
    std::string interfaceAddress_;
    std::uint16_t port_{};
};

class UdpMulticastReceiver {
public:
    UdpMulticastReceiver(std::string multicastAddress, std::uint16_t port, std::string interfaceAddress = {})
        : multicastAddress_(std::move(multicastAddress)), interfaceAddress_(std::move(interfaceAddress)), port_(port) {
        open();
    }

    ~UdpMulticastReceiver() {
        if (socketFd_ >= 0) {
            ::close(socketFd_);
        }
    }

    bool valid() const {
        return socketFd_ >= 0;
    }

    ssize_t receive(std::byte* buffer, std::size_t size) {
        if (!valid()) {
            return -1;
        }

        return ::recv(socketFd_, buffer, size, 0);
    }

private:
    void open() {
        socketFd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (socketFd_ < 0) {
            return;
        }

        int reuse = 1;
        (void)::setsockopt(socketFd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        sockaddr_in bindAddress{};
        bindAddress.sin_family = AF_INET;
        bindAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        bindAddress.sin_port = htons(port_);
        if (::bind(socketFd_, reinterpret_cast<sockaddr*>(&bindAddress), sizeof(bindAddress)) < 0) {
            ::close(socketFd_);
            socketFd_ = -1;
            return;
        }

        ip_mreq membership{};
        ::inet_pton(AF_INET, multicastAddress_.c_str(), &membership.imr_multiaddr);
        if (interfaceAddress_.empty()) {
            membership.imr_interface.s_addr = htonl(INADDR_ANY);
        } else {
            ::inet_pton(AF_INET, interfaceAddress_.c_str(), &membership.imr_interface);
        }

        if (::setsockopt(socketFd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &membership, sizeof(membership)) < 0) {
            ::close(socketFd_);
            socketFd_ = -1;
        }
    }

    int socketFd_{-1};
    std::string multicastAddress_;
    std::string interfaceAddress_;
    std::uint16_t port_{};
};

} // namespace exchange::network