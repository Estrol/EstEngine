#ifndef __ESTEXCEPTION_H_
#define __ESTEXCEPTION_H_

#include <exception>

namespace Exceptions
{
    class EstException : public std::exception
    {
    public:
        EstException(const char* message);
        virtual ~EstException() throw();
        virtual const char* what() const throw();
    private:
        const char* m_Message;
    };
}

#endif