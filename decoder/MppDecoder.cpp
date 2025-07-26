#include "MppDecoder.h"
#include <iostream>

MppDecoder::MppDecoder()
    : ctx(nullptr), mpi(nullptr), packet(nullptr), codec_type(MPP_VIDEO_CodingAVC) {}

MppDecoder::~MppDecoder() {
    if (packet) mpp_packet_deinit(&packet);
    if (ctx) mpp_destroy(ctx);
}

bool MppDecoder::init(MppCodingType type) {
    codec_type = type;

    if (mpp_create(&ctx, &mpi) != MPP_OK) {
        std::cerr << "[MPP] Failed to create decoder ctx\n";
        return false;
    }

    if (mpp_init(ctx, MPP_CTX_DEC, codec_type) != MPP_OK) {
        std::cerr << "[MPP] Failed to init decoder ctx\n";
        return false;
    }

    initialized = true;
    return true;
}

bool MppDecoder::decode(uint8_t* data, size_t size, MppFrame& outFrame) {
    if (!initialized) return false;

    // Init packet
    if (mpp_packet_init(&packet, data, size) != MPP_OK) {
        std::cerr << "[MPP] Failed to init packet\n";
        return false;
    }

    // Send packet to decoder
    if (mpi->decode_put_packet(ctx, packet) != MPP_OK) {
        std::cerr << "[MPP] Failed to put packet\n";
        return false;
    }

    // Get decoded frame
    MppTask task = nullptr;
    MppFrame frame = nullptr;

    if (mpi->decode_get_frame(ctx, &frame) != MPP_OK) {
        std::cerr << "[MPP] Failed to get frame\n";
        return false;
    }

    if (frame) {
        outFrame = frame; // pass it out
        return true;
    }

    return false;
}
