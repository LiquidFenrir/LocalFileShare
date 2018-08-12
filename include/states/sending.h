#pragma once

#include "common.h"

#include "states/state.h"

class SendingState : public State
{
    public:
        SendingState(const std::string& path);
        ~SendingState();

        void update();
        void drawTop();
        void drawBottom();

    private:
        Thread receivePacketsThread, sendingThread, readFileThread;
};
