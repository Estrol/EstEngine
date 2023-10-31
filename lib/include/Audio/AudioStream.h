#ifndef __AUDIOTRACK_H_
#define __AUDIOTRACK_H_

#include "Backend/miniaudio.h"
#include <filesystem>
#include <memory>
#define NOMINMAX
#include <signalsmith-stretch/signalsmith-stretch.h>
#include <vector>

namespace Audio {
    struct AudioData
    {
        signalsmith::stretch::SignalsmithStretch *Stretch;
        ma_engine                                 Engine;
        ma_decoder                                Decoder;
        ma_sound                                  Sound;
        bool                                      Playing;
        bool                                      Pitch;
        double                                    AudioRate = 1.0;

        // Used for own memory
        std::vector<char> AudioBuffer;

        // Used for store sample pointer data
        const char *Pointer;
        size_t      Size;

        // Temporary buffer for processing
        std::vector<float> TempBuffer;
        std::vector<std::vector<float>> InAudioChannels;
        std::vector<std::vector<float>> OutAudioChannels;
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
        void Load(const char *buf, size_t size);

        // Internal only for AudioSample
        void LoadMemory(const char *buf, size_t size);

    private:
        void Initialize();

        AudioData Data;
    };
} // namespace Audio

#endif