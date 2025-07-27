// RTSP -> MPP decode -> im2d crop -> MPP encode -> RTSP via live555
#include <iostream>
#include "decoder/MppDecoder.h"
#include "encoder/MppEncoder.h"
// #include "crop/Im2dCrop.h"
#include "crop/RgaCrop.h"   
#include "rtsp_client/LiveRtspInput.h"
#include "rtsp_server/LiveRtspOutput.h"

#define INPUT_RTSP  "rtsp://10.0.0.31/media/1/1/Profile1"
#define OUTPUT_PATH "/tmp/encoded.h264" // temporary pipe file for live555 to read

int main() {
    std::cout << "[INFO] Starting RTSP crop streamer...\n";

    // 1. Init decoder
    MppDecoder decoder;
    if (!decoder.init(MPP_VIDEO_CodingAVC)) {
        std::cerr << "[ERROR] Decoder init failed" << std::endl;
        return 1;
    }

    // 2. Init encoder
    MppEncoder encoder(1920, 1080);
    if (!encoder.init()) {
        std::cerr << "[ERROR] Encoder init failed" << std::endl;
        return 1;
    }

    // 3. Init cropper
    // Im2dCrop cropper;
    // cropper.set_roi(0, 0, 1920, 1080);
    RgaCrop cropper;                            // <== RgaCrop ishlatyapmiz
    cropper.set_roi(0, 0, 1920, 1080);

    // 4. Start RTSP input thread
    LiveRtspInput input(INPUT_RTSP);
    input.set_on_frame([&](uint8_t* data, size_t size) {
        MppFrame frame;
        if (!decoder.decode(data, size, frame)) return;

        MppFrame cropped;
        if (!cropper.crop(frame, cropped)) return;

        std::vector<uint8_t> encoded;
        if (!encoder.encode(cropped, encoded)) return;

        LiveRtspOutput::write_frame(encoded.data(), encoded.size());
    });
    input.start();

    // 5. Start live555 RTSP server
    LiveRtspOutput server("/tmp/encoded.h264", 8554, "crop");
    server.run();

    return 0;
}
