#include "states/sending.h"
#include "states/filebrowser.h"

#include "app/room.h"

enum SendingStateMode
{
    SENDING_FILENAME,
    SENDING_DATA,

    DONE_SENDING
};

static SendingStateMode mode;

static std::string filePath;
static FILE* fh;

static NetworkPacket receivePacket;

static NetworkPacket dataPackets[2];
static int currentPacketSlot;

static u32 totalPackets, currentPacket;

static Handle mutex1, mutex2, receiveEvent, stopEvent;

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

static void sendingThreadFunc(void* voidArg)
{
    (void)voidArg;

    Handle handles[2] = {
        receiveEvent,
        stopEvent,
    };

    while(true)
    {
        s32 index = -1;
        svcWaitSynchronizationN(&index, handles, 2, false, U64_MAX);
        if(index == 1)
            break;

        if(mode == SENDING_FILENAME)
        {
            if(receivePacket.type == PACKET_METADATA)
            {
                DEBUG("received packet asking for metadata\n");
                fseek(fh, 0, SEEK_END);
                long int size = ftell(fh);
                fseek(fh, 0, SEEK_SET);

                totalPackets = (size / PACKET_DATA_SIZE) + (size % PACKET_DATA_SIZE ? 1 : 0);
                DEBUG("size in bytes, total packets:\n%ld, %ld\n", size, totalPackets);

                NetworkPacket metadataPacket;
                metadataPacket.type = PACKET_METADATA;
                metadataPacket.packetID = totalPackets;
                metadataPacket.size = filePath.size();
                memcpy(metadataPacket.data, filePath.data(), metadataPacket.size);
                metadataPacket.checksum = getBufChecksum(metadataPacket.data, metadataPacket.size);

                DEBUG("sending packet with metadata\n");
                currentRoom->sendPacket(metadataPacket);
            }
            else if(receivePacket.type == PACKET_OK)
            {
                DEBUG("received packet saying metadata was received ok, switching to data mode\n");
                mode = SENDING_DATA;
            }
        }
        else if(mode == SENDING_DATA)
        {
            if(receivePacket.type == PACKET_DATA)
            {
                if(currentPacketSlot == 0)
                {
                    svcWaitSynchronization(mutex1, U64_MAX);
                    svcReleaseMutex(mutex2);
                }
                else if(currentPacketSlot == 1)
                {
                    svcWaitSynchronization(mutex2, U64_MAX);
                    svcReleaseMutex(mutex1);
                }
                
                dataPackets[currentPacketSlot].packetID = currentPacket = receivePacket.packetID;
                currentRoom->sendPacket(dataPackets[currentPacketSlot]);
            }
            else if(receivePacket.type == PACKET_RESEND)
            {
                DEBUG("received packet asking to resend data\n");
                currentRoom->sendPacket(dataPackets[currentPacketSlot]);
            }
            else if(receivePacket.type == PACKET_OK)
            {
                if(currentPacket == totalPackets)
                {
                    DEBUG("done sending packets\n");
                    mode = DONE_SENDING;
                    break;
                }
                else
                {
                    currentPacketSlot = 1 - currentPacketSlot;  // swap 0 to 1 and 1 to 0
                }
            }
        }
    }
}

static void readFileThreadFunc(void* voidArg)
{
    (void)voidArg;

    Handle handles[3] = {
        mutex1,
        mutex2,
        stopEvent,
    };

    while(true)
    {
        s32 index = -1;
        svcWaitSynchronizationN(&index, handles, 3, false, U64_MAX);
        if(index == 2)
            break;

        dataPackets[index].size = fread(dataPackets[index].data, 1, PACKET_DATA_SIZE, fh);
        dataPackets[index].checksum = getBufChecksum(dataPackets[index].data, dataPackets[index].size);
        svcReleaseMutex(handles[index]);
    }
}

SendingState::SendingState(const std::string& path)
{
    DEBUG("SendingState::SendingState\n");
    DEBUG("Sending file: %s\n", path.c_str());
    filePath = path;

    mode = SENDING_FILENAME;
    fh = fopen(path.c_str(), "rb");

    memset(dataPackets, 0, sizeof(dataPackets));
    dataPackets[0].type = PACKET_DATA;
    dataPackets[1].type = PACKET_DATA;

    totalPackets = 0;
    currentPacket = 0;
    currentPacketSlot = 0;

    svcCreateMutex(&mutex1, false);
    svcCreateMutex(&mutex2, true);
    svcCreateEvent(&receiveEvent, RESET_ONESHOT);
    svcCreateEvent(&stopEvent, RESET_STICKY);

    s32 prio = 0;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);

    readFileThread = threadCreate(readFileThreadFunc, NULL, 32*1024, prio-1, 0, false);
    receivePacketsThread = threadCreate(receivePacketsThreadFunc, NULL, 32*1024, prio-2, 1, false);
    svcSleepThread(1e6);  // make sure the data is read and mutex is locked
    sendingThread = threadCreate(sendingThreadFunc, NULL, 32*1024, prio-1, 0, false);
}

SendingState::~SendingState()
{
    DEBUG("SendingState::~SendingState\n");

    svcSignalEvent(stopEvent);

    threadJoin(receivePacketsThread, U64_MAX);
    threadFree(receivePacketsThread);

    threadJoin(sendingThread, U64_MAX);
    threadFree(sendingThread);

    threadJoin(readFileThread, U64_MAX);
    threadFree(readFileThread);

    fclose(fh);

    svcCloseHandle(mutex1);
    svcCloseHandle(mutex2);
    svcCloseHandle(receiveEvent);
    svcCloseHandle(stopEvent);
}

void SendingState::drawTop()
{
    C2D_Text* appTitleText = makeText(APP_TITLE);
    C2D_DrawText(appTitleText, C2D_WithColor, 4, 4, 0.5f, textScale, textScale, textColor);

    C2D_Text* appVersionText = makeText(VERSION);
    float width = 0;
    C2D_TextGetDimensions(appVersionText, textScale, textScale, &width, NULL);
    C2D_DrawText(appVersionText, C2D_WithColor, 400 - 4 - width, 4 , 0.5f, textScale, textScale, textColor);

    C2D_Text* centeredText = makeText("Done sending, going back to file browser.");
    if(mode == SENDING_FILENAME)
    {
        centeredText = makeText("Waiting for receiver, press \uE001 to abort...");
    }
    else if(mode == SENDING_DATA)
    {
        centeredText = makeText("Sending data, press \uE001 to abort!");
    }

    float height = 0;
    C2D_TextGetDimensions(centeredText, textScale, textScale, NULL, &height);
    float y = (240-height)/2;
    drawCentered(GFX_TOP, centeredText, y, 0.5f, textScale, textScale, textColor);
}

void SendingState::drawBottom()
{
    if(mode == SENDING_DATA)
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

void SendingState::update()
{
    if(kDown & KEY_B || mode == DONE_SENDING)
    {
        nextState = new FileBrowserState;
    }
}
