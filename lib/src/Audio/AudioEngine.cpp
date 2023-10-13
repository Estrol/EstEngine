#include <Audio/AudioEngine.h>
#include <Audio/AudioStream.h>
#include <Audio/AudioSample.h>
#include <Exceptions/EstException.h>

using namespace Audio;

Engine* Engine::s_Instance = nullptr;

Engine* Engine::Get() 
{
    if (s_Instance == nullptr) 
    {
        s_Instance = new Engine;
    }

    return s_Instance;
}

void Engine::Destroy() 
{
    if (s_Instance != nullptr) 
    {
        delete s_Instance;
    }
}

Engine::Engine() 
{
    
}

Engine::~Engine() 
{
    ma_context_uninit(&m_Context);
}

void Engine::Init()
{
    auto config = ma_context_config_init();
    config.threadPriority = ma_thread_priority_realtime;

    ma_backend backends[] = {
#if _WIN32
        ma_backend_wasapi
#else
	    ma_backend_pulseaudio
#endif
    };
    
    auto result = ma_context_init(backends, sizeof(backends) / sizeof(backends[0]), &config, &m_Context);
	if (result != MA_SUCCESS) {
		throw Exceptions::EstException("Failed to initialize audio context");
	}
}

Stream* Engine::LoadStream(std::filesystem::path path)
{
    auto stream = std::make_unique<Stream>();
    stream->Load(path);

    m_Streams.push_back(std::move(stream));
    return m_Streams.back().get();
}

Stream* Engine::LoadStream(const char* buf, size_t size)
{
    auto stream = std::make_unique<Stream>();
    stream->Load(buf, size);

    m_Streams.push_back(std::move(stream));
    return m_Streams.back().get();
}

Sample* Engine::LoadSample(std::filesystem::path path)
{
    auto sample = std::make_unique<Sample>();
    sample->Load(path);

    m_Samples.push_back(std::move(sample));
    return m_Samples.back().get();
}

Sample* Engine::LoadSample(const char* buf, size_t size)
{
    auto sample = std::make_unique<Sample>();
    sample->Load(buf, size);

    m_Samples.push_back(std::move(sample));
    return m_Samples.back().get();
}

void Engine::Destroy(Sample* sample)
{
    for (auto it = m_Samples.begin(); it != m_Samples.end(); it++) 
    {
        if (it->get() == sample) 
        {
            m_Samples.erase(it);
            break;
        }
    }
}

void Engine::Destroy(Stream* stream)
{
    for (auto it = m_Streams.begin(); it != m_Streams.end(); it++) 
    {
        if (it->get() == stream) 
        {
            m_Streams.erase(it);
            break;
        }
    }
}
ma_context *Audio::Engine::GetContext()
{
    return &m_Context;
}