#pragma once

#include <string_view>
#include <vector>

namespace qy70
{
struct VoiceDescriptor
{
    int bankMsb;
    int program;
    int bankLsb;
    std::string_view name;
    std::string_view category;
};

std::string_view voiceModeName(int bankMsb);
std::string_view voiceName(int bankMsb, int bankLsb, int programNumber);
std::string_view voiceCategory(int bankMsb, int programNumber);
std::vector<VoiceDescriptor> voiceCatalog();
} // namespace qy70
