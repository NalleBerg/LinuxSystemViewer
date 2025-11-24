#include "alsa_player.h"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <wav_file>" << std::endl;
        return 1;
    }
    
    std::cout << "Testing ALSA player..." << std::endl;
    bool result = AlsaPlayer::playWavFile(argv[1]);
    
    if (result) {
        std::cout << "SUCCESS: Sound played" << std::endl;
        return 0;
    } else {
        std::cerr << "FAILED: Could not play sound" << std::endl;
        return 1;
    }
}
