#include <Audio/AudioEngine.h>
#include <Audio/AudioStream.h>
#include <Game.h>
#include <Graphics/Renderer.h>
#include <Screens/ScreenManager.h>
#include <UI/Image.h>
#include <UI/Rectangle.h>
#include <UI/Text.h>

#include <Imgui/imgui.h>

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
        static float radius = 0.0;

        srand((uint32_t)time(NULL));

        m_Quad->Size = UDim2::fromScale(0.25, 0.25);

        m_Image->CornerRadius = Vector4(0.15, 0.15, 0.15, 0.15);
        m_Image->Draw();

        m_Quad->Color3 = Color3::fromRGB(255, 255, 255);
        m_Quad->Position = UDim2::fromScale(0.5, 0.5);
        m_Quad->Size = UDim2::fromOffset(50, 50);
        m_Quad->CornerRadius = Vector4(radius, radius, radius, radius);
        m_Quad->Draw();

        m_Text->DrawString("Hello world");

        auto renderer = Graphics::Renderer::Get();
        renderer->ImGui_NewFrame();

        ImGui::ShowDemoWindow();

        ImGui::Begin("Whatever");

        ImGui::InputFloat("Radius", &radius);

        ImGui::End();

        renderer->ImGui_EndFrame();
    }

    bool Attach() override
    {
        m_Text = std::make_unique<UI::Text>();
        m_Text->Position = UDim2::fromScale(0.5, 0.5);
        m_Text->Alignment = UI::Alignment::Center;

        m_Quad = std::make_unique<UI::Rectangle>();

        m_Image = std::make_unique<UI::Image>("C:\\Users\\ACER\\Documents\\Games\\DPJAM\\Image\\Playing_BG3\\Frame1.bmp");

        auto manager = Audio::Engine::Get();
        m_Stream = manager->LoadStream("C:\\Users\\ACER\\Documents\\Games\\DPJAM\\Music\\dump_output\\Back_to_the_gatemp3_ref(0).wav");

        m_Stream->SetRate(1.2f);
        m_Stream->SetPitch(false);
        m_Stream->Play();
        return true;
    }

    bool Detach() override
    {
        m_Text.reset();
        m_Quad.reset();
        m_Stream->Stop();
        m_Image.reset();

        auto manager = Audio::Engine::Get();
        manager->Destroy(m_Stream);
        return true;
    }

private:
    std::unique_ptr<UI::Text>      m_Text;
    std::unique_ptr<UI::Rectangle> m_Quad;
    std::unique_ptr<UI::Image>     m_Image;
    Audio::Stream                 *m_Stream;
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