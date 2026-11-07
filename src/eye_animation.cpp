#include "eye_animation.h"

EyeAnimation::EyeAnimation(std::uint32_t now, std::uint32_t seed)
    : random_(seed ? seed : 1), gazeStarted_(now), blinkStarted_(now),
      blinkWait_(randomBetween(2500, 5500)) {}

std::uint32_t EyeAnimation::randomBetween(std::uint32_t minimum, std::uint32_t maximum) {
    random_ ^= random_ << 13;
    random_ ^= random_ >> 17;
    random_ ^= random_ << 5;
    return minimum + random_ % (maximum - minimum + 1);
}

void EyeAnimation::update(std::uint32_t now) {
    const std::uint32_t gazeElapsed = now - gazeStarted_;
    if (gazeElapsed >= gazeDuration_) {
        // Rebase at now: a late frame never replays an unbounded backlog.
        gazeStarted_ = now;
        if (moving_) {
            state_.x = targetX_;
            state_.y = targetY_;
            moving_ = false;
            gazeDuration_ = randomBetween(700, 1800);
        } else {
            fromX_ = state_.x;
            fromY_ = state_.y;
            targetX_ = (static_cast<float>(randomBetween(0, 2000)) / 1000.0f - 1.0f) * gazeLimit;
            targetY_ = (static_cast<float>(randomBetween(0, 2000)) / 1000.0f - 1.0f) * gazeLimit;
            moving_ = true;
            gazeDuration_ = 320;
        }
    } else if (moving_) {
        float t = static_cast<float>(gazeElapsed) / gazeDuration_;
        t = t * t * (3.0f - 2.0f * t);
        state_.x = fromX_ + (targetX_ - fromX_) * t;
        state_.y = fromY_ + (targetY_ - fromY_) * t;
    }

    const std::uint32_t blinkElapsed = now - blinkStarted_;
    if (!blinking_) {
        if (blinkElapsed >= blinkWait_) {
            blinking_ = true;
            blinkStarted_ = now;
        }
    } else if (blinkElapsed < 80) {
        state_.openness = 1.0f - static_cast<float>(blinkElapsed) / 80.0f;
    } else if (blinkElapsed < 120) {
        state_.openness = 0.0f;
    } else if (blinkElapsed < 240) {
        state_.openness = static_cast<float>(blinkElapsed - 120) / 120.0f;
    } else {
        state_.openness = 1.0f;
        blinking_ = false;
        blinkStarted_ = now;
        blinkWait_ = randomBetween(2500, 5500);
    }
}
