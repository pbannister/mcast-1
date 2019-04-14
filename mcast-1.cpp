#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>

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

void print_address(const in_addr& a) {
	printf("Address : %15s\n", ::inet_ntoa(a));
}

void print_endpoint(const sockaddr_in& sa) {
	printf("Endpoint : %15s:%u\n", ::inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
}

//
//
//

class socket_in_base_s {
protected:
	int sock;
	ssize_t actual;
public:
	int error;
public:
	bool bind_to(sockaddr_in & sa) {
		int v = ::bind(sock, (const sockaddr*)&sa, sizeof(sa));
		error = errno;
		return 0 == v;
	}
	bool connect_to(sockaddr_in & sa) {
		int v = ::connect(sock, (const sockaddr*)&sa, sizeof(sa));
		error = errno;
		return 0 == v;
	}
public:
	bool send_to(sockaddr_in& sa, void* p, unsigned n, int flags = 0) {
		actual = ::sendto(sock, p, n, flags, (sockaddr*) &sa, sizeof(sa));
		error = errno;
		return 0 <= actual;
	}
	bool recv_from(sockaddr_in& sa, void* p_recv, size_t n_recv, size_t& n_actual, int flags = 0) {
		socklen_t n_sa = sizeof(sa);
		ssize_t v = ::recvfrom(sock, p_recv, n_recv, flags, (sockaddr*) &sa, &n_sa);
		error = errno;
		if (v < 0) {
			n_actual = 0;
			return false;
		}
//		assert(n_sa == sizeof(sa));
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
	socket_in_base_s() : sock(-1), actual(0), error(0) {
	}
	virtual ~socket_in_base_s() {
		socket_close();
	}
};

class socket_dgram_s : public socket_in_base_s {
public:
	bool socket_open() {
		sock = ::socket(AF_INET, SOCK_DGRAM, 0);
		error = errno;
		return (0 <= sock);
	}
};

class socket_mcast_s : public socket_dgram_s {
public:
	sockaddr_in sa_mcast;
public:
	socket_mcast_s() : sa_mcast(sockaddr_in()) {
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
		int v = ::setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &b, sizeof(b));
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
		int v = ::setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &ni, sizeof(ni));
		error = errno;
		return 0 == v;
	}
public:
	bool membership_add(in_addr& ni,  in_addr& mcast) {
		ip_mreq m;
		m.imr_interface = ni;
		m.imr_multiaddr = mcast;
		int v = ::setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &m, sizeof(m));
		error = errno;
		return 0 == v;
	}
	bool membership_drop(in_addr& ni,  in_addr& mcast) {
		ip_mreq m;
		m.imr_interface = ni;
		m.imr_multiaddr = mcast;
		int v = ::setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &m, sizeof(m));
		error = errno;
		return 0 == v;
	}
public:
	bool send_mcast(void* p, unsigned n, int flags = 0) {
		return send_to(sa_mcast, p, n, flags);
	}
};

//
//
//

struct options_s {
	const char* s_endpoint_bind;
	const char* s_endpoint_connect;
	const char* s_endpoint_send;
	const char* s_mcast_interface;
	const char* s_mcast_membership;
	int which;
} g_options = {
	"",
	"",
	"",
	"",
	"",
	0
};

bool test_1(socket_mcast_s& sock_out) {
	printf("\nTEST: Receive mcast messages and reply to sender.\n");
	char buffer1[1024];
	char buffer2[1024];
	for (;;) {

		printf("Waiting for message...\n");
		sockaddr_in sa = {0};
		size_t n_actual = 0;
		if (!sock_out.recv_from(sa, buffer1, sizeof(buffer1), n_actual)) {
			printf("ERROR: recvfrom() returns error: %s\n", sock_out.error_as_string());
			return false;
		}
		printf("Received: %s\n", buffer1);

		printf("Sending reply...\n");
		sprintf(buffer2, "ACK: %s\n", buffer1);
		if (!sock_out.send_to(sa, buffer2, std::strlen(buffer2))) {
			printf("ERROR: sendto() returns error: %s\n", sock_out.error_as_string());
			return false;
		}
		printf("Sent: %s\n", buffer2);

	}
	return true;
}

