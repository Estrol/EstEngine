#include <Exceptions/EstException.h>
using namespace Exceptions;

EstException::EstException(const char* message)
    : m_Message(message)
{
}

EstException::EstException(std::string message)
    : m_Message(message.c_str())
{
}

EstException::~EstException() throw()
{
}

const char* EstException::what() const throw()
{
    return m_Message;
}

