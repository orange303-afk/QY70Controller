#pragma once

#include <string_view>

namespace qy70
{
std::string_view voiceModeName(int bankMsb);
std::string_view voiceName(int bankMsb, int bankLsb, int programNumber);
} // namespace qy70
