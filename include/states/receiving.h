#pragma once

#include "common.h"

#include "states/state.h"

class ReceivingState : public State
{
    public:
        ReceivingState();
        ~ReceivingState();

        void update();
        void drawTop();
        void drawBottom();

    private:
        Thread receivePacketsThread, receivingThread;
};
