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
  // Safe to early return these two cases, the only caller who waits on a reply
  // is from `OnDeviceSpeechRecognitionImpl::Install`, and it never calls in
  // for either case.
  if (!IsLanguageEnabled(language) ||
      local_ai::OnDeviceSpeechModelsState::GetInstance()->IsModelInstalled()) {
    return;
  }

  local_ai::MaybeRegisterOnDeviceSpeechModelsComponent(
      base::BindOnce(&BraveSodaInstaller::OnSpeechModelInstallFinished,
                     weak_factory_.GetWeakPtr()));
}

std::vector<std::string> BraveSodaInstaller::GetLiveCaptionEnabledLanguages()
    const {
  // Brave's model serves English only, and only while Brave may install it.
  // `SpeechRecognition.install()` rejects anything outside this list before it
  // reaches `InstallLanguage`.
  if (!local_ai::IsOnDeviceSpeechRecognitionAllowed(
          g_browser_process->local_state())) {
    return {};
  }
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

void BraveSodaInstaller::OnSpeechModelInstallFinished(bool success) {
  // A model arriving is reported from `OnSpeechModelDirChanged`, whether or
  // not a request of ours brought it.
  if (!success) {
    NotifyOnSodaInstallError(LanguageCode::kEnUs, ErrorCode::kUnspecifiedError);
  }
}

void BraveSodaInstaller::OnSpeechModelDirChanged(
    const base::FilePath& model_dir) {
  if (model_dir.empty()) {
    soda_binary_installed_ = false;
    installed_languages_.erase(LanguageCode::kEnUs);
    return;
  }

  soda_binary_installed_ = true;
  installed_languages_.insert(LanguageCode::kEnUs);
  NotifyOnSodaInstalled(LanguageCode::kEnUs);
}

}  // namespace speech
