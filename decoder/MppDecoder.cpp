// #include "MppDecoder.h"
// #include <iostream>


// MppDecoder::MppDecoder()
//     : ctx(nullptr), mpi(nullptr), packet(nullptr), codec_type(MPP_VIDEO_CodingAVC) {}

// MppDecoder::~MppDecoder() {
//     if (packet) mpp_packet_deinit(&packet);
//     if (ctx) mpp_destroy(ctx);
// }

// bool MppDecoder::init(MppCodingType type) {
//     codec_type = type;

//     if (mpp_create(&ctx, &mpi) != MPP_OK) {
//         std::cerr << "[MPP] Failed to create decoder ctx\n";
//         return false;
//     }

//     if (mpp_init(ctx, MPP_CTX_DEC, codec_type) != MPP_OK) {
//         std::cerr << "[MPP] Failed to init decoder ctx\n";
//         return false;
//     }
//     std::cout << "[MPP] Decoder initialized for codec type: " 
//               << (codec_type == MPP_VIDEO_CodingAVC ? "H.264" : "H.265") << "\n";

//     initialized = true;
//     return true;
// }


// bool MppDecoder::decode(uint8_t* data, size_t size, MppFrame& outFrame) {
//     if (!initialized || !data || size <= 0) return false;

//     std::cout << "[MPP] Decoding frame of size: " << size << " bytes\n";

//     // H.264 nal_type aniqlash
//     // int nal_type = data[0] & 0x1F;
//     int nal_type = data[0];
//     std::cout << "[NAL] Type: " << nal_type << "\n";

//     std::vector<uint8_t> annexb;

//     if (nal_type == 24) {  // H.264 STAP-A (0x18 = 24)
//         std::cout << "[NAL] STAP-A detected. Parsing multiple NAL units...\n";
//         size_t offset = 1; // STAP-A header skip

//         while (offset + 2 <= size) {
//             uint16_t nalSize = (data[offset] << 8) | data[offset + 1];
//             offset += 2;
//             if (offset + nalSize > size) break;

//             annexb.insert(annexb.end(), {0,0,0,1}); // Annex-B start code
//             annexb.insert(annexb.end(), data + offset, data + offset + nalSize);

//             int innerType = data[offset] & 0x1F;
//             std::cout << "   [NAL] Inner type: " << innerType
//                       << " size=" << nalSize << "\n";

//             offset += nalSize;
//         }
//     } else {
//         // Oddiy bitta NAL
//         annexb.insert(annexb.end(), {0,0,0,1});
//         annexb.insert(annexb.end(), data, data + size);
//     }

//     if (!annexb.empty()) {
//         if (mpp_packet_init(&packet, annexb.data(), annexb.size()) != MPP_OK) {
//             std::cerr << "[MPP] Failed to init packet\n";
//             return false;
//         }

//         if (mpi->decode_put_packet(ctx, packet) != MPP_OK) {
//             std::cerr << "[MPP] Failed to put packet\n";
//             mpp_packet_deinit(&packet);

//             return false;
//         }
//         mpp_packet_deinit(&packet);
//     }

//     // Frame olish
//     MppFrame frame = nullptr;
//     for (int i = 0; i < 3; i++) {
//         if (mpi->decode_get_frame(ctx, &frame) != MPP_OK) {
//             std::cerr << "[MPP] Failed to get frame\n";
//             return false;
//         }
//         if (frame) break;
//     }

//     if (frame) {
//         std::cout << "[MPP] Decoded frame: "
//                   << mpp_frame_get_width(frame) << "x"
//                   << mpp_frame_get_height(frame) << "\n";
//         outFrame = frame;
//         return true;
//     }

//     std::cerr << "[MPP] No frame decoded yet\n";
//     return false;
// }



#include "MppDecoder.h"
#include <iostream>
#include <cstring>

MppDecoder::MppDecoder()
    : ctx(nullptr), mpi(nullptr), codec_type(MPP_VIDEO_CodingAVC) {}

MppDecoder::~MppDecoder() {
    if (ctx) {
        mpp_destroy(ctx);
        ctx = nullptr;
    }
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

    std::cout << "[MPP] Decoder initialized (" 
              << (codec_type == MPP_VIDEO_CodingAVC ? "H.264" : "H.265") << ")\n";
    initialized = true;
    return true;
}

bool MppDecoder::decode(uint8_t* data, size_t size, MppFrame& outFrame) {
    if (!initialized || !data || size == 0) return false;

    std::vector<uint8_t> annexb;
    annexb.reserve(size + 4);

    // Always prepend Annex-B start code
    static const uint8_t startCode[4] = {0x00, 0x00, 0x00, 0x01};
    annexb.insert(annexb.end(), startCode, startCode + 4);
    annexb.insert(annexb.end(), data, data + size);

    MppPacket packet = nullptr;
    if (mpp_packet_init(&packet, annexb.data(), annexb.size()) != MPP_OK) {
        std::cerr << "[MPP] Failed to init packet\n";
        return false;
    }

    if (mpi->decode_put_packet(ctx, packet) != MPP_OK) {
        std::cerr << "[MPP] Failed to put packet\n";
        mpp_packet_deinit(&packet);
        return false;
    }
    mpp_packet_deinit(&packet);

    // Poll decoder for output frame
    for (int i = 0; i < 3; i++) {
        MppFrame frame = nullptr;
        if (mpi->decode_get_frame(ctx, &frame) != MPP_OK) {
            std::cerr << "[MPP] Failed to get frame\n";
            return false;
        }
        if (frame) {
            if (mpp_frame_get_info_change(frame)) {
                std::cout << "[MPP] Info change: resolution=" 
                          << mpp_frame_get_width(frame) << "x"
                          << mpp_frame_get_height(frame) << "\n";
                mpi->control(ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
            } else if (!mpp_frame_get_errinfo(frame) && !mpp_frame_get_discard(frame)) {
                std::cout << "[MPP] Got frame " 
                          << mpp_frame_get_width(frame) << "x"
                          << mpp_frame_get_height(frame) << "\n";
                outFrame = frame;
                return true;
            }
            mpp_frame_deinit(&frame); // release if not used
        }
    }

    std::cerr << "[MPP] No valid frame decoded yet\n";
    return false;
}
