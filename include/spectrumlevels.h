#pragma once

#include <QMetaType>

#include <array>

struct SpectrumLevels {
    static constexpr int kBandCount = 7;

    std::array<float, kBandCount> bands{};
    bool active = false;
};

Q_DECLARE_METATYPE(SpectrumLevels)
