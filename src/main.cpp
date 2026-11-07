#include <Arduino.h>
#include <M5Unified.h>
#include <utility/M5IOE1_Class.hpp>
#include <esp_system.h>
#include <soc/soc_memory_types.h>

#include <cmath>

#include "eye_animation.h"

static_assert(BOARD_ID == m5gfx::board_t::board_M5StopWatch,
              "BOARD_ID must select the StopWatch startup sequence");

namespace {
M5Canvas canvas(&M5.Display);
EyeAnimation animation(0, 1);
bool canvasReady = false;
std::uint32_t lastFrame = 0;
constexpr std::uint32_t frameInterval = 33;

bool prepareStopWatchIo() {
    m5::I2C_Class bus;
    if (!bus.begin(I2C_NUM_1, 47, 48)) {
        return false;
    }
    m5::M5PM1_Class power(0x6E, 100000, &bus);
    m5::M5IOE1_Class io(0x4F, 100000, &bus);
    bool ready = power.begin() && power.setDCDCOutput(true) && power.setLDOOutput(true);
    const std::uint32_t started = millis();
    if (ready) {
        // IOE1 can still be booting after M5GFX's 500 ms startup window.
        while (!(ready = io.begin())) {
            if (static_cast<std::uint32_t>(millis() - started) >= 3000) {
                break;
            }
            delay(5);
        }
    }
    const bool released = bus.release();
    return ready && released;
}

void renderCartoonEye(const EyeState& eye) {
    const int width = canvas.width();
    const int height = canvas.height();
    const int cx = width / 2;
    const int cy = height / 2;
    const int radius = (width < height ? width : height) / 2;
    canvas.fillScreen(TFT_BLACK);
    canvas.fillCircle(cx, cy, radius, canvas.color565(176, 176, 176));
    canvas.fillEllipse(cx, cy - radius * 10 / 100,
                       radius * 102 / 100, radius * 97 / 100, TFT_WHITE);

    const int irisX = cx + static_cast<int>(eye.x * radius);
    const int irisY = cy + static_cast<int>(eye.y * radius);
    const int irisRadius = radius * 48 / 100;
    const int pupilRadius = radius * 16 / 100;
    canvas.fillCircle(irisX, irisY, irisRadius, canvas.color565(151, 0, 0));
    canvas.fillEllipse(irisX, irisY - irisRadius * 6 / 100,
                       irisRadius * 98 / 100, irisRadius * 92 / 100,
                       canvas.color565(197, 0, 0));
    canvas.fillCircle(irisX, irisY, pupilRadius, TFT_BLACK);
    canvas.fillCircle(irisX + irisRadius * 36 / 100,
                      irisY - irisRadius * 35 / 100,
                      irisRadius * 23 / 100, canvas.color565(239, 184, 184));
}

void renderRealisticEye(const EyeState& eye) {
    const int width = canvas.width();
    const int height = canvas.height();
    const int cx = width / 2;
    const int cy = height / 2;
    const int radius = (width < height ? width : height) / 2;
    canvas.fillScreen(TFT_BLACK);
    for (int layer = 0; layer <= 24; ++layer) {
        const int inset = radius * layer / 48;
        canvas.fillCircle(cx - inset / 5, cy - inset / 4, radius - inset,
                          canvas.color565(178 + layer * 3, 183 + layer * 3,
                                          180 + layer * 3));
    }

    const int irisX = cx + static_cast<int>(eye.x * radius);
    const int irisY = cy + static_cast<int>(eye.y * radius);
    const int irisRadius = radius * 48 / 100;
    const int pupilRadius = irisRadius * 38 / 100;
    for (int layer = 0; layer <= 24; ++layer) {
        canvas.fillCircle(irisX, irisY, irisRadius - irisRadius * layer / 32,
                          canvas.color565(43 + layer * 2, 66 + layer * 3,
                                          70 + layer * 3));
    }
    // Fixed iris-local spokes: no random sampling or frame-dependent shimmer.
    for (int spoke = 0; spoke < 72; ++spoke) {
        const float angle = spoke * 6.2831853f / 72;
        const float dx = std::cos(angle);
        const float dy = std::sin(angle);
        const int outer = irisRadius * (88 + spoke % 7) / 100;
        const int inner = pupilRadius + irisRadius * (spoke % 5) / 100;
        const auto color = spoke % 3 == 0 ? canvas.color565(108, 145, 140)
                                          : canvas.color565(67, 102, 105);
        canvas.drawLine(irisX + static_cast<int>(dx * inner),
                        irisY + static_cast<int>(dy * inner),
                        irisX + static_cast<int>(dx * outer),
                        irisY + static_cast<int>(dy * outer), color);
    }
    canvas.fillCircle(irisX, irisY, pupilRadius + radius / 100,
                      canvas.color565(35, 49, 51));
    canvas.fillCircle(irisX, irisY, pupilRadius, canvas.color565(8, 14, 18));
    canvas.fillEllipse(irisX + irisRadius * 30 / 100,
                       irisY - irisRadius * 34 / 100,
                       irisRadius * 20 / 100, irisRadius * 14 / 100,
                       canvas.color565(242, 248, 244));
    canvas.fillCircle(irisX - irisRadius * 29 / 100,
                      irisY + irisRadius * 30 / 100,
                      irisRadius * 7 / 100, canvas.color565(184, 207, 208));
}

void renderEye(const EyeState& eye, EyeStyle style) {
    if (style == EyeStyle::Cartoon) {
        renderCartoonEye(eye);
    } else {
        renderRealisticEye(eye);
    }
    const int width = canvas.width();
    const int height = canvas.height();
    const int cx = width / 2;
    const int cy = height / 2;
    const int radius = (width < height ? width : height) / 2;
    const int aperture = static_cast<int>(radius * eye.openness);
    canvas.fillRect(0, 0, width, cy - aperture, TFT_BLACK);
    canvas.fillRect(0, cy + aperture, width, height - cy - aperture, TFT_BLACK);
    if (aperture == 0) {
        canvas.drawLine(cx - radius / 2, cy, cx + radius / 2, cy, TFT_DARKGREY);
    }
    canvas.pushSprite(0, 0);
}
}  // namespace

