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
#include "brave/components/local_ai/core/utils.h"
#include "chrome/browser/browser_process.h"
#include "components/soda/constants.h"

namespace speech {

BraveSodaInstaller::BraveSodaInstaller() {
  local_ai::OnDeviceSpeechModelsState::GetInstance()->AddObserver(this);
}

BraveSodaInstaller::~BraveSodaInstaller() {
  local_ai::OnDeviceSpeechModelsState::GetInstance()->RemoveObserver(this);
}

void BraveSodaInstaller::InstallLanguage(std::string_view language,
                                         PrefService* global_prefs) {
  // Safe to return early in both cases. The only caller that waits on a reply,
  // `OnDeviceSpeechRecognitionImpl::Install`, never calls in for either.
  if (!IsLanguageEnabled(language) ||
      local_ai::OnDeviceSpeechModelsState::GetInstance()->IsModelInstalled()) {
    return;
  }

  local_ai::MaybeRegisterOnDeviceSpeechModelsComponent(
      base::BindOnce(&BraveSodaInstaller::OnSpeechModelInstallFinished,
                     weak_factory_.GetWeakPtr(), GetLanguageCode(language)));
}

std::vector<std::string> BraveSodaInstaller::GetLiveCaptionEnabledLanguages()
    const {
  // Only while Brave may install its model. `SpeechRecognition.install()`
  // rejects anything outside this list before it reaches `InstallLanguage`.
  if (!local_ai::IsOnDeviceSpeechRecognitionAllowed(
          g_browser_process->local_state())) {
    return {};
  }
  return GetBraveOnDeviceSpeechSodaLanguageNames();
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

void BraveSodaInstaller::OnSpeechModelInstallFinished(
    LanguageCode language_code,
    bool success) {
  // A model arriving is reported from `OnSpeechModelDirChanged`, whether or
  // not a request of ours brought it.
  if (!success) {
    NotifyOnSodaInstallError(language_code, ErrorCode::kUnspecifiedError);
  }
}

void BraveSodaInstaller::OnSpeechModelDirChanged(
    const base::FilePath& model_dir) {
  soda_binary_installed_ = !model_dir.empty();

  // One model serves every language, so they install and uninstall together.
  for (const std::string& language :
       GetBraveOnDeviceSpeechSodaLanguageNames()) {
    const LanguageCode language_code = GetLanguageCode(language);
    if (model_dir.empty()) {
      installed_languages_.erase(language_code);
      continue;
    }
    installed_languages_.insert(language_code);
    NotifyOnSodaInstalled(language_code);
  }
}

}  // namespace speech
