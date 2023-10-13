#include <Audio/AudioSample.h>
#include <Audio/AudioStream.h>
#include <Exceptions/EstException.h>
#include <fstream>
using namespace Audio;

Sample::Sample()
{
}

Sample::~Sample()
{
}

void Sample::Load(std::filesystem::path path)
{
    if (!std::filesystem::exists(path)) 
    {
        throw Exceptions::EstException("Failed to load audio sample");
    }

    std::fstream fs(path, std::ios::binary | std::ios::in);

    fs.seekg(0, std::ios::end);
    size_t size = fs.tellg();
    fs.seekg(0, std::ios::beg);

    m_Buffer.resize(size);
    fs.read(m_Buffer.data(), size);

    fs.close();
}

void Sample::Load(const char* buf, size_t size)
{
    if (!buf) 
    {
        throw Exceptions::EstException("Failed to load audio sample");
    }

    m_Buffer.resize(size);
    memcpy(m_Buffer.data(), buf, size);
}

Stream* Sample::GetStream()
{
    auto stream = std::make_unique<Stream>();
    stream->LoadMemory(m_Buffer.data(), m_Buffer.size());

    m_Streams.push_back(std::move(stream));
    return m_Streams.back().get();
}