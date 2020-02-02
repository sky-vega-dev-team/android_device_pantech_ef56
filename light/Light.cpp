/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.light@2.0-service.ef56"

#include <log/log.h>
#include <fstream>
#include "Light.h"

namespace android {
namespace hardware {
namespace light {
namespace V2_0 {
namespace implementation {

#define LEDS                       "/sys/class/leds/"
#define LCD_LED                    LEDS "lcd-backlight/"
#define BUTTON_LED                 LEDS "gpled_0/"
#define BUTTON1_LED                LEDS "gpled_1/"
#define BUTTON2_LED                LEDS "gpled_2/"
#define BUTTON3_LED                LEDS "gpled_3/"
#define BRIGHTNESS                 "brightness"

/*
 * Write value to path and close file.
 */
static void set(std::string path, std::string value) {
    std::ofstream file(path);
    /* Only write brightness value if stream is open, alive & well */
    if (file.is_open()) {
        file << value;
    } else {
        /* Fire a warning a bail out */
        ALOGE("failed to write %s to %s", value.c_str(), path.c_str());
        return;
    }
}

static void set(std::string path, int value) {
    set(path, std::to_string(value));
}

/*
 * Scale each value of the brightness ramp according to the
 * brightness of the color.
 */

static void handleBacklight(Type /*type*/, const LightState& state) {
    uint32_t brightness = state.color & 0xFF;
    brightness *= 16; // HACK
    set(LCD_LED BRIGHTNESS, brightness);
}

static void handleButtons(Type /*type*/, const LightState& state) {
    uint32_t brightness = state.color & 0xFF;
    brightness *= 16; // HACK
    set(BUTTON_LED BRIGHTNESS, brightness);
    set(BUTTON1_LED BRIGHTNESS, brightness);
    set(BUTTON2_LED BRIGHTNESS, brightness);
    set(BUTTON3_LED BRIGHTNESS, brightness);
}

/*
 * Keep sorted in the order of importance.
 */
static std::map<Type, std::function<void(Type type, const LightState&)>> lights = {
    {Type::BACKLIGHT, handleBacklight},
    {Type::BUTTONS, handleButtons},
};

Light::Light() {}

Return<Status> Light::setLight(Type type, const LightState& state) {
    auto it = lights.find(type);

    if (it == lights.end()) {
        return Status::LIGHT_NOT_SUPPORTED;
    }

    /*
     * Lock global mutex until light state is updated.
     */

    std::lock_guard<std::mutex> lock(globalLock);
    it->second(type, state);
    return Status::SUCCESS;
}

Return<void> Light::getSupportedTypes(getSupportedTypes_cb _hidl_cb) {
    std::vector<Type> types;

    for (auto const& light : lights) types.push_back(light.first);

    _hidl_cb(types);

    return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace light
}  // namespace hardware
}  // namespace android
