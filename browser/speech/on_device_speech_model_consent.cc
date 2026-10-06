/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/on_device_speech_model_consent.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "brave/browser/speech/on_device_speech_model_permission_request.h"
#include "brave/browser/speech/on_device_speech_recognition_util.h"
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

namespace {

void OnConsentDecided(
    base::OnceCallback<void(base::OnceCallback<void(bool)>)> install,
    base::OnceCallback<void(bool)> callback,
    bool allowed) {
  if (!allowed) {
    std::move(callback).Run(false);
    return;
  }
  std::move(install).Run(std::move(callback));
}

}  // namespace

bool ShouldRequestBraveOnDeviceSpeechModelConsent(
    const std::vector<std::string>& languages,
    media::mojom::SpeechRecognitionQuality quality) {
  // A user already agreed, and the download is under way, failed, or is to be
  // asked for again. This is also what lets `Install` run again once the
  // prompt is allowed.
  if (languages.empty() || local_ai::IsOnDeviceSpeechModelEnabled(
                               g_browser_process->local_state())) {
    return false;
  }

  // Brave's policy answers unavailable for what it does not serve, which
  // upstream then refuses, and available once the model is installed.
  return std::ranges::all_of(languages, [quality](const std::string& language) {
    return GetBraveOnDeviceSpeechAvailability(language, quality) ==
           media::mojom::AvailabilityStatus::kDownloadable;
  });
}

void RequestBraveOnDeviceSpeechModelConsent(
    content::RenderFrameHost& rfh,
    base::OnceCallback<void(base::OnceCallback<void(bool)>)> install,
    base::OnceCallback<void(bool)> callback) {
  Profile* profile = Profile::FromBrowserContext(rfh.GetBrowserContext());
  auto* manager = permissions::PermissionRequestManager::FromWebContents(
      content::WebContents::FromRenderFrameHost(&rfh));

  // The same as the Widevine prompt, a Tor window never asks.
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
                base::BindOnce(&OnConsentDecided, std::move(install),
                               std::move(callback))));
}

}  // namespace speech
