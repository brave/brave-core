/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/speech/on_device_speech_recognition_impl.h"

#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace speech {

// Forward declared to avoid adding a compile-time dependency.
// Implementation is provided by //brave/browser/speech:chromium_impl.
bool ShouldPromptForBraveOnDeviceSpeechModel(
    const std::vector<std::string>& languages,
    media::mojom::SpeechRecognitionQuality quality);
void PromptForBraveOnDeviceSpeechModel(
    content::RenderFrameHost& rfh,
    base::OnceCallback<void(base::OnceCallback<void(bool)>)> install,
    base::OnceCallback<void(bool)> callback);

namespace {

// Prompts before `Install` starts a download of Brave's model, and runs
// `Install` again once the user allows it. Takes `callback` only when it
// prompts.
bool MaybePromptBeforeInstall(
    base::WeakPtr<OnDeviceSpeechRecognitionImpl> impl,
    content::RenderFrameHost& rfh,
    const std::vector<std::string>& languages,
    media::mojom::SpeechRecognitionQuality quality,
    OnDeviceSpeechRecognitionImpl::InstallCallback& callback) {
#if BUILDFLAG(ENABLE_LOCAL_AI)
  if (!ShouldPromptForBraveOnDeviceSpeechModel(languages, quality)) {
    return false;
  }
  PromptForBraveOnDeviceSpeechModel(
      rfh,
      base::BindOnce(&OnDeviceSpeechRecognitionImpl::Install, std::move(impl),
                     languages, quality),
      std::move(callback));
  return true;
#else
  return false;
#endif
}

}  // namespace

}  // namespace speech

#include <chrome/browser/speech/on_device_speech_recognition_impl.cc>