void setup() {
    if (!prepareStopWatchIo()) {
        Serial.begin(115200);
        Serial.println("StopWatch IO controller unavailable");
        return;
    }
    auto cfg = M5.config();
    cfg.serial_baudrate = 0;
    cfg.internal_imu = false;
    cfg.internal_rtc = false;
    cfg.internal_mic = false;
    cfg.internal_spk = false;
    cfg.external_display_value = 0;
    cfg.output_power = false;
    cfg.pmic_button = false;
    cfg.disable_rtc_irq = false;
    M5.begin(cfg);

    canvas.setColorDepth(16);
    canvas.setPsram(true);
    void* buffer = psramFound()
        ? canvas.createSprite(M5.Display.width(), M5.Display.height()) : nullptr;
    // M5GFX can fall back to internal RAM if its PSRAM allocation fails.
    canvasReady = buffer != nullptr && esp_ptr_external_ram(buffer);
    if (!canvasReady) {
        canvas.deleteSprite();
        M5.Display.fillScreen(TFT_BLACK);
        M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Display.setCursor(20, M5.Display.height() / 2);
        M5.Display.print("Eye buffer unavailable");
        return;
    }
    const std::uint32_t now = millis();
    animation = EyeAnimation(now, esp_random());
    lastFrame = now;
    renderEye(animation.state(), animation.style());
}

void loop() {
    if (!canvasReady) {
        return;
    }
    M5.update();
    // Consume each release-click once, even when this loop skips rendering.
    if (M5.BtnA.wasClicked()) {
        animation.toggleStyle();
    }
    const std::uint32_t now = millis();
    if (static_cast<std::uint32_t>(now - lastFrame) < frameInterval) {
        return;
    }
    lastFrame = now;
    animation.update(now);
    renderEye(animation.state(), animation.style());
}
