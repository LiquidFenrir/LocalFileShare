#pragma once

#include "common.h"

#include "states/state.h"

struct Entry
{
    fs::path path;
    bool isFolder;
};

class FileBrowserState : public State
{
    public:
        FileBrowserState();
        ~FileBrowserState();

        void update();
        void drawTop();
        void drawBottom();

    private:
        void openFolder();
        
        std::vector<Entry> files;
        int scroll, selected;
};
