/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/speech/on_device_speech_recognition_impl.h"

#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace speech {

// The hooks `OnDeviceSpeechRecognitionImpl::Install` asks the user through
// before it downloads Brave's on-device speech recognition model. See the
// plaster rewrite of this source.
#if BUILDFLAG(ENABLE_LOCAL_AI)
// Forward declared to avoid adding a compile-time dependency.
// Implementation is provided by //brave/browser/speech:chromium_impl.
bool ShouldRequestBraveOnDeviceSpeechModelConsent(
    const std::vector<std::string>& languages,
    media::mojom::SpeechRecognitionQuality quality);
void RequestBraveOnDeviceSpeechModelConsent(
    content::RenderFrameHost& rfh,
    base::OnceCallback<void(base::OnceCallback<void(bool)>)> install,
    base::OnceCallback<void(bool)> callback);
#else
// Nothing to ask without Brave's model, so `Install` runs as upstream does.
inline bool ShouldRequestBraveOnDeviceSpeechModelConsent(
    const std::vector<std::string>&,
    media::mojom::SpeechRecognitionQuality) {
  return false;
}
inline void RequestBraveOnDeviceSpeechModelConsent(
    content::RenderFrameHost&,
    base::OnceCallback<void(base::OnceCallback<void(bool)>)>,
    base::OnceCallback<void(bool)>) {}
#endif

}  // namespace speech

#include <chrome/browser/speech/on_device_speech_recognition_impl.cc>
