#define MINIAUDIO_IMPLEMENTATION
#include <Audio/AudioEngine.h>
#include <Audio/AudioStream.h>
#include <Exceptions/EstException.h>
#include <fstream>

using namespace Audio;

Stream::Stream()
{
    Data = {};
    // Data.SoundTouch = new soundtouch::SoundTouch();
    Data.Stretch = new signalsmith::stretch::SignalsmithStretch();
}

Stream::~Stream()
{
    ma_sound_uninit(&Data.Sound);
    ma_engine_uninit(&Data.Engine);
    ma_decoder_uninit(&Data.Decoder);

    // Data.SoundTouch->flush();
    // Data.SoundTouch->clear();
    // delete Data.SoundTouch;
    delete Data.Stretch;
}

void Stream::Load(std::filesystem::path path)
{
    if (!std::filesystem::exists(path)) {
        throw Exceptions::EstException("Failed to load audio stream");
    }

    std::fstream fs(path, std::ios::binary | std::ios::in);

    fs.seekg(0, std::ios::end);
    size_t size = fs.tellg();
    fs.seekg(0, std::ios::beg);

    Data.AudioBuffer.resize(size);
    fs.read(Data.AudioBuffer.data(), size);

    fs.close();

    Initialize();
}

void Stream::Load(const char *buf, size_t size)
{
    if (!buf) {
        throw Exceptions::EstException("Failed to load audio stream");
    }

    Data.AudioBuffer.resize(size);
    memcpy(Data.AudioBuffer.data(), buf, size);

    Initialize();
}

void Stream::LoadMemory(const char *buf, size_t size)
{
    if (!buf) {
        throw Exceptions::EstException("Failed to load audio stream from memory");
    }

    Data.Pointer = buf;
    Data.Size = size;

    Initialize();
}

// Audio data processing
// TODO: use static array instead of dynamic array?
void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    // using namespace soundtouch;

    ma_engine *pEngine = (ma_engine *)pDevice->pUserData;
    if (!pEngine) {
        return;
    }

    // HACK!!
    AudioData *pData = (AudioData *)pEngine->pProcessUserData;

    if (pData->Pitch) {
        ma_engine_read_pcm_frames(pEngine, pOutput, frameCount, nullptr);
    } else {
        ma_uint64 frameToRead = frameCount;
        ma_uint64 channelFrameToRead = frameToRead * pData->Decoder.outputChannels;

        if (pData->TempBuffer.size() < channelFrameToRead) {
            pData->TempBuffer.resize(channelFrameToRead);
        }

        ma_uint64 sampleReaded = 0;
        auto      result = ma_engine_read_pcm_frames(pEngine, pData->TempBuffer.data(), frameToRead, &sampleReaded);

        if (!sampleReaded)
            return;

        auto &in = pData->InAudioChannels;
        auto &out = pData->OutAudioChannels;

        if (in.size() != pData->Decoder.outputChannels) {
            in.resize(pData->Decoder.outputChannels);
            out.resize(in.size());
        }

        for (int i = 0; i < in.size(); i++) {
            if (in[i].size() < sampleReaded) {
                in[i].resize(sampleReaded);
            }

            if (out[i].size() < frameCount) {
                out[i].resize(frameCount);
            }
        }

        // copy data from temp buffer to input buffer based on channel size
        for (ma_uint32 i = 0; i < pData->Decoder.outputChannels; i++) {
            for (ma_uint32 j = 0; j < frameCount; j++) {
                int index = (j * pData->Decoder.outputChannels) + i;
                // check out of bounds;
                if (index >= pData->Decoder.outputChannels * sampleReaded)
                    __debugbreak();

                in[i][j] = pData->TempBuffer[index];
            }
        }

        pData->Stretch->process(in, (int)sampleReaded, out, (int)frameCount);

        // copy data from output buffer to pOutput based on channel size
        for (ma_uint32 i = 0; i < pData->Decoder.outputChannels; i++) {
            for (ma_uint32 j = 0; j < frameCount; j++) {
                unsigned int index = (j * pData->Decoder.outputChannels) + i;
                // check out of bounds;
                if (index >= pData->Decoder.outputChannels * frameCount)
                    __debugbreak();

                ((float *)pOutput)[index] = out[i][j];
            }
        }
    }
}

