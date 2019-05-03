#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>

#include "nstl/strings.h"
#include "nstl/arrayopts.h"
#include "nstl/sockets.h"
#include "nstl/endpoints.h"

string_free_list_s string_s::o_free(256);

//
//
//

struct options_s {
    const char* s_endpoint_bind;
    const char* s_endpoint_connect;
    const char* s_endpoint_send;
    array_opt_s a_mcast_interface;
    array_opt_s a_mcast_membership;
    int which;
    int dt_client;
} g_options = { "", "", "", 10, 10, 0, 1 };

bool test_1(socket_mcast_s& sock) {
    printf("\nTEST: Receive mcast messages and reply to sender.\n");
    char buffer1[1024];
    char buffer2[1024];
    for (;;) {

        printf("Waiting for message...\n");
        endpoint_inet_s endpoint1;
        size_t n_actual = 0;
        if (!sock.recv_from(endpoint1.sa, buffer1, sizeof(buffer1), n_actual)) {
            printf("ERROR: recvfrom() returns error: %s\n",
                    sock.error_as_string());
            return false;
        }
        endpoint1.endpoint_print("read");
        printf("Received: %s\n", buffer1);

        printf("Sending reply...\n");
        endpoint1.endpoint_print("write");
        sprintf(buffer2, "ACK: %s\n", buffer1);
        if (!sock.send_to(endpoint1.sa, buffer2, 1 + std::strlen(buffer2))) {
            printf("ERROR: sendto() returns error: %s\n",
                    sock.error_as_string());
            return false;
        }
        printf("Sent: %s\n", buffer2);

    }
    return true;
}

bool test_2(socket_mcast_s& sock) {
    printf("\nTEST: Send messages to mcast and receive messages.\n");
    char buffer1[1024];
    char buffer2[1024];
    int id_message = 0;
    for (;;) {

        printf("Sending message...\n");
        endpoint_inet_s::endpoint_print(sock.sa_mcast, "write");
        sprintf(buffer1, "HELO to whomever #%d\n", ++id_message);
        if (!sock.send_mcast(buffer1, 1 + std::strlen(buffer1))) {
            printf("ERROR: sendto() returns error: %s\n",
                    sock.error_as_string());
            return false;
        }
        printf("Sent: %s\n", buffer1);

        printf("Waiting for message...\n");
        endpoint_inet_s endpoint1;
        size_t n_actual = 0;
        if (!sock.recv_from(endpoint1.sa, buffer2, sizeof(buffer2), n_actual)) {
            printf("ERROR: recvfrom() returns error: %s\n",
                    sock.error_as_string());
            return false;
        }
        endpoint1.endpoint_print("read");
        printf("Received: %s\n", buffer2);

        sleep(g_options.dt_client);
    }
    return true;
}

bool test_3(socket_mcast_s& sock) {
    printf("\nTEST: Send one mcast message, then note 'from' of reply.\nSend to 'from' and receive messages from any.\n");
    char buffer1[1024];
    char buffer2[1024];
    int id_message = 0;

    printf("Sending initial message...\n");
    endpoint_inet_s::endpoint_print(sock.sa_mcast, "write");
    sprintf(buffer1, "HELO to whomever #%d\n", ++id_message);
    if (!sock.send_mcast(buffer1, 1 + std::strlen(buffer1))) {
        printf("ERROR: sendto() returns error: %s\n", sock.error_as_string());
        return false;
    }
    printf("Sent: %s\n", buffer1);

    printf("Waiting for message...\n");
    endpoint_inet_s endpoint1;
    size_t n_actual = 0;
    if (!sock.recv_from(endpoint1.sa, buffer2, sizeof(buffer2), n_actual)) {
        printf("ERROR: recvfrom() returns error: %s\n", sock.error_as_string());
        return false;
    }
    endpoint1.endpoint_print("read");
    printf("Received: %s\n", buffer2);
    printf("Will reply to sender, only.\n");

    for (;;) {

        printf("Sending message to initial from...\n");
        endpoint1.endpoint_print("write");
        sprintf(buffer1, "HELO to whomever #%d\n", ++id_message);
        if (!sock.send_to(endpoint1.sa, buffer1, 1 + std::strlen(buffer1))) {
            printf("ERROR: sendto() returns error: %s\n",
                    sock.error_as_string());
            return false;
        }
        printf("Sent: %s\n", buffer1);

        printf("Waiting for message...\n");
        
        endpoint_inet_s endpoint2;
        size_t n_actual = 0;
        if (!sock.recv_from(endpoint2.sa, buffer2, sizeof(buffer2), n_actual)) {
            printf("ERROR: recvfrom() returns error: %s\n",
                    sock.error_as_string());
            return false;
        }
        endpoint2.endpoint_print("read");
        printf("Received: %s\n", buffer2);

    }
    return true;
}

void print_error(socket_inet_base_s& o, const char* name) {
    printf("ERROR: %s - error(%d): %s\n", name, o.error, o.error_as_string());
}

