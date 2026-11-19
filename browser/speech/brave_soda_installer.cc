/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/brave_soda_installer.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/check.h"
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
  // Safe to return early in both cases. The only caller that waits on a reply,
  // `OnDeviceSpeechRecognitionImpl::Install`, waits for neither: it refuses a
  // language that is not enabled, and it does not wait for an installed one.
  if (!IsLanguageEnabled(language)) {
    return;
  }
  const local_ai::OnDeviceSpeechLocale* locale =
      local_ai::ResolveOnDeviceSpeechLocale(language);
  CHECK(locale);  // Safe: an unknown locale is not enabled and returned above.
  if (local_ai::GetInstalledOnDeviceSpeechModelForLocale(*locale)) {
    return;
  }
  RequestEnabledModel(*locale, /*failed_model=*/std::nullopt);
}

std::vector<std::string> BraveSodaInstaller::GetLiveCaptionEnabledLanguages()
    const {
  // The locales whose model Brave can offer. `SpeechRecognition.install()`
  // refuses the other languages before it calls `InstallLanguage`.
  std::vector<std::string> names;
  for (const local_ai::OnDeviceSpeechLocale& locale :
       local_ai::GetOnDeviceSpeechLocales()) {
    if (local_ai::CanOfferOnDeviceSpeechModel(g_browser_process->local_state(),
                                              locale.model)) {
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

void BraveSodaInstaller::RequestEnabledModel(
    const local_ai::OnDeviceSpeechLocale& locale,
    std::optional<local_ai::OnDeviceSpeechModel> failed_model) {
  const std::optional<local_ai::OnDeviceSpeechModel> enabled =
      local_ai::GetEnabledOnDeviceSpeechModel(g_browser_process->local_state());
  // Fail when the enabled model cannot install `locale`: no model is enabled,
  // the enabled model is `failed_model`, or it does not serve `locale`.
  if (!enabled || enabled == failed_model ||
      !local_ai::OnDeviceSpeechModelServesLocale(*enabled, locale)) {
    NotifyOnSodaInstallError(GetLanguageCode(locale.tag),
                             ErrorCode::kUnspecifiedError);
    return;
  }
  local_ai::MaybeRegisterOnDeviceSpeechModelsComponent(
      *enabled,
      base::BindOnce(&BraveSodaInstaller::OnSpeechModelInstallFinished,
                     weak_factory_.GetWeakPtr(), locale, *enabled));
}

void BraveSodaInstaller::OnSpeechModelInstallFinished(
    const local_ai::OnDeviceSpeechLocale& locale,
    local_ai::OnDeviceSpeechModel model,
    bool success) {
  // `OnSpeechModelDirChanged` reports a model that arrives, also when no
  // request of this class brought it.
  if (success) {
    return;
  }
  // While the request waited, the user can enable another model that serves
  // `locale`, such as the multilingual model in place of the English model. The
  // request then waits for that model.
  RequestEnabledModel(locale, /*failed_model=*/model);
}

void BraveSodaInstaller::OnSpeechModelDirChanged(
    local_ai::OnDeviceSpeechModel model,
    const base::FilePath& model_dir) {
  // Brave ships no SODA binary. A model on disk takes its place, because
  // `install()` also waits for the binary.
  soda_binary_installed_ = local_ai::OnDeviceSpeechModelsState::GetInstance()
                               ->GetInstalledModel()
                               .has_value();

  // Reads all the locales again, not only the locales of `model`. `AddObserver`
  // also reports the model that is not on disk, and that report must not remove
  // the locales of the model on disk. A locale is reported once, when it
  // becomes installed.
  for (const local_ai::OnDeviceSpeechLocale& locale :
       local_ai::GetOnDeviceSpeechLocales()) {
    const LanguageCode language_code = GetLanguageCode(locale.tag);
    if (!local_ai::GetInstalledOnDeviceSpeechModelForLocale(locale)) {
      installed_languages_.erase(language_code);
      continue;
    }
    if (installed_languages_.insert(language_code).second) {
      NotifyOnSodaInstalled(language_code);
    }
  }
}

}  // namespace speech
