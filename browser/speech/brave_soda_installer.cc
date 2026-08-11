/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/brave_soda_installer.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "brave/components/local_ai/core/on_device_speech_models_component_installer.h"
#include "chrome/browser/browser_process.h"
#include "components/component_updater/component_updater_service.h"
#include "components/soda/constants.h"
#include "components/update_client/crx_update_item.h"
#include "components/update_client/update_client_errors.h"

namespace speech {

BraveSodaInstaller::BraveSodaInstaller() {
  local_ai::OnDeviceSpeechModelsState::GetInstance()->AddObserver(this);
}

BraveSodaInstaller::~BraveSodaInstaller() {
  local_ai::OnDeviceSpeechModelsState::GetInstance()->RemoveObserver(this);
}

void BraveSodaInstaller::Init(PrefService* profile_prefs,
                              PrefService* global_prefs) {}

void BraveSodaInstaller::InstallSoda(PrefService* global_prefs) {}

void BraveSodaInstaller::UninstallSoda(PrefService* global_prefs) {}

void BraveSodaInstaller::UninstallLanguage(std::string_view language,
                                           PrefService* global_prefs) {}

void BraveSodaInstaller::RegisterLanguage(std::string_view language,
                                          PrefService* global_prefs) {}

void BraveSodaInstaller::UnregisterLanguage(std::string_view language,
                                            PrefService* global_prefs) {}

void BraveSodaInstaller::InstallLanguage(std::string_view language,
                                         PrefService* global_prefs) {
  auto* state = local_ai::OnDeviceSpeechModelsState::GetInstance();
  if (state->IsModelInstalled()) {
    return;
  }

  auto* component_updater = g_browser_process->component_updater();
  if (component_updater &&
      !component_updater_observation_.IsObservingSource(component_updater)) {
    component_updater_observation_.Observe(component_updater);
  }

  local_ai::MaybeRegisterOnDeviceSpeechModelsComponent(
      component_updater, global_prefs,
      base::BindOnce(&BraveSodaInstaller::OnComponentUpdaterResult,
                     weak_factory_.GetWeakPtr()));
}

void BraveSodaInstaller::OnEvent(const update_client::CrxUpdateItem& item) {
  if (item.id != local_ai::kOnDeviceSpeechModelsComponentId) {
    return;
  }

  // Only handle failure case here, success case is handled via
  // `ComponentReady`. In-progress events are ignored as we don't report
  // downloading status.
  if (item.state != update_client::ComponentState::kUpdateError) {
    return;
  }

  NotifyOnSodaInstallError(LanguageCode::kEnUs, ErrorCode::kUnspecifiedError);
}

void BraveSodaInstaller::OnComponentUpdaterResult(update_client::Error error) {
  // Success needs no action, because `ComponentReady` publishes the install dir
  // before this runs and that is what reports the model as installed.
  auto* state = local_ai::OnDeviceSpeechModelsState::GetInstance();
  if (error == update_client::Error::NONE && state->IsModelInstalled()) {
    return;
  }

  // Not a failure. A download for the component is already running, so this
  // request was refused as a duplicate. That download reports its own outcome,
  // success through `ComponentReady` and failure through `OnEvent`.
  if (error == update_client::Error::UPDATE_IN_PROGRESS) {
    return;
  }

  NotifyOnSodaInstallError(LanguageCode::kEnUs, ErrorCode::kUnspecifiedError);
}

std::vector<std::string> BraveSodaInstaller::GetLiveCaptionEnabledLanguages()
    const {
  // Brave's model serves English only. `install()` rejects anything outside
  // this list before it reaches `InstallLanguage`, which is what keeps a
  // request for another language from installing an English model.
  return {GetLanguageName(LanguageCode::kEnUs)};
}

std::vector<std::string> BraveSodaInstaller::GetAvailableLanguages() const {
  return GetLiveCaptionEnabledLanguages();
}

base::FilePath BraveSodaInstaller::GetSodaBinaryPath() const {
  return base::FilePath();
}

base::FilePath BraveSodaInstaller::GetLanguagePath(
    std::string_view language) const {
  return base::FilePath();
}

void BraveSodaInstaller::OnSpeechModelInstalledChanged(bool installed) {
  if (!installed) {
    soda_binary_installed_ = false;
    installed_languages_.erase(LanguageCode::kEnUs);
    return;
  }

  // Brave's model covers what upstream splits into a binary and a language
  // pack, so both flip together.
  soda_binary_installed_ = true;
  installed_languages_.insert(LanguageCode::kEnUs);
  NotifyOnSodaInstalled(LanguageCode::kEnUs);
}

}  // namespace speech
