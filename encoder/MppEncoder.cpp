#include "MppEncoder.h"
#include <iostream>
#include <cstring>

MppEncoder::MppEncoder(int w, int h)
    : width(w), height(h), ctx(nullptr), mpi(nullptr), cfg(nullptr) {}

MppEncoder::~MppEncoder() {
    if (ctx) mpp_destroy(ctx);
}

bool MppEncoder::init() {
    if (mpp_create(&ctx, &mpi) != MPP_OK) {
        std::cerr << "[ENC] Failed to create encoder ctx\n";
        return false;
    }

    if (mpp_init(ctx, MPP_CTX_ENC, MPP_VIDEO_CodingAVC) != MPP_OK) {
        std::cerr << "[ENC] Failed to init encoder\n";
        return false;
    }

    if (mpi->control(ctx, MPP_ENC_GET_CFG, &cfg) != MPP_OK) {
        std::cerr << "[ENC] Failed to get encoder config\n";
        return false;
    }

    // Configure resolution
    mpp_enc_cfg_set_s32(cfg, "prep:width",        width);
    mpp_enc_cfg_set_s32(cfg, "prep:height",       height);
    mpp_enc_cfg_set_s32(cfg, "prep:hor_stride",   width);
    mpp_enc_cfg_set_s32(cfg, "prep:ver_stride",   height);
    mpp_enc_cfg_set_s32(cfg, "prep:format",       MPP_FMT_NV12);

    // Coding settings
    mpp_enc_cfg_set_s32(cfg, "rc:mode",           MPP_ENC_RC_MODE_CBR);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_flex",    0);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_num",     15);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_den",     1);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_num",    15);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_den",    1);
    mpp_enc_cfg_set_s32(cfg, "rc:bitrate",        2000000); // 2 Mbps

    mpp_enc_cfg_set_s32(cfg, "codec:type",        MPP_VIDEO_CodingAVC);
    mpp_enc_cfg_set_s32(cfg, "h264:profile",      100); // High
    mpp_enc_cfg_set_s32(cfg, "h264:level",        40);

    if (mpi->control(ctx, MPP_ENC_SET_CFG, cfg) != MPP_OK) {
        std::cerr << "[ENC] Failed to set encoder config\n";
        return false;
    }

    initialized = true;
    return true;
}

bool MppEncoder::encode(MppFrame frame, std::vector<uint8_t>& out) {
    if (!initialized || !frame) return false;

    if (mpi->encode_put_frame(ctx, frame) != MPP_OK) {
        std::cerr << "[ENC] Failed to put frame\n";
        return false;
    }

    MppPacket packet = nullptr;
    if (mpi->encode_get_packet(ctx, &packet) != MPP_OK || !packet) {
        std::cerr << "[ENC] Failed to get packet\n";
        return false;
    }

    void* pkt_ptr = mpp_packet_get_pos(packet);
    size_t pkt_len = mpp_packet_get_length(packet);
    uint8_t* data = static_cast<uint8_t*>(pkt_ptr);

    out.assign(data, data + pkt_len);

    mpp_packet_deinit(&packet);
    return true;
}
