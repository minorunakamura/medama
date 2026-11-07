#pragma once

#include <cstdint>

struct EyeState {
    // Gaze coordinates are fractions of the eyeball radius; openness is 0..1.
    float x = 0.0f;
    float y = 0.0f;
    float openness = 1.0f;
};

enum class EyeStyle { Cartoon, Realistic };

class EyeAnimation {
public:
    static constexpr float gazeLimit = 0.35f;

    EyeAnimation(std::uint32_t now, std::uint32_t seed);
    void update(std::uint32_t now);
    const EyeState& state() const { return state_; }
    EyeStyle style() const { return style_; }
    void toggleStyle() {
        style_ = style_ == EyeStyle::Cartoon ? EyeStyle::Realistic : EyeStyle::Cartoon;
    }

private:
    std::uint32_t randomBetween(std::uint32_t minimum, std::uint32_t maximum);

    EyeState state_;
    EyeStyle style_ = EyeStyle::Cartoon;
    std::uint32_t random_;
    std::uint32_t gazeStarted_;
    std::uint32_t gazeDuration_ = 1000;
    bool moving_ = false;
    float fromX_ = 0.0f;
    float fromY_ = 0.0f;
    float targetX_ = 0.0f;
    float targetY_ = 0.0f;
    std::uint32_t blinkStarted_;
    std::uint32_t blinkWait_;
    bool blinking_ = false;
};
