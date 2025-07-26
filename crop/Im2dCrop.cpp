#include "Im2dCrop.h"
#include <im2d.h>
#include <rockchip/rk_type.h>
#include <rockchip/rk_mpi_cmd.h>
#include <iostream>

Im2dCrop::Im2dCrop()
    : roi_x(0), roi_y(0), roi_w(0), roi_h(0) {}

Im2dCrop::~Im2dCrop() {}

void Im2dCrop::set_roi(int x, int y, int w, int h) {
    roi_x = x;
    roi_y = y;
    roi_w = w;
    roi_h = h;
}

bool Im2dCrop::crop(MppFrame src, MppFrame& dst) {
    if (!src || roi_w <= 0 || roi_h <= 0) return false;

    int src_w = mpp_frame_get_width(src);
    int src_h = mpp_frame_get_height(src);
    MppBuffer src_buf = mpp_frame_get_buffer(src);
    if (!src_buf) return false;

    // Allocate destination buffer
    size_t dst_buf_size = roi_w * roi_h * 3 / 2;
    MppBuffer dst_buf = nullptr;
    MppBufferGroup grp = nullptr;

    if (mpp_buffer_group_get_internal(&grp, MPP_BUFFER_TYPE_DRM) != MPP_OK) {
        std::cerr << "[RGA] Failed to get buffer group\n";
        return false;
    }
    if (mpp_buffer_get(grp, &dst_buf, dst_buf_size) != MPP_OK) {
        std::cerr << "[RGA] Failed to alloc dst buffer\n";
        return false;
    }

    // Setup src and dst info
    im_rect src_rect = { roi_x, roi_y, roi_x + roi_w, roi_y + roi_h };
    im_rect dst_rect = { 0, 0, roi_w, roi_h };

    im2d_image src_img = {};
    src_img.width = src_w;
    src_img.height = src_h;
    src_img.format = IM_FMT_NV12;
    src_img.act_w = src_w;
    src_img.act_h = src_h;
    src_img.vir_w = src_w;
    src_img.vir_h = src_h;
    src_img.fd[0] = mpp_buffer_get_fd(src_buf);
    src_img.fd[1] = -1;

    im2d_image dst_img = src_img;
    dst_img.width = roi_w;
    dst_img.height = roi_h;
    dst_img.act_w = roi_w;
    dst_img.act_h = roi_h;
    dst_img.vir_w = roi_w;
    dst_img.vir_h = roi_h;
    dst_img.fd[0] = mpp_buffer_get_fd(dst_buf);
    dst_img.fd[1] = -1;

    int ret = im2d_crop_and_paste(&src_img, &src_rect, &dst_img, &dst_rect);
    if (ret != 0) {
        std::cerr << "[RGA] Crop failed\n";
        return false;
    }

    // Build MppFrame from dst_buf
    if (mpp_frame_init(&dst) != MPP_OK) return false;
    mpp_frame_set_width(dst, roi_w);
    mpp_frame_set_height(dst, roi_h);
    mpp_frame_set_format(dst, MPP_FMT_NV12);
    mpp_frame_set_buffer(dst, dst_buf);

    return true;
}
