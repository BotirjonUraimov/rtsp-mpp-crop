#include "RgaCrop.h"
// #include <rockchip/rk_type.h>
// #include <rockchip/rk_mpi_cmd.h>
// #include <RockchipRga.h>
// #include <rga.h>
 #include <iostream>
#include <rk_type.h>
#include <rk_mpi.h>
#include <rk_mpi_cmd.h>
#include <rga/RockchipRga.h>
#include <rga/rga.h>
#include <rga/RgaApi.h>
#include "mpp_frame.h"
#include <RockchipRga.h>  // for RK_FORMAT_NV12



RgaCrop::RgaCrop()
    : roi_x(0), roi_y(0), roi_w(0), roi_h(0) {}

RgaCrop::~RgaCrop() {}

void RgaCrop::set_roi(int x, int y, int w, int h) {
    roi_x = x;
    roi_y = y;
    roi_w = w;
    roi_h = h;
}

bool RgaCrop::crop(MppFrame src, MppFrame& dst) {
    if (!src || roi_w <= 0 || roi_h <= 0) return false;

    int src_w = mpp_frame_get_width(src);
    int src_h = mpp_frame_get_height(src);
    MppBuffer src_buf = mpp_frame_get_buffer(src);
    if (!src_buf) return false;

    size_t dst_buf_size = roi_w * roi_h * 3 / 2;
    MppBuffer dst_buf = nullptr;
    MppBufferGroup grp = nullptr;

    if (mpp_buffer_group_get_internal(&grp, MPP_BUFFER_TYPE_DRM) != MPP_OK) {
        std::cerr << "[RGA1] Failed to get buffer group\n";
        return false;
    }
    if (mpp_buffer_get(grp, &dst_buf, dst_buf_size) != MPP_OK) {
        std::cerr << "[RGA1] Failed to alloc dst buffer\n";
        return false;
    }

    RockchipRga rgautil;
    rga_info_t srcinfo, dstinfo;
    memset(&srcinfo, 0, sizeof(srcinfo));
    memset(&dstinfo, 0, sizeof(dstinfo));

    srcinfo.fd = mpp_buffer_get_fd(src_buf);
    srcinfo.mmuFlag = 1;
    srcinfo.rect.xoffset = roi_x;
    srcinfo.rect.yoffset = roi_y;
    srcinfo.rect.width   = roi_w;
    srcinfo.rect.height  = roi_h;
    srcinfo.rect.wstride = src_w;
    srcinfo.rect.hstride = src_h;
    srcinfo.format = RK_FORMAT_NV12;

    dstinfo.fd = mpp_buffer_get_fd(dst_buf);
    dstinfo.mmuFlag = 1;
    dstinfo.rect.xoffset = 0;
    dstinfo.rect.yoffset = 0;
    dstinfo.rect.width   = roi_w;
    dstinfo.rect.height  = roi_h;
    dstinfo.rect.wstride = roi_w;
    dstinfo.rect.hstride = roi_h;
    dstinfo.format = RK_FORMAT_NV12;

    int ret = rgautil.RkRgaBlit(&srcinfo, &dstinfo, nullptr);
    if (ret) {
        std::cerr << "[RGA1] RkRgaBlit failed, code: " << ret << "\n";
        return false;
    }

    // Build output MppFrame
    if (mpp_frame_init(&dst) != MPP_OK) return false;
    mpp_frame_set_width(dst, roi_w);
    mpp_frame_set_height(dst, roi_h);
    mpp_frame_set_format(dst, MPP_FMT_NV12);
    mpp_frame_set_buffer(dst, dst_buf);

    return true;
}
