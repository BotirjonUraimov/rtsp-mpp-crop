#pragma once

#include <vector>
#include <cstdint>
#include <rockchip/rk_mpi.h>
#include <mpp.h>

class MppDecoder {
public:
    MppDecoder();
    ~MppDecoder();

    bool init(MppCodingType type);
    bool decode(uint8_t* data, size_t size, MppFrame& outFrame);

private:
    MppCtx ctx;
    MppApi* mpi;
    MppPacket packet;
    MppCodingType codec_type;
    bool initialized = false;
};
