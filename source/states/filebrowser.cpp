#include "states/filebrowser.h"
#include "states/sending.h"
#include "states/receiving.h"

extern "C" {
#include <unistd.h>
}

static bool isOnRoot()
{
    char cwd[256] = {0};
    getcwd(cwd, 256);
    return cwd[1] == '\0'; // if there is more than 1 character ('/'), we aren't on root
}

FileBrowserState::FileBrowserState()
{
    DEBUG("FileBrowserState::FileBrowserState\n");

    DEBUG("Starting at root\n");
    chdir("/");
    this->openFolder();
}

FileBrowserState::~FileBrowserState()
{
    DEBUG("FileBrowserState::~FileBrowserState\n");
}

void FileBrowserState::drawTop()
{
    C2D_Text* appTitleText = makeText(APP_TITLE);
    C2D_DrawText(appTitleText, C2D_WithColor, 4, 4, 0.5f, textScale, textScale, textColor);

    C2D_Text* appVersionText = makeText(VERSION);
    float width = 0;
    C2D_TextGetDimensions(appVersionText, textScale, textScale, &width, NULL);
    C2D_DrawText(appVersionText, C2D_WithColor, 400 - 4 - width, 4 , 0.5f, textScale, textScale, textColor);
}

void FileBrowserState::drawBottom()
{
    char* positionTextChar = NULL;
    asprintf(&positionTextChar, "%d/%zd", this->selected+1, this->files.size());
    C2D_Text* positionText = makeText(positionTextChar);
    float width = 0, height = 0;
    C2D_TextGetDimensions(positionText, textScale, textScale, &width, &height);
    C2D_DrawText(positionText, C2D_WithColor, 320 - 4 - width, 240 - 4 - height, 0.5f, textScale, textScale, textColor);
    free(positionTextChar);

    C2D_ImageTint arrowTint;
    C2D_PlainImageTint(&arrowTint, whiteColor, 1.0f);
    C2D_Image arrowImage = C2D_SpriteSheetGetImage(spritesheet, sprites_arrow_idx);
    if(scroll > 0)
        C2D_DrawImageAt(arrowImage, (320 - 24)/2, 0, 0.5f, &arrowTint);
    if(this->files.size() > static_cast<size_t>(maxLinesPerScreen) && static_cast<size_t>(this->scroll) < this->files.size() - maxLinesPerScreen)
        C2D_DrawImageAt(arrowImage, (320 - 24)/2, 240-24, 0.5f, &arrowTint, 1.0f, -1.0f);

    static float textX = 32+8;
    for(int i = this->scroll; i < this->scroll + maxLinesPerScreen; i++)
    {
        if(static_cast<size_t>(i) >= this->files.size())
            break;

        auto& entry = this->files[i];

        float y = 24 + (i-this->scroll)*32;

        u32 color = textColor;
        C2D_ImageTint tint;
        C2D_PlainImageTint(&tint, whiteColor, 1.0f);

        if(i == this->selected)
        {
            C2D_DrawRectSolid(0, y, 0.5f, 320, 32, whiteColor);
            color = selectedTextColor;
            C2D_PlainImageTint(&tint, blackColor, 1.0f);
        }

        C2D_Text* text = NULL;
        C2D_Image image;
        if(entry.path == ".")
        {
            text = makeText("Press \uE000 to download here!");
            image = C2D_SpriteSheetGetImage(spritesheet, sprites_download_idx);
        }
        else if(entry.path == "..")
        {
            text = makeText("Press \uE001 to go back!");
            image = C2D_SpriteSheetGetImage(spritesheet, sprites_go_back_idx);
        }
        else
        {
            text = makeText(entry.path.filename().string());
            if(entry.isFolder)
                image = C2D_SpriteSheetGetImage(spritesheet, sprites_folder_idx);
            else
                image = C2D_SpriteSheetGetImage(spritesheet, sprites_file_idx);
        }
        C2D_DrawText(text, C2D_WithColor, textX, y+8, 0.6f, textScale, textScale, color);
        C2D_DrawImageAt(image, 0, y, 0.6f, &tint);
    }
}

