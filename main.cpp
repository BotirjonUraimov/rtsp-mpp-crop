// RTSP -> MPP decode -> im2d crop -> MPP encode -> RTSP via live555
#include <iostream>
#include "decoder/MppDecoder.h"
#include "encoder/MppEncoder.h"
// #include "crop/Im2dCrop.h"
#include "crop/RgaCrop.h"   
#include "rtsp_client/LiveRtspInput.h"
#include "rtsp_server/LiveRtspOutput.h"
#include <mpp_frame.h>

#define INPUT_RTSP  "rtsp://10.0.0.31/media/1/1/Profile1"
#define OUTPUT_PATH "/tmp/encoded.h264" // temporary pipe file for live555 to read

int main() {
    std::cout << "[INFO] Starting RTSP crop streamer...\n";

    // 1. Init decoder
    std::cout << "[INFO] Initializing MPP decoder...\n";
    MppDecoder decoder;
    if (!decoder.init(MPP_VIDEO_CodingAVC)) {   // H.264 decode
        std::cerr << "[ERROR] Decoder init failed" << std::endl;
        return 1;
    }

    // 2. Init encoder
    std::cout << "[INFO] Initializing MPP encoder...\n";
    MppEncoder encoder(1920, 1080, MPP_VIDEO_CodingAVC);
    if (!encoder.init()) {
        std::cerr << "[ERROR] Encoder init failed" << std::endl;
        return 1;
    }



    // 3. Init cropper
    // Im2dCrop cropper;
    // cropper.set_roi(0, 0, 1920, 1080);
    std::cout << "[INFO] Initializing RGA cropper...\n";
    RgaCrop cropper;                            // <== RgaCrop ishlatyapmiz
    cropper.set_roi(0, 0, 1920, 1080);

    LiveRtspInput input(INPUT_RTSP);
    input.set_on_frame([&](uint8_t* data, size_t size) {
        std::cout << "[DEBUG] Frame size=" << size << " bytes" << std::endl;

        MppFrame decoded;

        std::cout << "[DEBUG] Decoding frame...\n";
    
        if (!decoder.decode(data, size, decoded)) return;
        std::cout << "[DEBUG] Frame decoded successfully\n";

        MppFrame cropped;
        if (!cropper.crop(decoded, cropped)) return;

        std::vector<uint8_t> encoded;
        if (!encoder.encode(cropped, encoded)) return;

        LiveRtspOutput::write_frame(encoded.data(), encoded.size());
    });
    input.start();

    // 5. Start live555 RTSP server
    std::cout << "[INFO] Starting RTSP output server...\n";
    LiveRtspOutput server("/tmp/encoded.h264", 8554, "crop");
    server.run();

    return 0;
}
