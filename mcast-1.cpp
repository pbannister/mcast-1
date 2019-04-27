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

string_free_list_s string_s::o_free(256);

//
//
//

bool parse_address(const char* s_address, in_addr& address) {
	int v = ::inet_aton(s_address, &address);
	if (0 == v) {
		printf("ERROR: Cannot parse address: %s\n", s_address);
		return false;
	}
	return true;
}

bool parse_endpoint(const char* s_endpoint, sockaddr_in& sa) {
	std::memset(&sa, 0, sizeof(sa));

	const char* p = std::strrchr(s_endpoint, ':');
	if (!p) {
		printf("ERROR: no :port\n");
		return false;
	}

	int n_address = (int)(p - s_endpoint);
	char s_address[n_address+1];
	std::strncpy(s_address, s_endpoint, n_address);
	s_address[n_address] = 0;
	const char* s_port = p + 1;

	long n_port = std::strtol(s_port, 0, 0);
	if (n_port < 0) {
		printf("ERROR: port too small\n");
		return false;
	}
	if ((1<<16) <= n_port) {
		printf("ERROR: port too big\n");
		return false;
	}

	sa.sin_family = AF_INET;
	sa.sin_port = htons(n_port);

	return parse_address(s_address, sa.sin_addr);
}

void print_address(const in_addr& a, const char* name) {
	printf("%20s %5s - address %s\n", ::inet_ntoa(a), "", name);
}

void print_endpoint(const sockaddr_in& sa, const char* name) {
	printf("%20s:%-5u - endpoint %s\n", ::inet_ntoa(sa.sin_addr), ntohs(sa.sin_port), name);
}

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
} g_options = {
	"",
	"",
	"",
	10,
	10,
	0,
	1
};

