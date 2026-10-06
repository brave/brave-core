/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Implements the hooks that the chromium_src override of
// `chrome/browser/speech/on_device_speech_recognition_impl.cc` forward
// declares.

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
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
    std::string_view language,
    media::mojom::SpeechRecognitionQuality quality);

// Whether `Install` prompts before it downloads: only when it would start one.
// Once the model is enabled it does not prompt, which lets `Install` run again
// after the user allows it.
bool ShouldPromptForBraveOnDeviceSpeechModel(
    const std::vector<std::string>& languages,
    media::mojom::SpeechRecognitionQuality quality) {
  if (languages.empty() || local_ai::ShouldInstallOnDeviceSpeechModel(
                               g_browser_process->local_state())) {
    return false;
  }
  return std::ranges::all_of(languages, [quality](const std::string& language) {
    return GetBraveOnDeviceSpeechAvailability(language, quality) ==
           media::mojom::AvailabilityStatus::kDownloadable;
  });
}

// Runs `install` with `callback` once the user allows it, and otherwise answers
// false. A Tor window and "Don't ask again" are answered without a prompt.
void PromptForBraveOnDeviceSpeechModel(
    content::RenderFrameHost& rfh,
    base::OnceCallback<void(base::OnceCallback<void(bool)>)> install,
    base::OnceCallback<void(bool)> callback) {
  Profile* profile = Profile::FromBrowserContext(rfh.GetBrowserContext());
  auto* manager = permissions::PermissionRequestManager::FromWebContents(
      content::WebContents::FromRenderFrameHost(&rfh));
  if (!manager || profile->IsTor() ||
      !profile->GetPrefs()->GetBoolean(
          local_ai::prefs::kAskEnableOnDeviceSpeechModel)) {
    std::move(callback).Run(false);
    return;
  }

  manager->AddRequest(
      &rfh, std::make_unique<OnDeviceSpeechModelPermissionRequest>(
                *g_browser_process->local_state(), *profile->GetPrefs(),
                rfh.GetLastCommittedOrigin().GetURL(),
                base::BindOnce(
                    [](base::OnceCallback<void(base::OnceCallback<void(bool)>)>
                           install,
                       base::OnceCallback<void(bool)> callback, bool allowed) {
                      if (!allowed) {
                        std::move(callback).Run(false);
                        return;
                      }
                      std::move(install).Run(std::move(callback));
                    },
                    std::move(install), std::move(callback))));
}

}  // namespace speech
