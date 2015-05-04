
#include "demofile.h"
#include "demofilebitbuf.h"
#include "netmessages.h"
#include <assert.h>
#include <vector>

void ParsePacket(const std::vector<unsigned char>& packet)
{
    assert(packet.size() <= NET_MAX_PAYLOAD);
    CBitRead bitBuf(packet.data(), packet.size());
    while (bitBuf.GetNumBitsLeft() >= NETMSG_TYPE_BITS)
    {
        uint32 typeId = bitBuf.ReadUBitLong(NETMSG_TYPE_BITS);
        printf("%i\n", typeId);
        ProcessNetMsg(typeId, bitBuf);
    }
}

void ParseSignonData(const std::vector<unsigned char>& signonData)
{
    CBitRead bitbuf(signonData.data(), signonData.size());
    const char cmd = bitbuf.ReadChar();
    assert(cmd == dem_signon);
    const int32 tick = bitbuf.ReadLong();
    bitbuf.SeekRelative(sizeof(democmdinfo_t) * 8);
    const int32 seq1 = bitbuf.ReadLong();
    const int32 seq2 = bitbuf.ReadLong();
    assert(seq1 == seq2);

    const int32 numBytes = bitbuf.ReadLong();
    std::vector<unsigned char> packet;
    packet.resize(numBytes);
    bitbuf.ReadBytes(&packet[0], numBytes);
    ParsePacket(packet);
}

int main(const int argc, const char* argv[])
{
    if (argc < 2)
    {
        return -1;
    }

    CDemoFile demoFile;
    if (!demoFile.Open(argv[1]))
    {
        return -1;
    }

    auto demoHeader = demoFile.GetDemoHeader();

    ParseSignonData(demoFile.GetSignOnData());

    unsigned char cmd;
    int32 tick;
    int32 sequenceInfo1;
    int32 sequenceInfo2;
    democmdinfo_t cmdInfo;
    std::vector<unsigned char> packet;
    demoFile.ReadCmdHeader(cmd, tick);

    assert(cmd == dem_synctick && tick == 0);
    
    const int numFrames = demoHeader->playback_frames;
    const int numTicks = demoHeader->playback_ticks;
    for (int i = 0; i <= numFrames; ++i)
    {
        demoFile.ReadCmdHeader(cmd, tick);
        //printf("tick: %i\n", tick);
        switch (cmd)
        {
            case dem_packet:
                demoFile.ReadCmdInfo(cmdInfo);
                demoFile.ReadSequenceInfo(sequenceInfo1, sequenceInfo2);
                assert(sequenceInfo1 == sequenceInfo2);
                demoFile.ReadRawData(packet);
                ParsePacket(packet);
                break;
            case dem_stop:
                assert(i == numFrames && tick == numTicks);
                break;
            case dem_synctick:
            case dem_consolecmd:
            case dem_usercmd:
            case dem_datatables:
            default:
                assert(false);
                break;
        }
    }

    demoFile.Close();

    return 0;
}