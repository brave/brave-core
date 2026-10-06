/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_MODEL_CONSENT_H_
#define BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_MODEL_CONSENT_H_

#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "media/mojo/mojom/speech_recognizer.mojom-forward.h"

namespace content {
class RenderFrameHost;
}  // namespace content

// The hooks `OnDeviceSpeechRecognitionImpl::Install` asks the user through
// before it downloads Brave's on-device speech recognition model. The
// chromium_src override of
// `chrome/browser/speech/on_device_speech_recognition_impl.cc` forward declares
// them, because it cannot depend on this target.

namespace speech {

// Whether `Install` is to ask the user before it downloads. True only for a
// request that would start a download, so false both when nothing is to be
// downloaded and when upstream refuses the request anyway.
bool ShouldRequestBraveOnDeviceSpeechModelConsent(
    const std::vector<std::string>& languages,
    media::mojom::SpeechRecognitionQuality quality);

// Asks the user whether to download the model. Runs `install` with `callback`
// once they agree, and otherwise answers `callback` with false. A Tor window
// and a profile that chose "Don't ask again" are answered without a prompt.
void RequestBraveOnDeviceSpeechModelConsent(
    content::RenderFrameHost& rfh,
    base::OnceCallback<void(base::OnceCallback<void(bool)>)> install,
    base::OnceCallback<void(bool)> callback);

}  // namespace speech

#endif  // BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_MODEL_CONSENT_H_
