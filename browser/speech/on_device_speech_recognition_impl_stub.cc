/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Implements the hooks that the chromium_src override of
// `chrome/browser/speech/on_device_speech_recognition_impl.cc` forward
// declares, for builds without local AI. Those builds ship no Brave model.

#include "chrome/browser/speech/on_device_speech_recognition_impl.h"

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"

namespace speech {

// With no Brave model, there is no upstream branch to send `kDictation` down.
media::mojom::SpeechRecognitionQuality ToBraveOnDeviceSpeechQuality(
    media::mojom::SpeechRecognitionQuality quality) {
  return quality;
}

// With no Brave model, just fall through to upstream.
bool MaybePromptOrCancelBraveOnDeviceSpeechInstall(
    bool frame_can_use_on_device_speech,
    base::WeakPtr<OnDeviceSpeechRecognitionImpl> impl,
    const std::vector<std::string>& languages,
    media::mojom::SpeechRecognitionQuality quality,
    OnDeviceSpeechRecognitionImpl::InstallCallback& callback) {
  return false;
}

}  // namespace speech
