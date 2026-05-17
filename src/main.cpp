#include "ProtonApplication.hpp"

int main()
{
    Proton::ApplicationConfig config;
    config.Name = "Proton G Sandbox";
    config.Width = 1280;
    config.Height = 720;

    Proton::Application app(config);
    return app.Run();
}