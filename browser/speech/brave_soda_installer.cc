/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/brave_soda_installer.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "brave/browser/speech/on_device_speech_languages.h"
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
  // Safe to return early in every case. The only caller that waits on a reply,
  // `OnDeviceSpeechRecognitionImpl::Install`, never calls in for any of them.
  std::optional<local_ai::OnDeviceSpeechModel> model =
      GetOnDeviceSpeechModelForLanguage(language);
  if (!IsLanguageEnabled(language) || !model ||
      local_ai::OnDeviceSpeechModelsState::GetInstance()->IsModelInstalled(
          *model)) {
    return;
  }

  local_ai::MaybeRegisterOnDeviceSpeechModelsComponent(
      *model,
      base::BindOnce(&BraveSodaInstaller::OnSpeechModelInstallFinished,
                     weak_factory_.GetWeakPtr(), GetLanguageCode(language)));
}

std::vector<std::string> BraveSodaInstaller::GetLiveCaptionEnabledLanguages()
    const {
  // Only while Brave may install its models. `SpeechRecognition.install()`
  // rejects anything outside this list before it reaches `InstallLanguage`.
  if (!local_ai::CanOfferOnDeviceSpeech(g_browser_process->local_state())) {
    return {};
  }
  // Only the locales a model serves, which leaves out the multilingual model's
  // locales while its feature is disabled. `install()` asks with these
  // canonical tags, never with a page's own tag.
  std::vector<std::string> names;
  for (const BraveOnDeviceSpeechLocale& locale :
       GetBraveOnDeviceSpeechLocales()) {
    if (GetOnDeviceSpeechModelForLanguage(locale.locale)) {
      names.emplace_back(locale.locale);
    }
  }
  return names;
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
    local_ai::OnDeviceSpeechModel model,
    const base::FilePath& model_dir) {
  auto* state = local_ai::OnDeviceSpeechModelsState::GetInstance();
  soda_binary_installed_ =
      state->IsModelInstalled(local_ai::OnDeviceSpeechModel::kEnglish) ||
      state->IsModelInstalled(local_ai::OnDeviceSpeechModel::kMultilingual);

  // Every locale a model serves installs and uninstalls with it.
  for (const BraveOnDeviceSpeechLocale& locale :
       GetBraveOnDeviceSpeechLocales()) {
    if (GetOnDeviceSpeechModelForLanguage(locale.locale) != model) {
      continue;
    }
    const LanguageCode language_code = locale.language_code;
    if (model_dir.empty()) {
      installed_languages_.erase(language_code);
      continue;
    }
    installed_languages_.insert(language_code);
    NotifyOnSodaInstalled(language_code);
  }
}

}  // namespace speech
