#include <Game.h>
#include <Screens/Manager.h>
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
        return true;
    }

    bool Detach() override
    {
        m_Text.reset();
        return true;
    }

private:
    std::unique_ptr<UI::Text> m_Text;
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
    MyGame game;

    RunInfo info = {};
    info.title = "Game";
    info.resolution = { 1280, 720 };
    info.fullscreen = false;
    info.threadMode = ThreadMode::Multi;
    info.graphics = Graphics::API::OpenGL;

    game.Run(info);
}