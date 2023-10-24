#ifndef __ESTEXCEPTION_H_
#define __ESTEXCEPTION_H_

#include <exception>
#include <string>

namespace Exceptions
{
    class EstException : public std::exception
    {
    public:
        EstException(const char* message);
        EstException(std::string message);
        virtual ~EstException() throw();
        virtual const char* what() const throw();
    private:
        const char* m_Message;
    };
}

#endif