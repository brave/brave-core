/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/brave_soda_installer.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/check_is_test.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "brave/components/local_ai/core/on_device_speech_models_component_installer.h"
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
  // Callers like Live Caption could call in without checking this first.
  if (!IsLanguageEnabled(language)) {
    return;
  }

  if (local_ai::OnDeviceSpeechModelsState::GetInstance()->IsModelInstalled()) {
    return;
  }

  if (model_install_requested_for_testing_) {
    model_install_requested_for_testing_.Run();
  }

  local_ai::MaybeRegisterOnDeviceSpeechModelsComponent(
      base::BindOnce(&BraveSodaInstaller::OnSpeechModelInstallFinished,
                     weak_factory_.GetWeakPtr()));
}

std::vector<std::string> BraveSodaInstaller::GetLiveCaptionEnabledLanguages()
    const {
  // Brave's model serves English only. `SpeechRecognition.install()` rejects
  // anything outside this list before it reaches `InstallLanguage`.
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

void BraveSodaInstaller::SetModelInstallRequestedCallbackForTesting(  // IN-TEST
    base::RepeatingClosure callback) {
  CHECK_IS_TEST();
  model_install_requested_for_testing_ = std::move(callback);
}

void BraveSodaInstaller::SetModelInstallFinishedCallbackForTesting(  // IN-TEST
    base::RepeatingCallback<void(bool)> callback) {
  CHECK_IS_TEST();
  model_install_finished_for_testing_ = std::move(callback);
}

void BraveSodaInstaller::OnSpeechModelInstallFinished(bool success) {
  // A model arriving is reported from `OnSpeechModelDirChanged`, whether or
  // not a request of ours brought it.
  if (!success) {
    NotifyOnSodaInstallError(LanguageCode::kEnUs, ErrorCode::kUnspecifiedError);
  }

  if (model_install_finished_for_testing_) {
    model_install_finished_for_testing_.Run(success);
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
