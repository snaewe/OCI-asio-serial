#ifndef __DDSEXCEPTION_H__
#define __DDSEXCEPTION_H__

#include <exception>
#include <sstream>

class DDSException : public std::exception {
	std::string _what;
public:
	DDSException(const char *what) : _what(what) {}
	~DDSException() throw() {}
	const char *what() const throw() { return _what.c_str(); }
};

#endif
