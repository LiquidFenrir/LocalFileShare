#pragma once

#include "common.h"

#include "app/room.h"

#include "states/state.h"

class RoomSelectionState : public State
{
    public:
        RoomSelectionState();
        ~RoomSelectionState();

        void update();
        void drawTop();
        void drawBottom();

    private:
        void scanRooms();
        std::vector<std::shared_ptr<Room>> rooms;
        int scroll, selected;
};
