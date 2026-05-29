#pragma once

#include <QMetaType>

#include <array>

struct SpectrumLevels {
    static constexpr int kChannelCount = 2;
    static constexpr int kBandCount = 7;
    static constexpr int kBlockCount = 6;

    std::array<std::array<int, kBandCount>, kChannelCount> channels{};
    bool active = false;
};

Q_DECLARE_METATYPE(SpectrumLevels)
