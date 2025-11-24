// Quick tool to generate test sound WAV files
#include <fstream>
#include <cmath>
#include <cstdint>
#include <vector>
#include <string>

void writeWAV(const std::string& filename, double frequency, double duration, double leftVol, double rightVol) {
    const int sampleRate = 44100;
    const int channels = 2;
    const int bitsPerSample = 16;
    int sampleCount = static_cast<int>(sampleRate * duration);
    
    std::vector<int16_t> audioData(sampleCount * channels);
    double amplitude = 32767.0 * 0.5;
    
    for (int i = 0; i < sampleCount; ++i) {
        double t = static_cast<double>(i) / sampleRate;
        double sample = amplitude * std::sin(2.0 * M_PI * frequency * t);
        audioData[i * 2] = static_cast<int16_t>(sample * leftVol);
        audioData[i * 2 + 1] = static_cast<int16_t>(sample * rightVol);
    }
    
    int dataSize = audioData.size() * sizeof(int16_t);
    int fileSize = dataSize + 36;
    
    std::ofstream file(filename, std::ios::binary);
    file.write("RIFF", 4);
    file.write(reinterpret_cast<const char*>(&fileSize), 4);
    file.write("WAVE", 4);
    file.write("fmt ", 4);
    int fmtSize = 16;
    file.write(reinterpret_cast<const char*>(&fmtSize), 4);
    int16_t format = 1;
    file.write(reinterpret_cast<const char*>(&format), 2);
    int16_t ch = channels;
    file.write(reinterpret_cast<const char*>(&ch), 2);
    file.write(reinterpret_cast<const char*>(&sampleRate), 4);
    int byteRate = sampleRate * channels * bitsPerSample / 8;
    file.write(reinterpret_cast<const char*>(&byteRate), 4);
    int16_t blockAlign = channels * bitsPerSample / 8;
    file.write(reinterpret_cast<const char*>(&blockAlign), 2);
    int16_t bps = bitsPerSample;
    file.write(reinterpret_cast<const char*>(&bps), 2);
    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&dataSize), 4);
    file.write(reinterpret_cast<const char*>(audioData.data()), dataSize);
}

int main() {
    writeWAV("sounds/a440.wav", 440.0, 2.0, 0.5, 0.5);        // A440 stereo
    writeWAV("sounds/lowc.wav", 261.63, 2.0, 0.5, 0.5);       // Low C stereo
    writeWAV("sounds/left.wav", 440.0, 2.0, 1.0, 0.0);        // Left only
    writeWAV("sounds/right.wav", 440.0, 2.0, 0.0, 1.0);       // Right only
    return 0;
}
