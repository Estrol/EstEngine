#include <Exceptions/EstException.h>
#include <Misc/Filesystem.h>
#include <fstream>
using namespace Misc;
using namespace std::filesystem;

std::vector<uint8_t> Filesystem::ReadFile(path path)
{
    if (!exists(path)) {
        throw Exceptions::EstException("Failed to open file: " + path.string());
    }

    std::fstream fs(path, std::ios::in | std::ios::binary);
    if (!fs.is_open()) {
        throw Exceptions::EstException("Failed to open file: " + path.string());
    }

    fs.seekg(0, std::ios::end);
    size_t size = fs.tellg();
    fs.seekg(0, std::ios::beg);

    std::vector<uint8_t> buf(size);
    fs.read((char *)buf.data(), size);
    fs.close();

    return buf;
}

std::vector<uint16_t> Filesystem::ReadFile16(path path)
{
    if (!exists(path)) {
        throw Exceptions::EstException("Failed to open file: " + path.string());
    }

    std::fstream fs(path, std::ios::in | std::ios::binary);
    if (!fs.is_open()) {
        throw Exceptions::EstException("Failed to open file: " + path.string());
    }

    fs.seekg(0, std::ios::end);
    size_t sizeInBytes = fs.tellg();
    fs.seekg(0, std::ios::beg);

    bool needsPadding = false;

    if (sizeInBytes % 2 != 0) {
        sizeInBytes++;
        needsPadding = true;
    }

    size_t sizeInU16 = sizeInBytes / 2;

    std::vector<uint16_t> buf(sizeInU16);
    fs.read((char *)buf.data(), sizeInBytes - (needsPadding ? 1 : 0));

    if (needsPadding) {
        buf.back() = 0;
    }

    fs.close();

    return buf;
}