bool test_1(socket_mcast_s& sock) {
	printf("\nTEST: Receive mcast messages and reply to sender.\n");
	char buffer1[1024];
	char buffer2[1024];
	for (;;) {

		printf("Waiting for message...\n");
		sockaddr_in sa = {0};
		size_t n_actual = 0;
		if (!sock.recv_from(sa, buffer1, sizeof(buffer1), n_actual)) {
			printf("ERROR: recvfrom() returns error: %s\n", sock.error_as_string());
			return false;
		}
		print_endpoint(sa, "read");
		printf("Received: %s\n", buffer1);

		printf("Sending reply...\n");
		print_endpoint(sa, "write");
		sprintf(buffer2, "ACK: %s\n", buffer1);
		if (!sock.send_to(sa, buffer2, 1+std::strlen(buffer2))) {
			printf("ERROR: sendto() returns error: %s\n", sock.error_as_string());
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
		print_endpoint(sock.sa_mcast, "write");
		sprintf(buffer1, "HELO to whomever #%d\n", ++id_message);
		if (!sock.send_mcast(buffer1, 1+std::strlen(buffer1))) {
			printf("ERROR: sendto() returns error: %s\n", sock.error_as_string());
			return false;
		}
		printf("Sent: %s\n", buffer1);

		printf("Waiting for message...\n");
		sockaddr_in sa = {0};
		size_t n_actual = 0;
		if (!sock.recv_from(sa, buffer2, sizeof(buffer2), n_actual)) {
			printf("ERROR: recvfrom() returns error: %s\n", sock.error_as_string());
			return false;
		}
		print_endpoint(sa, "read");
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
	print_endpoint(sock.sa_mcast, "write");
	sprintf(buffer1, "HELO to whomever #%d\n", ++id_message);
	if (!sock.send_mcast(buffer1, 1+std::strlen(buffer1))) {
		printf("ERROR: sendto() returns error: %s\n", sock.error_as_string());
		return false;
	}
	printf("Sent: %s\n", buffer1);

	printf("Waiting for message...\n");
	sockaddr_in sa_from = {0};
	size_t n_actual = 0;
	if (!sock.recv_from(sa_from, buffer2, sizeof(buffer2), n_actual)) {
		printf("ERROR: recvfrom() returns error: %s\n", sock.error_as_string());
		return false;
	}
	print_endpoint(sa_from, "read");
	printf("Received: %s\n", buffer2);
	printf("Will reply to sender, only.\n");

	for (;;) {

		printf("Sending message to initial from...\n");
		print_endpoint(sa_from, "write");
		sprintf(buffer1, "HELO to whomever #%d\n", ++id_message);
		if (!sock.send_to(sa_from, buffer1, 1+std::strlen(buffer1))) {
			printf("ERROR: sendto() returns error: %s\n", sock.error_as_string());
			return false;
		}
		printf("Sent: %s\n", buffer1);

		printf("Waiting for message...\n");
		sockaddr_in sa = {0};
		size_t n_actual = 0;
		if (!sock.recv_from(sa, buffer2, sizeof(buffer2), n_actual)) {
			printf("ERROR: recvfrom() returns error: %s\n", sock.error_as_string());
			return false;
		}
		print_endpoint(sa, "read");
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
		sockaddr_in sa = {0};
		if (!parse_endpoint(g_options.s_endpoint_bind, sa)) {
			printf("ERROR: Cannot parse bind endpoint.\n");
			return false;
		}
		print_endpoint(sa, "bind");
		if (!sock.bind_to(sa)) {
			print_error(sock, "binding socket to endpoint");
			return false;
		}
		printf("OK bind\n");
	}
	if (*g_options.s_endpoint_connect) {
		sockaddr_in sa = {0};
		if (!parse_endpoint(g_options.s_endpoint_connect, sa)) {
			printf("ERROR: Cannot parse connect endpoint.\n");
			return false;
		}
		print_endpoint(sa, "connect");
		if (!sock.connect_to(sa)) {
			print_error(sock, "connecting socket to endpoint");
			return false;
		}
		printf("OK connect\n");
	}
	if (*g_options.s_endpoint_send) {
		if (!parse_endpoint(g_options.s_endpoint_send, sock.sa_mcast)) {
			printf("ERROR: Cannot parse send endpoint.\n");
			return false;
		}
		print_endpoint(sock.sa_mcast, "mcast");
		printf("OK destination\n");
	}
	{
		in_addr interface_mcast = {0};
		if (sock.interface_get(interface_mcast)) {
			print_address(interface_mcast, "mcast interface (existing)");
		} else {
			printf("Mcast interface not yet set.\n");
		}
		for (unsigned i = 0; i < g_options.a_mcast_interface.array_size(); ++i) {
			const char* s_mcast_interface = g_options.a_mcast_interface[i];
			if (!parse_address(s_mcast_interface, interface_mcast)) {
				printf("ERROR: Cannot parse interface.\n");
				return false;
			}
			print_address(interface_mcast, "mcast interface (specified)");
			if (!sock.interface_set(interface_mcast)) {
				print_error(sock, "cannot set mcast interface address");
				return false;
			}
			printf("OK set interface as mcast\n");
			// Verify the interface is what we just set.
			if (sock.interface_get(interface_mcast)) {
				print_address(interface_mcast, "mcast interface (as set)");
			} else {
				printf("Mcast interface not set??\n");
			}
		}
		for (unsigned i = 0; i < g_options.a_mcast_interface.array_size(); ++i) {
			string_s buffer(g_options.a_mcast_membership[i]);
			char* p_membership = buffer.as_buffer();
			char* p_interface = buffer.strchr('@');
			if (p_interface) {
				*p_interface++ = 0;
				if (!parse_address(p_interface, interface_mcast)) {
					printf("ERROR: Cannot parse membership interface.\n");
					return false;
				}
			}
			in_addr address_mcast = {0};
			if (!parse_address(p_membership, address_mcast)) {
				printf("ERROR: Cannot parse mcast membership address.\n");
				return false;
			}
			print_address(interface_mcast, "mcast interface (specified for membership)");
			print_address(address_mcast, "mcast membership");
			if (!sock.membership_add(interface_mcast, address_mcast)) {
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
	printf(
		"\nUsage:\n\t%s [ options ]\n"
		"\nWhere options are:\n"
		"\t-b endpoint         bind to endpoint  : %s\n"
		"\t-d endpoint         send to endpoint  : %s\n"
		"\t-i address          mcast interface   : %s\n"
		"\t-m mcast@interface  mcast membership  : %s\n"
		"\t-t seconds          client sleep      : %d\n"
		"\t-1                  test 1\n"
		"\t-2                  test 2\n"
		"\n",
		av0,
		g_options.s_endpoint_bind,
		g_options.s_endpoint_send,
		g_options.a_mcast_interface.serialize(", "),
		g_options.a_mcast_membership.serialize(", "),
		g_options.dt_client
	);
	exit(1);
}

void options_read(int ac, char** av) {
	for (;;) {
		int c = ::getopt(ac, av, "b:d:i:m:t:12h");
		if (c < 0) break;
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
