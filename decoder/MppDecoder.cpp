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
    std::cout << "[MPP] Decoder initialized for codec type: " 
              << (codec_type == MPP_VIDEO_CodingHEVC ? "H.265" : "H.264") << "\n";

    initialized = true;
    return true;
}

bool MppDecoder::decode(uint8_t* data, size_t size, MppFrame& outFrame) {
    if (!initialized) return false;

    if (data && size > 0) {
        // SPS/PPS/VPS detection (debug uchun)
        int nal_type = (data[0] & 0x7E) >> 1;
        if (nal_type == 32) std::cout << "[NAL] VPS detected\n";
        if (nal_type == 33) std::cout << "[NAL] SPS detected\n";
        if (nal_type == 34) std::cout << "[NAL] PPS detected\n";
        if (nal_type == 19 || nal_type == 20) std::cout << "[NAL] IDR (keyframe) detected\n";

        if (mpp_packet_init(&packet, data, size) != MPP_OK) {
            std::cerr << "[MPP] Failed to init packet\n";
            return false;
        }

        if (mpi->decode_put_packet(ctx, packet) != MPP_OK) {
            std::cerr << "[MPP] Failed to put packet\n";
            mpp_packet_deinit(&packet);
            return false;
        }

        mpp_packet_deinit(&packet);
    }

    // Try multiple times to get a frame
    MppFrame frame = nullptr;
    for (int i = 0; i < 3; i++) {
        if (mpi->decode_get_frame(ctx, &frame) != MPP_OK) {
            std::cerr << "[MPP] Failed to get frame\n";
            return false;
        }
        if (frame) break;
    }

    if (frame) {
        std::cout << "[MPP] Decoded frame: "
                  << mpp_frame_get_width(frame) << "x"
                  << mpp_frame_get_height(frame) << "\n";
        outFrame = frame;
        return true;
    }

    std::cerr << "[MPP] No frame decoded yet\n";
    return false;
}