void Stream::Initialize()
{
    auto engine = Engine::Get()->GetContext();

    ma_decoder_config config = ma_decoder_config_init_default();
    config.format = ma_format_f32;

    auto result = 0;
    if (Data.Pointer != nullptr) {
        result = ma_decoder_init_memory(Data.Pointer, Data.Size, &config, &Data.Decoder);
    } else {
        result = ma_decoder_init_memory(Data.AudioBuffer.data(), Data.AudioBuffer.size(), &config, &Data.Decoder);
    }

    if (result != MA_SUCCESS) {
        throw Exceptions::EstException("Failed to initialize audio decoder");
    }

    // Data.SoundTouch->setSampleRate(Data.Decoder.outputSampleRate);
    // Data.SoundTouch->setChannels(Data.Decoder.outputChannels);

    Data.Stretch->presetDefault(Data.Decoder.outputChannels, static_cast<float>(Data.Decoder.outputSampleRate));

    auto engineConfig = ma_engine_config_init();
    engineConfig.dataCallback = data_callback;
    engineConfig.pProcessUserData = &Data;
    engineConfig.pContext = engine;

    result = ma_engine_init(&engineConfig, &Data.Engine);
    if (result != MA_SUCCESS) {
        throw Exceptions::EstException("Failed to initialize audio engine");
    }

    result = ma_sound_init_from_data_source(&Data.Engine, &Data.Decoder, 0, nullptr, &Data.Sound);
    if (result != MA_SUCCESS) {
        throw Exceptions::EstException("Failed to initialize sound");
    }

    ma_engine_stop(&Data.Engine);
}

void Stream::Play()
{
    if (Data.Playing) {
        return;
    }

    if (ma_sound_is_playing(&Data.Sound)) {
        if (!ma_sound_at_end(&Data.Sound)) {
            Data.Playing = true;
            ma_sound_start(&Data.Sound);
            return;
        }
    }

    // Data.SoundTouch->clear();
    Data.Stretch->reset();

    Data.Playing = true;
    ma_engine_start(&Data.Engine);
    auto result = ma_sound_start(&Data.Sound);
    if (result != MA_SUCCESS) {
        throw Exceptions::EstException("Failed to start audio device");
    }
}

void Stream::Stop()
{
    ma_sound_stop(&Data.Sound);
    ma_sound_seek_to_pcm_frame(&Data.Sound, 0);

    Data.Playing = false;
}

void Stream::Pause()
{
    ma_sound_stop(&Data.Sound);
    Data.Playing = false;
}

void Stream::Seek(float miliseconds)
{
    uint32_t position = static_cast<uint32_t>(miliseconds * Data.Decoder.outputSampleRate);

    // Data.SoundTouch->clear();
    ma_sound_seek_to_pcm_frame(&Data.Sound, position);
}

void Stream::SetVolume(float volume)
{
    ma_sound_set_volume(&Data.Sound, volume);
}

float Stream::GetVolume() const
{
    return ma_sound_get_volume(&Data.Sound);
}

float Stream::GetLength() const
{
    throw Exceptions::EstException("Not implemented");
}

float Stream::GetCurrent() const
{
    throw Exceptions::EstException("Not implemented");
}

void Stream::SetPitch(bool enable)
{
    Data.Pitch = enable;
}

void Stream::SetRate(float pitch)
{
    Data.AudioRate = pitch;

    ma_sound_set_pitch(&Data.Sound, pitch);
    Data.Stretch->setTransposeFactor(1 / pitch);
    // Data.SoundTouch->setPitch(1 / pitch);
}
