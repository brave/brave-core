/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_UTIL_H_
#define BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_UTIL_H_

#include <string_view>

#include "media/mojo/mojom/speech_recognizer.mojom.h"

namespace speech {

// Brave's on-device speech recognition availability, in place of upstream's.
// The chromium_src override of
// `chrome/browser/speech/on_device_speech_recognition_util.cc` forward declares
// this, because it cannot depend on this target.
media::mojom::AvailabilityStatus GetBraveOnDeviceSpeechAvailability(
    std::string_view language,
    media::mojom::SpeechRecognitionQuality quality);

}  // namespace speech

#endif  // BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_UTIL_H_
