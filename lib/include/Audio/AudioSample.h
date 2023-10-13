#ifndef __AUDIOSAMPLE_H_
#define __AUDIOSAMPLE_H_

#include <vector>
#include <memory>
#include <filesystem>

namespace Audio {
    class Stream;

    class Sample
    {
    public:
        Sample();
        virtual ~Sample();

        void Load(std::filesystem::path path);
        void Load(const char* buf, size_t size);

        Stream* GetStream();
    private:
        std::vector<std::unique_ptr<Stream>> m_Streams;
        std::vector<char> m_Buffer;
    };
}

#endif