bool test_2(socket_mcast_s& sock_out) {
	printf("\nTEST: Send messages to mcast and receive messages.\n");
	char buffer1[1024];
	char buffer2[1024];
	int id_message = 0;
	for (;;) {

		printf("Sending message...\n");
		sprintf(buffer1, "HELO to whomever #%d\n", ++id_message);
		if (!sock_out.send_mcast(buffer1, std::strlen(buffer1))) {
			printf("ERROR: sendto() returns error: %s\n", sock_out.error_as_string());
			print_endpoint(sock_out.sa_mcast);
			return false;
		}
		printf("Sent: %s\n", buffer1);

		printf("Waiting for message...\n");
		sockaddr_in sa = {0};
		size_t n_actual = 0;
		if (!sock_out.recv_from(sa, buffer2, sizeof(buffer2), n_actual)) {
			printf("ERROR: recvfrom() returns error: %s\n", sock_out.error_as_string());
			return false;
		}
		printf("Received: %s\n", buffer2);

	}
	return true;
}

bool test_3(socket_mcast_s& sock_out) {
	printf("\nTEST: Send one mcast message, then note 'from' of reply.\nSend to 'from' and receive messages from any.\n");
	char buffer1[1024];
	char buffer2[1024];
	int id_message = 0;

	printf("Sending initial message...\n");
	sprintf(buffer1, "HELO to whomever #%d\n", ++id_message);
	if (!sock_out.send_mcast(buffer1, std::strlen(buffer1))) {
		printf("ERROR: sendto() returns error: %s\n", sock_out.error_as_string());
		return false;
	}
	printf("Sent: %s\n", buffer1);

	printf("Waiting for message...\n");
	sockaddr_in sa_from = {0};
	size_t n_actual = 0;
	if (!sock_out.recv_from(sa_from, buffer2, sizeof(buffer2), n_actual)) {
		printf("ERROR: recvfrom() returns error: %s\n", sock_out.error_as_string());
		return false;
	}
	printf("Received: %s\n", buffer2);
	printf("Will reply to sender, only.\n");

	for (;;) {

		printf("Sending message to initial from...\n");
		sprintf(buffer1, "HELO to whomever #%d\n", ++id_message);
		if (!sock_out.send_to(sa_from, buffer1, std::strlen(buffer1))) {
			printf("ERROR: sendto() returns error: %s\n", sock_out.error_as_string());
			return false;
		}
		printf("Sent: %s\n", buffer1);

		printf("Waiting for message...\n");
		sockaddr_in sa = {0};
		size_t n_actual = 0;
		if (!sock_out.recv_from(sa, buffer2, sizeof(buffer2), n_actual)) {
			printf("ERROR: recvfrom() returns error: %s\n", sock_out.error_as_string());
			return false;
		}
		printf("Received: %s\n", buffer2);

	}
	return true;
}

