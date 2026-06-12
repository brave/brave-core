/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/on_device_speech_recognition_util.h"

#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_models_component_installer.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/update_client/update_client_errors.h"
#include "ui/base/l10n/l10n_util.h"

namespace speech {

namespace {

// Terminal outcome of the component updater request. Success needs no action,
// because `ComponentReady` populates the install dir before this runs, and that
// is what settles the waiters with `true`.
void OnModelInstallResult(update_client::Error error) {
  auto* state = local_ai::OnDeviceSpeechModelsState::GetInstance();
  if (error == update_client::Error::NONE && !state->GetInstallDir().empty()) {
    return;
  }

  // Another request is already downloading the model, so that request's outcome
  // is what settles this one.
  if (error == update_client::Error::UPDATE_IN_PROGRESS) {
    return;
  }

  state->OnInstallFailed();
}

}  // namespace

media::mojom::AvailabilityStatus GetBraveOnDeviceSpeechAvailability(
    content::BrowserContext* context,
    std::string_view language,
    media::mojom::SpeechRecognitionQuality quality) {
  // Brave ships no upstream on-device backend, so a quality its own model does
  // not serve, or the feature being off, leaves nothing to fall back to. Asking
  // the same question the install path asks is what keeps the two from
  // disagreeing about which qualities exist.
  if (!UsesBraveOnDeviceSpeech(quality)) {
    return media::mojom::AvailabilityStatus::kUnavailable;
  }

  // English only for now.
  if (l10n_util::GetLanguage(language) != "en") {
    return media::mojom::AvailabilityStatus::kUnavailable;
  }

  // The model is delivered by the component updater. Report `kDownloadable`
  // until it lands so the page can drive the install flow.
  if (local_ai::OnDeviceSpeechModelsState::GetInstance()
          ->GetInstallDir()
          .empty()) {
    return media::mojom::AvailabilityStatus::kDownloadable;
  }

  return media::mojom::AvailabilityStatus::kAvailable;
}

bool UsesBraveOnDeviceSpeech(media::mojom::SpeechRecognitionQuality quality) {
  return base::FeatureList::IsEnabled(
             local_ai::kBraveOnDeviceSpeechRecognition) &&
         (quality == media::mojom::SpeechRecognitionQuality::kCommand ||
          quality == media::mojom::SpeechRecognitionQuality::kDictation);
}

void InstallBraveOnDeviceSpeechModel(
    PrefService* local_state,
    component_updater::ComponentUpdateService* component_updater,
    base::OnceCallback<void(bool)> callback) {
  auto* state = local_ai::OnDeviceSpeechModelsState::GetInstance();

  // Queue ahead of everything that can fail below, so that every exit settles
  // the reply. This runs `callback` immediately when the model is already on
  // disk, which is the common case after the first session.
  state->NotifyWhenSettled(std::move(callback));
  if (!state->GetInstallDir().empty()) {
    return;
  }

  if (!local_state->GetBoolean(
          local_ai::prefs::kBraveOnDeviceSpeechModelEnabled)) {
    // First install request. Record the activation and register the component.
    // Registration's completion callback is what asks for the download.
    local_state->SetBoolean(local_ai::prefs::kBraveOnDeviceSpeechModelEnabled,
                            true);
    local_ai::RegisterOnDeviceSpeechModelsComponent(
        component_updater, /*activated=*/true,
        base::BindOnce(&OnModelInstallResult));
    return;
  }

  // Registered in this or an earlier session, so ask for the download directly.
  // This is also the retry path after a download failed.
  local_ai::EnsureOnDeviceSpeechModelInstalled(
      base::BindOnce(&OnModelInstallResult));
}

}  // namespace speech
