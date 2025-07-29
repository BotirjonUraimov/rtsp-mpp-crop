#pragma once

#include <vector>
#include <cstdint>
#include <rockchip/rk_mpi.h>

class MppEncoder {
public:
    MppEncoder(int width, int height, MppCodingType type = MPP_VIDEO_CodingAVC);
    ~MppEncoder();

    bool init();
    bool encode(MppFrame frame, std::vector<uint8_t>& out);

private:
    int width;
    int height;
    MppCodingType codec_type;

    MppCtx ctx;
    MppApi* mpi;
    MppEncCfg cfg;
    bool initialized = false;
};