bool test_mc() {
	socket_mcast_s sock_out;
	if (!sock_out.socket_open()) {
		printf("ERROR: Cannot create socket.\n");
		return false;
	}
	if (*g_options.s_endpoint_bind) {
		sockaddr_in sa = {0};
		if (!parse_endpoint(g_options.s_endpoint_bind, sa)) {
			printf("ERROR: Cannot parse bind endpoint.\n");
			return false;
		}
		print_endpoint(sa);
		if (!sock_out.bind_to(sa)) {
			printf("ERROR: binding socket to endpoint: %s  error(%d): %s\n", g_options.s_endpoint_bind, sock_out.error, sock_out.error_as_string());
			return false;
		}
		printf("OK bind() to: %s\n", g_options.s_endpoint_bind);
	}
	if (*g_options.s_endpoint_connect) {
		sockaddr_in sa = {0};
		if (!parse_endpoint(g_options.s_endpoint_connect, sa)) {
			printf("ERROR: Cannot parse connect endpoint.\n");
			return false;
		}
		print_endpoint(sa);
		if (!sock_out.connect_to(sa)) {
			printf("ERROR: connecting socket to endpoint: %s  error(%d): %s\n", g_options.s_endpoint_connect, sock_out.error, sock_out.error_as_string());
			return false;
		}
		printf("OK bind() to: %s\n", g_options.s_endpoint_bind);
	}
	if (*g_options.s_endpoint_send) {
		if (!parse_endpoint(g_options.s_endpoint_send, sock_out.sa_mcast)) {
			printf("ERROR: Cannot parse send endpoint.\n");
			return false;
		}
		print_endpoint(sock_out.sa_mcast);
		printf("OK will sendto() to: %s\n", g_options.s_endpoint_send);
	}
	{
		in_addr interface_mcast = {0};
		if (sock_out.interface_get(interface_mcast)) {
			printf("Mcast interface is already set.\n");
			print_address(interface_mcast);
		} else {
			printf("Mcast interface not yet set.\n");
		}
		if (*g_options.s_mcast_interface) {
			if (!parse_address(g_options.s_mcast_interface, interface_mcast)) {
				printf("ERROR: Cannot parse interface.\n");
				return false;
			}
			print_address(interface_mcast);
			if (!sock_out.interface_set(interface_mcast)) {
				printf("ERROR: Cannot set mcast interface address: %s  error(%d): %s.\n", g_options.s_mcast_interface, sock_out.error, sock_out.error_as_string());
//				print_address(interface_mcast);
				return false;
			}
			printf("OK set interface as mcast: %s\n", g_options.s_mcast_interface);
		}
		if (*g_options.s_mcast_membership) {
			in_addr address_mcast = {0};
			if (!parse_address(g_options.s_mcast_membership, address_mcast)) {
				printf("ERROR: Cannot parse mcast membership address.\n");
				return false;
			}
			print_address(address_mcast);
			if (!sock_out.membership_add(interface_mcast, address_mcast)) {
				printf("ERROR: Cannot set mcast interface membership: %s  error(%d): %s\n", g_options.s_mcast_membership, sock_out.error, sock_out.error_as_string());
//				print_address(interface_mcast);
				return false;
			}
			printf("OK added mcast membership: %s to interface: %s\n", g_options.s_mcast_membership, g_options.s_mcast_interface);
		}
	}

	switch (g_options.which) {
	case 1:
		test_1(sock_out);
		return true;
	case 2:
		test_2(sock_out);
		return true;
	case 3:
		test_3(sock_out);
		return true;
	}

	return false;
}

void usage(const char* av0) {
	printf(
		"\nUsage:\n\t%s [ options ]\n"
		"\nWhere options are:\n"
		"\t-b endpoint      bind to endpoint  : %s\n"
		"\t-d endpoint      send to endpoint  : %s\n"
		"\t-i address       mcast interface   : %s\n"
		"\t-m address       mcast membership  : %s\n"
		"\t-1               test 1\n"
		"\t-2               test 2\n"
		"\t-3               test 3\n"
		"\n",
		av0,
		g_options.s_endpoint_bind,
		g_options.s_endpoint_send,
		g_options.s_mcast_interface,
		g_options.s_mcast_membership
	);
	exit(1);
}

void options_read(int ac, char** av) {
	for (;;) {
		int c = ::getopt(ac, av, "b:d:i:m:123h");
		if (c < 0) break;
		switch (c) {
		case 'b':
			g_options.s_endpoint_bind = optarg;
			break;
		case 'd':
			g_options.s_endpoint_send = optarg;
			break;
		case 'i':
			g_options.s_mcast_interface = optarg;
			break;
		case 'm':
			g_options.s_mcast_membership = optarg;
			break;
		case '1':
			g_options.which = 1;
			break;
		case '2':
			g_options.which = 2;
			break;
		case '3':
			g_options.which = 3;
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