#pragma once 

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <cstring>

//
//	Base class for INET sockets.
//

class socket_inet_base_s {
protected:
    int sock;
    ssize_t actual;
public:
    int error;
public:
    bool bind_to(sockaddr_in & sa) {
        int v = ::bind(sock, (const sockaddr*) &sa, sizeof(sa));
        error = errno;
        return 0 == v;
    }
    bool connect_to(sockaddr_in & sa) {
        int v = ::connect(sock, (const sockaddr*) &sa, sizeof(sa));
        error = errno;
        return 0 == v;
    }
public:
    bool send_to(sockaddr_in& sa, void* p, unsigned n, int flags = 0) {
        actual = ::sendto(sock, p, n, flags, (sockaddr*) &sa, sizeof(sa));
        error = errno;
        return 0 <= actual;
    }
    bool recv_from(sockaddr_in& sa, void* p_recv, size_t n_recv,
            size_t& n_actual, int flags = 0) {
        socklen_t n_sa = sizeof(sa);
        ssize_t v = ::recvfrom(sock, p_recv, n_recv, flags, (sockaddr*) &sa,
                &n_sa);
        error = errno;
        if (v < 0) {
            n_actual = 0;
            return false;
        }
        n_actual = v;
        return true;
    }
public:
    bool socket_close() {
        if (sock < 0) {
            return true;
        }
        ::close(sock);
        error = errno;
        sock = -1;
        return 0 == error;
    }
public:
    const char* error_as_string() {
        return ::strerror(error);
    }
public:
    socket_inet_base_s() :
            sock(-1), actual(0), error(0) {
    }
    virtual ~socket_inet_base_s() {
        socket_close();
    }
};

//
//	Base class for DGRAM sockets.
//

class socket_dgram_s: public socket_inet_base_s {
public:
    bool socket_open() {
        sock = ::socket(AF_INET, SOCK_DGRAM, 0);
        error = errno;
        return (0 <= sock);
    }
};

//
//	Base class for multicast sockets.
//

class socket_mcast_s: public socket_dgram_s {
public:
    sockaddr_in sa_mcast;
public:
    socket_mcast_s() :
            sa_mcast(sockaddr_in()) {
    }
public:
    bool loop_get(bool& b) {
        socklen_t len = sizeof(b);
        int v = ::getsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &b, &len);
        error = errno;
        return 0 == v;
    }
    bool loop_set(bool on) {
        char b = on;
        int v = ::setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &b,
                sizeof(b));
        error = errno;
        return 0 == v;
    }
public:
    bool ttl_get(bool& b) {
        socklen_t len = sizeof(b);
        int v = ::getsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &b, &len);
        error = errno;
        return 0 == v;
    }
    bool ttl_set(bool on) {
        char b = on;
        int v = ::setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &b, sizeof(b));
        error = errno;
        return 0 == v;
    }
public:
    bool interface_get(in_addr& ni) {
        socklen_t len = sizeof(ni);
        int v = ::getsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &ni, &len);
        error = errno;
        return 0 == v;
    }
    bool interface_set(in_addr& ni) {
        int v = ::setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &ni,
                sizeof(ni));
        error = errno;
        return 0 == v;
    }
public:
    bool membership_add(in_addr& ni, in_addr& mcast) {
        ip_mreq m;
        m.imr_interface = ni;
        m.imr_multiaddr = mcast;
        int v = ::setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &m,
                sizeof(m));
        error = errno;
        return 0 == v;
    }
    bool membership_drop(in_addr& ni, in_addr& mcast) {
        ip_mreq m;
        m.imr_interface = ni;
        m.imr_multiaddr = mcast;
        int v = ::setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &m,
                sizeof(m));
        error = errno;
        return 0 == v;
    }
public:
    bool send_mcast(void* p, unsigned n, int flags = 0) {
        return send_to(sa_mcast, p, n, flags);
    }
};
