#ifndef ALSA_PLAYER_H
#define ALSA_PLAYER_H

#include <string>

// Direct ALSA API player - talks to Linux audio HAL
class AlsaPlayer {
public:
    // Play a WAV file using ALSA API directly
    // Returns true on success, false on failure
    // If cancelFlag is provided and becomes true, playback stops immediately
    static bool playWavFile(const std::string& filename, const bool* cancelFlag = nullptr);
};

#endif // ALSA_PLAYER_H
