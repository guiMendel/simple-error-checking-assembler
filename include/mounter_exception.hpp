#ifndef __MOUNTER_EXCEPTION__
#define __MOUNTER_EXCEPTION__

#include <iostream>
#include <exception>

class MounterException : public std::exception {
    protected:
    const int line;
    const std::string type;
    const std::string message;

    public:
    MounterException(int line, std::string type, std::string message) :
        message(message),
        type(type),
        line(line)
        {}

    const char* what() const noexcept {return message.c_str();}
    const char* get_type() const {return type.c_str();}
    const int get_line() const {return line;}
};

#endif