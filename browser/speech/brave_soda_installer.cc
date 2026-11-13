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
#include "brave/components/local_ai/core/on_device_speech_locales.h"
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
  const local_ai::OnDeviceSpeechLocale* locale =
      local_ai::ResolveOnDeviceSpeechLocale(language);
  if (!IsLanguageEnabled(language) || !locale ||
      local_ai::OnDeviceSpeechModelsState::GetInstance()->IsModelInstalled(
          locale->model)) {
    return;
  }

  local_ai::MaybeRegisterOnDeviceSpeechModelsComponent(
      locale->model,
      base::BindOnce(&BraveSodaInstaller::OnSpeechModelInstallFinished,
                     weak_factory_.GetWeakPtr(), GetLanguageCode(locale->tag)));
}

std::vector<std::string> BraveSodaInstaller::GetLiveCaptionEnabledLanguages()
    const {
  // Only the locales whose model Brave can offer. That is no locale while the
  // feature or the master switch is off, and only en-US while
  // `kBraveOnDeviceSpeechMultilingual` is off. `SpeechRecognition.install()`
  // rejects anything outside this list before it reaches `InstallLanguage`.
  // `install()` asks with these locales' tags, never with a page's own tag.
  const PrefService* local_state = g_browser_process->local_state();
  std::vector<std::string> names;
  for (const local_ai::OnDeviceSpeechLocale& locale :
       local_ai::GetOnDeviceSpeechLocales()) {
    if (local_ai::CanOfferOnDeviceSpeechModel(local_state, locale.model)) {
      names.emplace_back(locale.tag);
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
  for (const local_ai::OnDeviceSpeechLocale& locale :
       local_ai::GetOnDeviceSpeechLocales()) {
    if (locale.model != model) {
      continue;
    }
    const LanguageCode language_code = GetLanguageCode(locale.tag);
    if (model_dir.empty()) {
      installed_languages_.erase(language_code);
      continue;
    }
    installed_languages_.insert(language_code);
    NotifyOnSodaInstalled(language_code);
  }
}

}  // namespace speech
