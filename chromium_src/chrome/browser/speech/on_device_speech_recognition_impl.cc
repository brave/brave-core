/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/speech/on_device_speech_recognition_impl.h"

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"

namespace speech {

// Forward declared to avoid adding a compile-time dependency.
// Implementation is provided by //brave/browser/speech:chromium_impl.
media::mojom::SpeechRecognitionQuality ToBraveOnDeviceSpeechQuality(
    media::mojom::SpeechRecognitionQuality quality);
bool MaybePromptOrCancelBraveOnDeviceSpeechInstall(
    bool frame_can_use_on_device_speech,
    base::WeakPtr<OnDeviceSpeechRecognitionImpl> impl,
    const std::vector<std::string>& languages,
    media::mojom::SpeechRecognitionQuality quality,
    OnDeviceSpeechRecognitionImpl::InstallCallback& callback);

}  // namespace speech

#include <chrome/browser/speech/on_device_speech_recognition_impl.cc>
