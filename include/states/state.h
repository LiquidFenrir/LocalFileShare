#pragma once

#include "common.h"

class State
{
    public:
        virtual ~State() { };

        virtual void update() = 0;
        virtual void drawTop() = 0;
        virtual void drawBottom() = 0;

        State* nextState = nullptr;
};
