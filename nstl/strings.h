#pragma once 

//
//	Free list of strings.
//

class string_free_list_s {

public:
	const unsigned n_size;

public:
	struct node_s {
		node_s* p_next;
		const unsigned n_size;
		char* s_buffer;
		node_s(unsigned _n) : p_next(0), n_size(_n), s_buffer(0) {
			s_buffer = new char[n_size];
			*s_buffer = 0;
		}
		~node_s() {
			delete s_buffer;
		}
	};

protected:
	node_s* p_free;

public:
	string_free_list_s(unsigned _n) : n_size(_n), p_free(0) {
	}
	~string_free_list_s() {
		while (p_free) {
			node_s* p = p_free;
			p_free = p->p_next;
			delete p;
		}
	}

public:
	node_s* node_get() {
		node_s* p = p_free;
		p_free = p->p_next;
		p->p_next = 0;
		*(p->s_buffer) = 0;
		return p;
	}
	node_s* node_get(unsigned _n) {
		if ((_n <= n_size) && p_free) {
			return node_get();
		}
		node_s* p = new node_s(n_size);
		return p;
	}
	void node_put(node_s* p) {
		if (n_size == p->n_size) {
			p->p_next = p_free;
			p_free = p;
		}
	}
};

//
//	Thin wrapper around standard C strings with free list.
//

class string_s {

protected:
	string_free_list_s::node_s* p_node;

protected:
	static string_free_list_s o_free;

public:
	string_s() : p_node(0) {
		p_node = o_free.node_get();
	}
	string_s(const char* s) : p_node(0) {
		p_node = o_free.node_get();
		strcpy(s);
	}
	~string_s() {
		o_free.node_put(p_node);
		p_node = 0;
	}

public:
	void string_room(unsigned n) {
		if (n <= p_node->n_size) {
			return;
		}
		o_free.node_put(p_node);
		p_node = o_free.node_get(n);
	}

public:
	void strcpy(const char* s) {
		int n = std::strlen(s);
		string_room(n+1);
		std::strcpy(p_node->s_buffer, s);
	}
	bool strcat(const char* s) {
		int n1 = std::strlen(p_node->s_buffer);
		int n2 = std::strlen(s);
		if ((n1 + n2) < p_node->n_size) {
			std::strcpy(p_node->s_buffer + n1, s);
			return true;
		}
		return false;
	}
	char* strchr(int c) {
		return std::strchr(p_node->s_buffer, c);
	}

public:
	char* as_buffer() {
		return p_node->s_buffer;
	}
	operator const char*() {
		return as_buffer();
	}

};
