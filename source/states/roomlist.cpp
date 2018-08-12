#include "states/roomlist.h"
#include "states/filebrowser.h"

RoomSelectionState::RoomSelectionState()
{
    DEBUG("RoomSelectionState::RoomSelectionState\n");
    this->scanRooms();
}

RoomSelectionState::~RoomSelectionState()
{
    DEBUG("RoomSelectionState::~RoomSelectionState\n");
}

void RoomSelectionState::drawTop()
{
    C2D_Text* appTitleText = makeText(APP_TITLE);
    C2D_DrawText(appTitleText, C2D_WithColor, 4, 4, 0.5f, textScale, textScale, textColor);

    C2D_Text* appVersionText = makeText(VERSION);
    float width = 0;
    C2D_TextGetDimensions(appVersionText, textScale, textScale, &width, NULL);
    C2D_DrawText(appVersionText, C2D_WithColor, 400 - 4 - width, 4 , 0.5f, textScale, textScale, textColor);
}

void RoomSelectionState::drawBottom()
{
    char* positionTextChar = NULL;
    asprintf(&positionTextChar, "%d/%zd", this->selected+1, this->rooms.size()+2);
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
    if(this->rooms.size()+2 > static_cast<size_t>(maxLinesPerScreen) && static_cast<size_t>(this->scroll) < this->rooms.size()+2 - maxLinesPerScreen)
        C2D_DrawImageAt(arrowImage, (320 - 24)/2, 240-24, 0.5f, &arrowTint, 1.0f, -1.0f);

    static float textX = 32+8;
    for(int i = this->scroll; i < this->scroll + maxLinesPerScreen; i++)
    {
        if(static_cast<size_t>(i) >= this->rooms.size()+2)
            break;

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
        if(i == 0)
        {
            text = makeText("Create a new room");
            image = C2D_SpriteSheetGetImage(spritesheet, sprites_new_room_idx);
        }
        else if(i == 1)
        {
            text = makeText("Refresh the list");
            image = C2D_SpriteSheetGetImage(spritesheet, sprites_refresh_idx);
        }
        else
        {
            auto& room = this->rooms[i-2];
            text = makeText(room->getName());
            image = C2D_SpriteSheetGetImage(spritesheet, sprites_room_idx);
        }
        C2D_DrawText(text, C2D_WithColor, textX, y+8, 0.6f, textScale, textScale, color);
        C2D_DrawImageAt(image, 0, y, 0.6f, &tint);
    }
}

void RoomSelectionState::update()
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
                if(this->rooms.size()+2 > static_cast<size_t>(maxLinesPerScreen) && static_cast<size_t>(this->scroll) < this->rooms.size()+2 - maxLinesPerScreen)
                    this->scroll++;
                if(this->selected < this->scroll)
                    this->selected++;
            }
        }
        else
        {
            int y = (touch.py - 24) / 32;
            if(static_cast<size_t>(y) < this->rooms.size()+2)
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
        // DEBUG("an A press is an A press. you can't say it's only a half.\n");
        if(this->selected == 0)
        {
            DEBUG("Creating a new room\n");
            currentRoom = std::make_shared<Room>();
            this->nextState = new FileBrowserState;
        }
        else if(this->selected == 1)
        {
            DEBUG("Refreshing list\n");
            this->scanRooms();
        }
        else
        {
            DEBUG("Joining room %d\n", this->selected-1);
            currentRoom = this->rooms[this->selected-2];
            currentRoom->join();
            this->nextState = new FileBrowserState;
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
        this->selected = this->rooms.size()+2-1;
        if(this->rooms.size()+2 > static_cast<size_t>(maxLinesPerScreen))
        {
            this->scroll = this->rooms.size()+2 - maxLinesPerScreen;
        }
        else
        {
            this->scroll = 0;
        }
    }
    else if(kDown & KEY_DOWN)
    {
        this->selected++;
        if(static_cast<size_t>(this->selected) >= this->rooms.size()+2)
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

void RoomSelectionState::scanRooms()
{
    this->rooms.clear();
    this->rooms = getRoomList();
    auto compareFunc = [](const auto& a, const auto& b) -> bool
    {
        return !a->getName().compare(b->getName());
    };
    std::sort(this->rooms.begin(), this->rooms.end(), compareFunc);
    if(this->scroll != 0)
        this->scroll = 0;
    if(this->selected >= 2)
        this->selected = 0;
}
