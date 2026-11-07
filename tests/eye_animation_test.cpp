#include "eye_animation.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>

namespace {
void assertRange(const EyeState& eye) {
    assert(std::isfinite(eye.x) && std::isfinite(eye.y) && std::isfinite(eye.openness));
    assert(std::fabs(eye.x) <= EyeAnimation::gazeLimit);
    assert(std::fabs(eye.y) <= EyeAnimation::gazeLimit);
    assert(eye.openness >= 0.0f && eye.openness <= 1.0f);
    // The iris (radius 0.48) stays inside the eyeball.
    assert(std::hypot(eye.x, eye.y) + 0.48f < 1.0f);
}

void gazeMovesAndHolds() {
    EyeAnimation eye(0, 42);
    assert(eye.state().x == 0.0f && eye.state().y == 0.0f);
    assert(eye.state().openness == 1.0f);
    eye.update(999);
    assert(eye.state().x == 0.0f && eye.state().y == 0.0f);
    eye.update(1000);
    eye.update(1160);
    const EyeState midway = eye.state();
    assert(midway.x != 0.0f || midway.y != 0.0f);
    eye.update(1320);
    const EyeState target = eye.state();
    assert(std::fabs(target.x - 2.0f * midway.x) < 0.00001f);
    assert(std::fabs(target.y - 2.0f * midway.y) < 0.00001f);
    eye.update(1500);
    eye.update(2019);
    assert(eye.state().x == target.x && eye.state().y == target.y);
}

void blinkClosesAndReopensWithinRange() {
    EyeAnimation eye(0, 42);
    bool closing = false;
    bool closed = false;
    bool reopening = false;
    bool reopened = false;
    for (std::uint32_t now = 0; now <= 30000; now += 10) {
        eye.update(now);
        const EyeState& state = eye.state();
        assertRange(state);
        if (now < 2500) {
            assert(state.openness == 1.0f);
        }
        if (!closed && state.openness > 0.0f && state.openness < 1.0f) closing = true;
        if (state.openness == 0.0f) closed = true;
        if (closed && state.openness > 0.0f && state.openness < 1.0f) reopening = true;
        if (reopening && state.openness == 1.0f) reopened = true;
    }
    assert(closing && closed && reopening && reopened);
}

void styleAlternatesWithoutResettingMotion() {
    EyeAnimation switched(0, 42);
    EyeAnimation unchanged(0, 42);
    assert(switched.style() == EyeStyle::Cartoon);
    bool switchedBackDuringBlink = false;
    for (std::uint32_t now = 0; now <= 15000; now += 10) {
        switched.update(now);
        unchanged.update(now);
        if (now == 1160) {
            assert(switched.state().x != 0.0f || switched.state().y != 0.0f);
            switched.toggleStyle();
            assert(switched.style() == EyeStyle::Realistic);
            EyeAnimation restarted(now, 42);
            assert(restarted.style() == EyeStyle::Cartoon);
        }
        if (!switchedBackDuringBlink && unchanged.state().openness == 0.0f) {
            assert(switched.style() == EyeStyle::Realistic);
            switched.toggleStyle();
            assert(switched.style() == EyeStyle::Cartoon);
            switchedBackDuringBlink = true;
        }
        // Also compare future phases and random choices after both switches.
        assert(switched.state().x == unchanged.state().x);
        assert(switched.state().y == unchanged.state().y);
        assert(switched.state().openness == unchanged.state().openness);
    }
    assert(switchedBackDuringBlink);
}

void millisWraparound() {
    const std::uint32_t start = std::numeric_limits<std::uint32_t>::max() - 500;
    EyeAnimation ordinary(0, 73);
    EyeAnimation wrapped(start, 73);
    for (std::uint32_t elapsed = 0; elapsed <= 15000; elapsed += 10) {
        ordinary.update(elapsed);
        wrapped.update(start + elapsed);
        assert(ordinary.state().x == wrapped.state().x);
        assert(ordinary.state().y == wrapped.state().y);
        assert(ordinary.state().openness == wrapped.state().openness);
        assertRange(wrapped.state());
    }
}

void lateFramesRecover() {
    EyeAnimation eye(0, 42);
    const std::uint32_t late = 0x70000000;
    eye.update(late);
    assertRange(eye.state());
    eye.update(late + 80);
    assert(eye.state().openness == 0.0f);
    eye.update(late + 240);
    assert(eye.state().openness == 1.0f);
    eye.update(late + 320);
    const EyeState target = eye.state();
    assert(target.x != 0.0f || target.y != 0.0f);
    eye.update(late + 500);
    assert(eye.state().x == target.x && eye.state().y == target.y);
    eye.update(late + 10000);
    assertRange(eye.state());
    eye.update(late + 10080);
    assert(eye.state().openness == 0.0f);
    eye.update(late + 1000000);
    assertRange(eye.state());
    assert(eye.state().openness == 1.0f);
    eye.update(late + 1000010);
    assert(eye.state().openness == 1.0f);
}
}  // namespace

int main() {
    assert(std::hypot(EyeAnimation::gazeLimit, EyeAnimation::gazeLimit) + 0.48f < 1.0f);
    gazeMovesAndHolds();
    blinkClosesAndReopensWithinRange();
    styleAlternatesWithoutResettingMotion();
    millisWraparound();
    lateFramesRecover();
    std::puts("eye_animation: all assertions passed");
}
