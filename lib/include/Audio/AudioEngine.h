#ifndef __AUDIOENGINE_H_
#define __AUDIOENGINE_H_

#include "./Backend/miniaudio.h"
#include <filesystem>
#include <vector>
#include <memory>

namespace Audio {
    class Sample;
    class Stream;

    class Engine
    {
    public:
        void Init();

        Stream* LoadStream(std::filesystem::path path);
        Stream* LoadStream(const char* buf, size_t size);

        Sample* LoadSample(std::filesystem::path path);
        Sample* LoadSample(const char* buf, size_t size);

        void Destroy(Sample* sample);
        void Destroy(Stream* stream);

        ma_context* GetContext();

        static Engine* Get();
        static void Destroy();
    private:
        Engine();
        ~Engine();

        static Engine* s_Instance;

        std::vector<std::unique_ptr<Stream>> m_Streams;
        std::vector<std::unique_ptr<Sample>> m_Samples;

        ma_context m_Context;
    };
}

#endif