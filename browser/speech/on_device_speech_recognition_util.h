/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_UTIL_H_
#define BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_UTIL_H_

#include <string_view>

#include "base/functional/callback_forward.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"

class PrefService;

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace content {
class BrowserContext;
}  // namespace content

namespace speech {

// Brave's on-device speech recognition availability check. Replaces the
// upstream SODA / optimization-guide computation (Brave ships neither). Both
// the `available()` query and the session-start path resolve to this through
// the chromium_src replacement of
// `speech::GetOnDeviceSpeechRecognitionAvailabilityStatus`.
//
// Brave serves the `kCommand` and `kDictation` qualities in English only, from
// a model delivered by the component updater. Reports `kUnavailable` when the
// feature is off (there is no on-device backend at all), `kDownloadable` until
// the model is installed, and `kAvailable` once it is.
media::mojom::AvailabilityStatus GetBraveOnDeviceSpeechAvailability(
    content::BrowserContext* context,
    std::string_view language,
    media::mojom::SpeechRecognitionQuality quality);

// Whether Brave's own model serves `quality`. False when the feature is off,
// because Brave then has no on-device backend at all.
bool UsesBraveOnDeviceSpeech(media::mojom::SpeechRecognitionQuality quality);

// Starts, or joins, an install of Brave's on-device speech model, running
// `callback` with whether the model ended up installed. Carries the reply to
// `SpeechRecognition.install()`, so `callback` always runs: right away when the
// model is already on disk, on the download's first terminal outcome
// otherwise, and with `false` when no download can be started at all.
//
// The first call records the activation pref, which is what gates startup
// registration of the component, so a user who never asks for on-device speech
// never downloads the model.
void InstallBraveOnDeviceSpeechModel(
    PrefService* local_state,
    component_updater::ComponentUpdateService* component_updater,
    base::OnceCallback<void(bool)> callback);

}  // namespace speech

#endif  // BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_UTIL_H_