void FileBrowserState::update()
{
    if(kDown & KEY_TOUCH)
    {
        if(touch.py < 24)
        {
            if(touch.px >= (320-24)/2 && touch.px <= (320+24)/2)
            {
                if(this->scroll > 0)
                    this->scroll--;
                if(this->selected >= this->scroll + maxLinesPerScreen)
                    this->selected--;
            }
        }
        else if(touch.py > 240-24)
        {
            if(touch.px >= (320-24)/2 && touch.px <= (320+24)/2)
            {
                if(this->files.size() > static_cast<size_t>(maxLinesPerScreen) && static_cast<size_t>(this->scroll) < this->files.size() - maxLinesPerScreen)
                    this->scroll++;
                if(this->selected < this->scroll)
                    this->selected++;
            }
        }
        else
        {
            int y = (touch.py - 24) / 32;
            if(static_cast<size_t>(y) < this->files.size())
            {
                if(this->selected == y)
                    goto handlePress;
                else
                    this->selected = y;
            }
        }
    }
    else if(kDown & KEY_A)
    {
        handlePress:
        auto& entry = this->files[this->selected];
        if(entry.isFolder)
        {
            const char* path = entry.path.c_str();
            DEBUG("entering folder: %s\n", path);
            chdir(path);
            this->openFolder();
        }
        else
        {
            if(this->selected == (isOnRoot() ? 0 : 1))
            {
                DEBUG("Downloading to this folder\n");
                this->nextState = new ReceivingState;
            }
            else
            {
                DEBUG("Sending file\n");
                this->nextState = new SendingState(entry.path.filename());
            }
        }
    }
    else if(kDown & KEY_B)
    {
        if(!isOnRoot())
        {
            DEBUG("going back 1 folder\n");
            chdir("..");
            this->openFolder();
        }
    }
    else if(kDown & KEY_LEFT)
    {
        jumpToTop:
        this->selected = 0;
        this->scroll = 0;
    }
    else if(kDown & KEY_RIGHT)
    {
        jumpToBottom:
        this->selected = this->files.size()-1;
        if(this->files.size() > static_cast<size_t>(maxLinesPerScreen))
        {
            this->scroll = this->files.size() - maxLinesPerScreen;
        }
        else
        {
            this->scroll = 0;
        }
    }
    else if(kDown & KEY_DOWN)
    {
        this->selected++;
        if(static_cast<size_t>(this->selected) >= this->files.size())
        {
            goto jumpToTop;
        }
        else if(this->selected - this->scroll >= maxLinesPerScreen)
        {
            this->scroll++;
        }
    }
    else if(kDown & KEY_UP)
    {
        this->selected--;
        if(this->selected < 0)
        {
            goto jumpToBottom;
        }
        else if(this->scroll > 0 && this->selected - this->scroll == -1)
        {
            this->scroll--;
        }
    }
}

static bool compareFunc(const Entry& a, const Entry& b)
{
    if(a.path == "." || a.path == "..")
        return true;
    else if(b.path == "." || b.path == "..")
        return false;
    else if(a.isFolder && !b.isFolder)
        return true;
    else if(!a.isFolder && b.isFolder)
        return false;
    else
        return !a.path.compare(b.path);
}

void FileBrowserState::openFolder()
{
    this->files.clear();
    this->files.reserve(2);
    for(auto& p : fs::directory_iterator("."))
    {
        Entry entry;
        entry.path = p.path();
        entry.isFolder = p.is_directory();
        this->files.push_back(entry);
    }

    Entry currentFolder;
    currentFolder.path = ".";
    currentFolder.isFolder = false;
    this->files.push_back(currentFolder);

    if(!isOnRoot())
    {
        Entry backFolder;
        backFolder.path = "..";
        backFolder.isFolder = true;
        this->files.push_back(backFolder);
    }

    std::sort(this->files.begin(), this->files.end(), compareFunc);

    this->scroll = 0;
    this->selected = 0;
}
