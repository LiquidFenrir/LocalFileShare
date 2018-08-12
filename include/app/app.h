#pragma once

#include "common.h"
#include "states/state.h"

class App
{
    public:
        App(int argc, char* argv[]);
        ~App();

        void update();
    private:
        State* state;

        u32 old_time_limit;
};