bool socket_configure(socket_mcast_s& sock) {
    if (!sock.socket_open()) {
        printf("ERROR: Cannot create socket.\n");
        return false;
    }
    if (*g_options.s_endpoint_bind) {
        endpoint_inet_s endpoint_bind;
        if (!endpoint_bind.endpoint_parse(g_options.s_endpoint_bind)) {
            printf("ERROR: Cannot parse bind endpoint.\n");
            return false;
        }
        endpoint_bind.endpoint_print("bind");
        if (!sock.bind_to(endpoint_bind.sa)) {
            print_error(sock, "binding socket to endpoint");
            return false;
        }
        printf("OK bind\n");
    }
    if (*g_options.s_endpoint_connect) {
        endpoint_inet_s endpoint_connect;
        if (!endpoint_connect.endpoint_parse(g_options.s_endpoint_connect)) {
            printf("ERROR: Cannot parse connect endpoint.\n");
            return false;
        }
        endpoint_connect.endpoint_print("connect");
        if (!sock.connect_to(endpoint_connect.sa)) {
            print_error(sock, "connecting socket to endpoint");
            return false;
        }
        printf("OK connect\n");
    }
    if (*g_options.s_endpoint_send) {
        if (!endpoint_inet_s::endpoint_parse(g_options.s_endpoint_send, sock.sa_mcast)) {
            printf("ERROR: Cannot parse send endpoint.\n");
            return false;
        }
        endpoint_inet_s::endpoint_print(sock.sa_mcast, "mcast");
        printf("OK destination\n");
    }
    {
        address_inet_s mcast_interface;
        if (sock.interface_get(mcast_interface.address)) {
            mcast_interface.address_print("mcast interface (existing)");
        } else {
            printf("Mcast interface not yet set.\n");
        }
        for (unsigned i = 0; i < g_options.a_mcast_interface.array_size(); ++i) {
            const char* s_mcast_interface = g_options.a_mcast_interface[i];
            address_inet_s mcast_interface;
            if (!mcast_interface.address_parse(s_mcast_interface)) {
                printf("ERROR: Cannot parse interface.\n");
                return false;
            }
            mcast_interface.address_print("mcast interface (specified)");
            if (!sock.interface_set(mcast_interface.address)) {
                print_error(sock, "cannot set mcast interface address");
                return false;
            }
            printf("OK set interface as mcast\n");
            // Verify the interface is what we just set.
            if (sock.interface_get(mcast_interface.address)) {
                mcast_interface.address_print("mcast interface (as set)");
            } else {
                printf("Mcast interface not set??\n");
            }
        }
        for (unsigned i = 0; i < g_options.a_mcast_interface.array_size(); ++i) {
            address_inet_s mcast_interface;
            address_inet_s mcast_address;
            string_s buffer(g_options.a_mcast_membership[i]);
            char* p_membership = buffer.as_buffer();
            char* p_interface = buffer.strchr('@');
            if (p_interface) {
                *p_interface++ = 0;
                if (!mcast_interface.address_parse(p_interface)) {
                    printf("ERROR: Cannot parse membership interface.\n");
                    return false;
                }
            }
            if (!mcast_address.address_parse(p_membership)) {
                printf("ERROR: Cannot parse mcast membership address.\n");
                return false;
            }
            mcast_interface.address_print("mcast interface (specified for membership)");
            mcast_address.address_print("mcast membership");
            if (!sock.membership_add(mcast_interface.address, mcast_address.address)) {
                print_error(sock, "cannot set mcast interface membership");
                return false;
            }
            printf("OK added mcast membership to interface\n");
        }
    }
    return true;
}

bool test_mc() {
    socket_mcast_s sock;
    socket_configure(sock);
    switch (g_options.which) {
    case 1:
        test_1(sock);
        return true;
    case 2:
        test_2(sock);
        return true;
    case 3:
        test_3(sock);
        return true;
    }
    return false;
}

void usage(const char* av0) {
    printf("\nUsage:\n\t%s [ options ]\n"
            "\nWhere options are:\n"
            "\t-b endpoint         bind to endpoint  : %s\n"
            "\t-d endpoint         send to endpoint  : %s\n"
            "\t-i address          mcast interface   : %s\n"
            "\t-m mcast@interface  mcast membership  : %s\n"
            "\t-t seconds          client sleep      : %d\n"
            "\t-1                  test 1\n"
            "\t-2                  test 2\n"
            "\n", av0, g_options.s_endpoint_bind, g_options.s_endpoint_send,
            g_options.a_mcast_interface.serialize(", "),
            g_options.a_mcast_membership.serialize(", "), g_options.dt_client);
    exit(1);
}

void options_read(int ac, char** av) {
    for (;;) {
        int c = ::getopt(ac, av, "b:d:i:m:t:12h");
        if (c < 0)
            break;
        switch (c) {
        case 'b':
            g_options.s_endpoint_bind = optarg;
            break;
        case 'd':
            g_options.s_endpoint_send = optarg;
            break;
        case 'i':
            g_options.a_mcast_interface.array_add(optarg);
            break;
        case 'm':
            g_options.a_mcast_membership.array_add(optarg);
            break;
        case 't':
            g_options.dt_client = atoi(optarg);
            break;
        case '1':
            g_options.which = 1;
            break;
        case '2':
            g_options.which = 2;
            break;
        default:
        case 'h':
            usage(*av);
        }
    }
    if (g_options.which) {
        // Good. We are doing something.
    } else {
        printf("ERROR: no test chosen.\n");
        usage(*av);
    }
}

int main(int ac, char** av) {
    options_read(ac, av);
    test_mc();
    return 0;
}
