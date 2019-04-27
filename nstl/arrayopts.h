#pragma once 

//
//	Array for use with command line options.
//

class array_opt_s {

protected:
	typedef const char* opt_t;

protected:
	const unsigned n_room;
	unsigned n_size;
	opt_t* p_array;

protected:
	unsigned n_sum;
	char* s_buffer;

public:
	unsigned array_size() {
		return n_room;
	}
	bool array_add(const char* s) {
		if (n_room <= n_size) {
			return false;
		}
		p_array[n_size++] = s;
		n_sum += std::strlen(s);
		return true;
	}
	const char* serialize(const char* s_join) {
		if (0 == n_size) {
			return "";
		}
		delete s_buffer;
		unsigned n = 1 + n_sum + (std::strlen(s_join) * (n_size - 1));
		s_buffer = new char[n];
		char* p = s_buffer;
		std::strcpy(p, p_array[0]);
		p += std::strlen(p);
		for (unsigned i = 1; i < n_size; ++i) {
			std::strcat(p, s_join);
			std::strcat(p, p_array[i]);
			p += std::strlen(p);
		}
		return s_buffer;
	}

public:
	const char* operator[] (unsigned i) {
		return ((i < n_size) ? p_array[i] : 0);
	}
	operator const char*() {
		return serialize(" ");
	}

public:
	array_opt_s() : n_room(0), n_size(0), p_array(0), n_sum(0), s_buffer(0) {}
	array_opt_s(unsigned _n) : n_room(_n), n_size(0), p_array(0), n_sum(0), s_buffer(0) {
		p_array = new opt_t[n_room];
	}
	array_opt_s(const array_opt_s& o) : n_room(0), n_size(0), p_array(0), n_sum(0), s_buffer(0) {
		p_array = new opt_t[n_room];
	}
	~array_opt_s() {
		delete p_array;
		delete s_buffer;
	}

private:
	void operator=(const array_opt_s&) {
		// nope
	}
};
// Yes. I have heard of STL.
