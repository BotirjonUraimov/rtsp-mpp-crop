#pragma once

#include <rockchip/rk_mpi.h>

class Im2dCrop {
public:
    Im2dCrop();
    ~Im2dCrop();

    void set_roi(int x, int y, int w, int h);
    bool crop(MppFrame src, MppFrame& dst);

private:
    int roi_x, roi_y, roi_w, roi_h;
};
