#pragma once

//
//
//

struct address_inet_s {
    in_addr address;
    static bool address_parse(const char*, in_addr& address);
    bool address_parse(const char*);
    static void address_print(in_addr&, const char*);
    void address_print(const char*);
};

struct endpoint_inet_s {
    sockaddr_in sa;
    static bool endpoint_parse(const char*, sockaddr_in&);
    bool endpoint_parse(const char*);
    static void endpoint_print(sockaddr_in&, const char*);
    void endpoint_print(const char*);
};

bool address_inet_s::address_parse(const char* s_address, in_addr& address) {
    int v = ::inet_aton(s_address, &address);
    if (0 == v) {
        printf("ERROR: Cannot parse address: %s\n", s_address);
        return false;
    }
    return true;
}

bool address_inet_s::address_parse(const char* s_address) {
    return address_parse(s_address, address);
}

void address_inet_s::address_print(in_addr& address, const char* name) {
    printf("%20s %5s - address %s\n", ::inet_ntoa(address), "", name);
}

void address_inet_s::address_print(const char* name) {
    address_print(address, name);
}

bool endpoint_inet_s::endpoint_parse(const char* _s, sockaddr_in& sa) {
    std::memset(&sa, 0, sizeof(sa));
    string_s s_buffer(_s);
    char* p1 = s_buffer.as_buffer();
    char* p2 = s_buffer.strrchr(':');
    if (!p2) {
        printf("ERROR: no :port\n");
        return false;
    }
    *p2++ = 0;

    const char* s_address = p1;
    const char* s_port = p2;

    long n_port = std::strtol(s_port, 0, 0);
    if (n_port < 0) {
        printf("ERROR: port too small\n");
        return false;
    }
    if ((1 << 16) <= n_port) {
        printf("ERROR: port too big\n");
        return false;
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons(n_port);

    return address_inet_s::address_parse(s_address, sa.sin_addr);
}

bool endpoint_inet_s::endpoint_parse(const char* _s) {
    return endpoint_parse(_s, sa);
}

void endpoint_inet_s::endpoint_print(sockaddr_in& sa, const char* name) {
    printf("%20s:%-5u - endpoint %s\n", ::inet_ntoa(sa.sin_addr),
            ntohs(sa.sin_port), name);
}

void endpoint_inet_s::endpoint_print(const char* name) {
    endpoint_print(sa, name);
}
