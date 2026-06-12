/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_UTIL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_UTIL_H_

#include "base/functional/callback_forward.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"

namespace speech {

// Backend selection for on-device speech recognition, which is what `Available`
// and `Install` need from Brave. It belongs next to
// `GetOnDeviceSpeechRecognitionAvailabilityStatus` because that function is
// already the availability half of the same question, so replacing this file
// answers both halves and the two cannot disagree.

// Whether any on-device backend serves `quality`. Upstream that is SODA for
// kCommand and the optimization guide, behind its own feature flags, for
// kConversation and kDictation. For Brave it is whatever its own model serves.
bool IsOnDeviceSpeechQualitySupported(
    media::mojom::SpeechRecognitionQuality quality);

// Whether Brave's own model serves `quality`, and so whether the request takes
// the install path below rather than upstream's SODA or optimization-guide
// arms. False when local AI is not built in.
bool UsesBraveOnDeviceSpeech(media::mojom::SpeechRecognitionQuality quality);

// Installs Brave's on-device speech model, running `callback` with whether the
// model ended up installed. Only called when `UsesBraveOnDeviceSpeech` is true,
// and `callback` always runs, because it carries the reply to
// `SpeechRecognition.install()`.
//
// This overload exists so the call site does not have to reach for the browser
// process globals. It forwards to the overload in
// brave/browser/speech/on_device_speech_recognition_util.h, which takes them as
// arguments so that nothing below //chrome/browser depends on them.
void InstallBraveOnDeviceSpeechModel(base::OnceCallback<void(bool)> callback);

}  // namespace speech

#include <chrome/browser/speech/on_device_speech_recognition_util.h>  // IWYU pragma: export

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_UTIL_H_
