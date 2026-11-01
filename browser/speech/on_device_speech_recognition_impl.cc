/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Implements the hooks that the chromium_src override of
// `chrome/browser/speech/on_device_speech_recognition_impl.cc` forward
// declares.

#include "chrome/browser/speech/on_device_speech_recognition_impl.h"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/speech/on_device_speech_model_permission_request.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "brave/components/local_ai/core/utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/permissions/permission_request_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"

namespace speech {

// Implemented in on_device_speech_recognition_util.cc.
media::mojom::AvailabilityStatus GetBraveOnDeviceSpeechAvailability(
    content::BrowserContext* context,
    std::string_view language,
    media::mojom::SpeechRecognitionQuality quality);

namespace {

// Whether `Install` prompts before it downloads: only when it would start one.
// Once the model is enabled it does not prompt, which lets `Install` run again
// after the user allows it.
bool ShouldPromptForBraveOnDeviceSpeechModel(
    content::BrowserContext* context,
    base::span<const std::string> languages,
    media::mojom::SpeechRecognitionQuality quality) {
  if (languages.empty() || local_ai::ShouldInstallOnDeviceSpeechModel(
                               g_browser_process->local_state())) {
    return false;
  }
  return std::ranges::all_of(
      languages, [context, quality](const std::string& language) {
        return GetBraveOnDeviceSpeechAvailability(context, language, quality) ==
               media::mojom::AvailabilityStatus::kDownloadable;
      });
}

// Answers the prompt that `PromptForBraveOnDeviceSpeechModel` shows. Runs
// `install` with `callback` when the user allows it, and otherwise answers
// false.
void OnPromptAnswered(
    base::OnceCallback<void(base::OnceCallback<void(bool)>)> install,
    base::OnceCallback<void(bool)> callback,
    bool allowed) {
  if (!allowed) {
    std::move(callback).Run(false);
    return;
  }
  std::move(install).Run(std::move(callback));
}

// Runs `install` with `callback` once the user allows it, and otherwise answers
// false. "Don't ask again" is answered without a prompt.
void PromptForBraveOnDeviceSpeechModel(
    content::RenderFrameHost& rfh,
    base::OnceCallback<void(base::OnceCallback<void(bool)>)> install,
    base::OnceCallback<void(bool)> callback) {
  Profile* profile = Profile::FromBrowserContext(rfh.GetBrowserContext());
  auto* manager = permissions::PermissionRequestManager::FromWebContents(
      content::WebContents::FromRenderFrameHost(&rfh));
  if (!manager || !profile->GetPrefs()->GetBoolean(
                      local_ai::prefs::kAskEnableOnDeviceSpeechModel)) {
    std::move(callback).Run(false);
    return;
  }

  manager->AddRequest(
      &rfh, std::make_unique<OnDeviceSpeechModelPermissionRequest>(
                *g_browser_process->local_state(), *profile->GetPrefs(),
                rfh.GetLastCommittedOrigin().GetURL(),
                base::BindOnce(&OnPromptAnswered, std::move(install),
                               std::move(callback))));
}

}  // namespace

// Sends `kDictation` down upstream's SODA branch, as `kCommand`, in both
// `Available` and `Install`. That branch is where `BraveSodaInstaller` installs
// Brave's model, and both functions must take it, so that `available()` reads
// the language names that `install()` records for the origin. Otherwise
// upstream refuses `kDictation` unless `kOnDeviceWebSpeechSmallExpertModel` is
// enabled, and `install()` asks the optimization guide for a model Brave does
// not ship.
//
// Nothing Brave decides depends on it. Brave's policy answers `kCommand` and
// `kDictation` alike, and `BraveSodaInstaller` installs one model for both.
media::mojom::SpeechRecognitionQuality ToBraveOnDeviceSpeechQuality(
    media::mojom::SpeechRecognitionQuality quality) {
  return quality == media::mojom::SpeechRecognitionQuality::kDictation
             ? media::mojom::SpeechRecognitionQuality::kCommand
             : quality;
}

// Cancels `Install` in a Private, Tor or Guest profile. Otherwise prompts
// before `Install` starts a download of Brave's model, and runs `Install` again
// once the user allows it. Returns true when it cancels or prompts, and only
// then takes `callback`.
// `frame_can_use_on_device_speech` is what the private
// `CanRenderFrameHostUseOnDeviceSpeechRecognition` says, so there is no prompt
// for a frame that upstream refuses.
//
// `quality` comes before `ToBraveOnDeviceSpeechQuality`. That is fine, because
// this asks Brave's policy, not upstream's branches, and `Install` maps the
// quality again when it runs again.
bool MaybePromptOrCancelBraveOnDeviceSpeechInstall(
    bool frame_can_use_on_device_speech,
    base::WeakPtr<OnDeviceSpeechRecognitionImpl> impl,
    const std::vector<std::string>& languages,
    media::mojom::SpeechRecognitionQuality quality,
    OnDeviceSpeechRecognitionImpl::InstallCallback& callback) {
  content::RenderFrameHost& rfh = impl->render_frame_host();
  content::BrowserContext* context = rfh.GetBrowserContext();
  // Cancels `Install` in Private, Tor and Guest profiles.
  if (!Profile::FromBrowserContext(context)->IsRegularProfile()) {
    std::move(callback).Run(false);
    return true;
  }
  if (!frame_can_use_on_device_speech ||
      !ShouldPromptForBraveOnDeviceSpeechModel(context, languages, quality)) {
    return false;
  }
  PromptForBraveOnDeviceSpeechModel(
      rfh,
      base::BindOnce(&OnDeviceSpeechRecognitionImpl::Install, std::move(impl),
                     languages, quality),
      std::move(callback));
  return true;
}

}  // namespace speech
