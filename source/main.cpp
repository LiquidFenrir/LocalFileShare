#include "app/app.h"

int __stacksize__ = (32 * 1024) + 0x4000 * 6;

bool running = true;

C2D_SpriteSheet spritesheet;

C3D_RenderTarget *top, *bottom;
C2D_TextBuf staticBuf, dynamicBuf;

u32 kDown, kHeld, kUp;
touchPosition touch;

int main(int argc, char* argv[])
{
    consoleDebugInit(debugDevice_SVC);
    auto app = std::make_unique<App>(argc, argv);

    while(aptMainLoop() && running)
        app->update();

    return 0;
}
