#include "states/receiving.h"
#include "states/filebrowser.h"

#include "app/room.h"

enum ReceivingStateMode
{
    RECEIVING_FILENAME,
    RECEIVING_DATA,

    DONE_RECEIVING
};

static ReceivingStateMode mode;

static std::string filePath;
static FILE* fh;

static NetworkPacket receivePacket;
static NetworkPacket metadataPacket, okPacket, resendPacket;
static NetworkPacket dataPacket;
static u32 totalPackets;
static u32& currentPacket = dataPacket.packetID;

static Handle receiveEvent, stopEvent;

static void receivePacketsThreadFunc(void* voidArg)
{
    (void)voidArg;

    while(true)
    {
        bool canceled = currentRoom->wait(stopEvent);
        if(canceled)
            break;

        size_t actualSize = 0;
        currentRoom->receivePacket(&receivePacket, &actualSize);

        if(actualSize != sizeof(receivePacket))
            svcBreak(USERBREAK_ASSERT);

        DEBUG("received packet\n");
        svcSignalEvent(receiveEvent);
    }
}

static void receivingThreadFunc(void* voidArg)
{
    (void)voidArg;

    Handle handles[2] = {
        receiveEvent,
        stopEvent,
    };

    while(true)
    {
        if(mode == RECEIVING_FILENAME)
        {
            DEBUG("sending packet asking for metadata\n");
            currentRoom->sendPacket(metadataPacket);

            s32 index = -1;
            svcWaitSynchronizationN(&index, handles, 2, false, U64_MAX);
            if(index == 1)
                break;

            if(receivePacket.checksum == getBufChecksum(receivePacket.data, receivePacket.size))
            {
                totalPackets = receivePacket.packetID;
                filePath = std::string((const char*)receivePacket.data, receivePacket.size);
                fh = fopen(filePath.c_str(), "wb");

                currentRoom->sendPacket(okPacket);

                mode = RECEIVING_DATA;
            }
        }
        else if(mode == RECEIVING_DATA)
        {
            currentRoom->sendPacket(dataPacket);
            s32 index = -1;
            svcWaitSynchronizationN(&index, handles, 2, false, U64_MAX);
            if(index == 1)
                break;

            if(receivePacket.checksum == getBufChecksum(receivePacket.data, receivePacket.size))
            {
                fwrite(receivePacket.data, 1, receivePacket.size, fh);
                if(currentPacket == totalPackets)
                {
                    mode = DONE_RECEIVING;
                    break;
                }
                else
                {
                    currentPacket++;
                }

                currentRoom->sendPacket(okPacket);
            }
            else
            {

            }
        }
    }
}

ReceivingState::ReceivingState()
{
    DEBUG("ReceivingState::ReceivingState\n");

    mode = RECEIVING_FILENAME;
    fh = NULL;

    memset(&dataPacket, 0, sizeof(dataPacket));
    memset(&resendPacket, 0, sizeof(dataPacket));
    memset(&metadataPacket, 0, sizeof(dataPacket));
    memset(&okPacket, 0, sizeof(okPacket));
    dataPacket.type = PACKET_DATA;
    dataPacket.packetID = 0;
    resendPacket.type = PACKET_RESEND;
    metadataPacket.type = PACKET_METADATA;
    okPacket.type = PACKET_OK;

    totalPackets = 0;

    svcCreateEvent(&receiveEvent, RESET_ONESHOT);
    svcCreateEvent(&stopEvent, RESET_STICKY);

    s32 prio = 0;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);

    receivePacketsThread = threadCreate(receivePacketsThreadFunc, NULL, 32*1024, prio-2, 1, false);
    receivingThread = threadCreate(receivingThreadFunc, NULL, 32*1024, prio-1, 0, false);
}

ReceivingState::~ReceivingState()
{
    DEBUG("ReceivingState::~ReceivingState\n");

    svcSignalEvent(stopEvent);

    threadJoin(receivePacketsThread, U64_MAX);
    threadFree(receivePacketsThread);

    threadJoin(receivingThread, U64_MAX);
    threadFree(receivingThread);

    if(fh)
        fclose(fh);

    svcCloseHandle(receiveEvent);
    svcCloseHandle(stopEvent);
}

void ReceivingState::drawTop()
{
    C2D_Text* appTitleText = makeText(APP_TITLE);
    C2D_DrawText(appTitleText, C2D_WithColor, 4, 4, 0.5f, textScale, textScale, textColor);

    C2D_Text* appVersionText = makeText(VERSION);
    float width = 0;
    C2D_TextGetDimensions(appVersionText, textScale, textScale, &width, NULL);
    C2D_DrawText(appVersionText, C2D_WithColor, 400 - 4 - width, 4, 0.5f, textScale, textScale, textColor);

    C2D_Text* centeredText = makeText("Done receiving, going back to file browser.");
    if(mode == RECEIVING_FILENAME)
    {
        centeredText = makeText("Waiting for the file name, press \uE001 to cancel...");
    }
    else if(mode == RECEIVING_DATA)
    {
        char* nameString = NULL;
        asprintf(&nameString, "Downloading %s...", filePath.c_str());
        centeredText = makeText(nameString);
        free(nameString);
    }
    float height = 0;
    C2D_TextGetDimensions(centeredText, textScale, textScale, NULL, &height);
    float y = (240-height)/2;
    drawCentered(GFX_TOP, centeredText, y, 0.5f, textScale, textScale, textColor);
}

void ReceivingState::drawBottom()
{
    if(mode == RECEIVING_DATA)
    {
        u32 percent = 100*((double)currentPacket/(double)totalPackets);
        C2D_DrawRectSolid(60-4, 110-4, 0.4f, 200+8, 20+8, blackColor);
        C2D_DrawRectSolid(60-2, 110-2, 0.5f, 200+4, 20+4, backgroundColor);
        C2D_DrawRectSolid(60, 110, 0.6f, percent*2, 20, whiteColor);
        char* text = NULL;
        asprintf(&text, "%ld out of %ld blocks", currentPacket+1, totalPackets);
        drawCentered(GFX_BOTTOM, makeText(text), 110, 0.7f, textScale, textScale, blackColor);
        free(text);
    }
}

void ReceivingState::update()
{
    if(kDown & KEY_B || mode == DONE_RECEIVING)
    {
        nextState = new FileBrowserState;
    }
}