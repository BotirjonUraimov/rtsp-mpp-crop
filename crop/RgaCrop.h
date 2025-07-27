#pragma once

#include <rga/rga.h>
#include <rga/RgaApi.h>
#include "rk_mpi.h"           // for MPP_FMT_NV12
#include "rk_mpi_cmd.h"       // optional but good for config
#include "mpp_frame.h"        // for mpp_frame_set_format()

class RgaCrop {
public:
    RgaCrop();
    ~RgaCrop();

    void set_roi(int x, int y, int w, int h);
    bool crop(MppFrame src, MppFrame& dst);

private:
    int roi_x, roi_y, roi_w, roi_h;
};
