#include "alsa_player.h"
#include <alsa/asoundlib.h>
#include <fstream>
#include <cstring>
#include <iostream>
#include <unistd.h>  // for usleep

// WAV file header structure
struct WavHeader {
    char riff[4];           // "RIFF"
    uint32_t fileSize;      // File size - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmtSize;       // Format chunk size
    uint16_t audioFormat;   // Audio format (1 = PCM)
    uint16_t numChannels;   // Number of channels
    uint32_t sampleRate;    // Sample rate
    uint32_t byteRate;      // Byte rate
    uint16_t blockAlign;    // Block align
    uint16_t bitsPerSample; // Bits per sample
    char data[4];           // "data"
    uint32_t dataSize;      // Data size
};

bool AlsaPlayer::playWavFile(const std::string& filename, const bool* cancelFlag) {
    std::cout << "ALSA: Attempting to play: " << filename << std::endl;
    std::cout.flush();
    
    // Open WAV file
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "ALSA ERROR: Failed to open file: " << filename << std::endl;
        std::cerr.flush();
        return false;
    }
    
    // Read WAV header
    WavHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));
    
    if (strncmp(header.riff, "RIFF", 4) != 0 || strncmp(header.wave, "WAVE", 4) != 0) {
        std::cerr << "Not a valid WAV file" << std::endl;
        return false;
    }
    
    // Open ALSA PCM device
    snd_pcm_t* pcm_handle;
    int err;
    
    // Try default device first, then pipewire, then hw:0,0
    const char* devices[] = {"default", "pipewire", "hw:0,0", nullptr};
    bool opened = false;
    
    for (int i = 0; devices[i] != nullptr && !opened; i++) {
        std::cout << "ALSA: Trying device: " << devices[i] << std::endl;
        std::cout.flush();
        err = snd_pcm_open(&pcm_handle, devices[i], SND_PCM_STREAM_PLAYBACK, 0);
        if (err >= 0) {
            std::cout << "ALSA: Successfully opened: " << devices[i] << std::endl;
            std::cout.flush();
            opened = true;
        } else {
            std::cerr << "ALSA: Failed to open " << devices[i] << ": " << snd_strerror(err) << std::endl;
            std::cerr.flush();
        }
    }
    
    if (!opened) {
        std::cerr << "Failed to open ALSA device: " << snd_strerror(err) << std::endl;
        std::cerr.flush();
        return false;
    }
    
    // Set hardware parameters
    snd_pcm_hw_params_t* hw_params;
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(pcm_handle, hw_params);
    
    // Set access type
    snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    
    // Set sample format
    snd_pcm_format_t format;
    if (header.bitsPerSample == 16) {
        format = SND_PCM_FORMAT_S16_LE;
    } else if (header.bitsPerSample == 8) {
        format = SND_PCM_FORMAT_U8;
    } else {
        std::cerr << "Unsupported bit depth: " << header.bitsPerSample << std::endl;
        snd_pcm_close(pcm_handle);
        return false;
    }
    snd_pcm_hw_params_set_format(pcm_handle, hw_params, format);
    
    // Set channels
    snd_pcm_hw_params_set_channels(pcm_handle, hw_params, header.numChannels);
    
    // Set sample rate
    unsigned int rate = header.sampleRate;
    snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &rate, 0);
    
    // Apply hardware parameters
    std::cout << "ALSA: Setting hw params (" << header.numChannels << " channels, " 
              << header.sampleRate << " Hz, " << header.bitsPerSample << " bits)" << std::endl;
    err = snd_pcm_hw_params(pcm_handle, hw_params);
    if (err < 0) {
        std::cerr << "ALSA ERROR: Failed to set hw params: " << snd_strerror(err) << std::endl;
        snd_pcm_close(pcm_handle);
        return false;
    }
    std::cout << "ALSA: Hardware configured successfully" << std::endl;
    
    // Prepare device
    snd_pcm_prepare(pcm_handle);
    
    // Read and play audio data
    const size_t bufferSize = 4096;
    char buffer[bufferSize];
    size_t totalRead = 0;
    
    bool muted = false;
    
    while (totalRead < header.dataSize && file) {
        // Check cancel flag before each buffer
        if (cancelFlag && *cancelFlag && !muted) {
            std::cout << "ALSA: Cancellation requested, muting output" << std::endl;
            std::cout.flush();
            
            // Mute by zeroing the buffer - sound plays silently
            muted = true;
        }
        
        size_t toRead = std::min(bufferSize, static_cast<size_t>(header.dataSize - totalRead));
        file.read(buffer, toRead);
        size_t actualRead = file.gcount();
        
        if (actualRead == 0) break;
        
        // If muted, zero the buffer so it plays silently
        if (muted) {
            memset(buffer, 0, actualRead);
        }
        
        // Write to ALSA device
        snd_pcm_sframes_t frames = actualRead / (header.bitsPerSample / 8) / header.numChannels;
        snd_pcm_sframes_t written = snd_pcm_writei(pcm_handle, buffer, frames);
        
        if (written < 0) {
            // Try to recover from errors
            written = snd_pcm_recover(pcm_handle, written, 0);
            if (written < 0) {
                std::cerr << "Write error: " << snd_strerror(written) << std::endl;
                break;
            }
        }
        
        totalRead += actualRead;
    }
    
    // Check if cancelled before draining
    if (cancelFlag && *cancelFlag) {
        std::cout << "ALSA: Skipping drain due to cancellation" << std::endl;
        std::cout.flush();
        snd_pcm_drop(pcm_handle);  // Drop any remaining samples
    } else {
        // Drain remaining samples - this blocks until playback completes
        std::cout << "ALSA: Draining remaining samples..." << std::endl;
        std::cout.flush();
        snd_pcm_nonblock(pcm_handle, 0); // Ensure blocking mode
        err = snd_pcm_drain(pcm_handle);  // Wait for all samples to play
        if (err < 0) {
            std::cerr << "ALSA WARNING: Drain failed: " << snd_strerror(err) << std::endl;
            std::cerr.flush();
            // Drop frames instead if drain failed
            snd_pcm_drop(pcm_handle);
        }
    }
    
    // Close device and ensure it's fully released
    std::cout << "ALSA: Closing device..." << std::endl;
    std::cout.flush();
    err = snd_pcm_close(pcm_handle);
    if (err < 0) {
        std::cerr << "ALSA WARNING: Close failed: " << snd_strerror(err) << std::endl;
        std::cerr.flush();
    }
    file.close();
    
    // Give the audio system a moment to fully release resources
    usleep(50000); // 50ms delay to ensure device is released
    
    std::cout << "ALSA: Playback completed successfully" << std::endl;
    std::cout.flush();
    return true;
}
