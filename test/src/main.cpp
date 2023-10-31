#include <Audio/AudioEngine.h>
#include <Audio/AudioStream.h>
#include <Game.h>
#include <Screens/ScreenManager.h>
#include <UI/Text.h>

class MyScreen : public Screens::Base
{
public:
    void Update(double delta) override
    {
        m_Text->Rotation += delta * 100;
        if (m_Text->Rotation > 360) {
            m_Text->Rotation -= 360;
        }
    }

    void Draw(double delta) override
    {
        m_Text->DrawStringFormatted("Hello world, %d", 255);
    }

    bool Attach() override
    {
        m_Text = std::make_unique<UI::Text>();
        m_Text->Position = UDim2::fromScale(0.5, 0.5);
        m_Text->Alignment = UI::Alignment::Center;

        auto manager = Audio::Engine::Get();
        m_Stream = manager->LoadStream("C:\\Users\\ACER\\Documents\\Games\\DPJAM\\Music\\dump_output\\Back_to_the_gatemp3_ref(0).wav");

        m_Stream->SetRate(1.2f);
        m_Stream->Play();
        return true;
    }

    bool Detach() override
    {
        m_Text.reset();
        m_Stream->Stop();

        auto manager = Audio::Engine::Get();
        manager->Destroy(m_Stream);
        return true;
    }

private:
    std::unique_ptr<UI::Text> m_Text;
    Audio::Stream            *m_Stream;
};

class MyGame : public Game
{
protected:
    void OnLoad() override
    {
        auto manager = Screens::Manager::Get();
        manager->AddScreen(0, new MyScreen());
        manager->SetScreen(0);
    }
};

int main()
{
    // enable memory leak test
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    MyGame game;

    RunInfo info = {};
    info.title = "Game";
    info.resolution = { 1280, 720 };
    info.fullscreen = false;
    info.threadMode = ThreadMode::Multi;
    info.graphics = Graphics::API::Vulkan;

    game.Run(info);
}