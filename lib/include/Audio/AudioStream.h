#ifndef __AUDIOTRACK_H_
#define __AUDIOTRACK_H_

#include "Backend/miniaudio.h"
#include <filesystem>
#include <vector>
#include <soundtouch/SoundTouch.h>

namespace Audio {
    struct AudioData {
        soundtouch::SoundTouch* SoundTouch;
        ma_engine Engine;
        ma_decoder Decoder;
        ma_sound Sound;
        bool Playing;
        bool Pitch;
        
        // Used for own memory
        std::vector<char> AudioBuffer;

        // Used for store sample pointer data
        const char* Pointer;
        size_t Size;

        // Temporary buffer for processing
        double AudioRate = 1.0;
        std::vector<float> TempBuffer;
    };

    class Stream
    {
    public:
        Stream();
        virtual ~Stream();

        void Play();
        void Stop();
        void Pause();
        void Seek(float seconds);

        void SetVolume(float volume);
        void SetRate(float pitch);
        void SetPitch(bool enable);
        
        float GetVolume() const;
        float GetLength() const;
        float GetCurrent() const;

        void Load(std::filesystem::path path);
        void Load(const char* buf, size_t size);

        // Internal only for AudioSample
        void LoadMemory(const char* buf, size_t size);

    private:
        void Initialize();
        
        AudioData Data;
    };
}

#endif