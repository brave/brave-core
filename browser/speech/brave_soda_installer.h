/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SPEECH_BRAVE_SODA_INSTALLER_H_
#define BRAVE_BROWSER_SPEECH_BRAVE_SODA_INSTALLER_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "components/soda/soda_installer.h"

class PrefService;

namespace local_ai {
struct OnDeviceSpeechLocale;
}  // namespace local_ai

namespace speech {

// The global `SodaInstaller`, replaced so that `SpeechRecognition.install()`
// installs the Brave model that the user enabled rather than SODA. Availability
// is answered elsewhere, by `GetBraveOnDeviceSpeechAvailability`, so nothing
// here is consulted for it.
//
// `OnDeviceSpeechRecognitionImpl::Install` reads this class to decide whether
// to act and whether the work is already done, then parks a reply settled by
// `OnSodaInstalled` or `OnSodaInstallError`.
class BraveSodaInstaller final
    : public SodaInstaller,
      public local_ai::OnDeviceSpeechModelsState::Observer {
 public:
  BraveSodaInstaller();
  ~BraveSodaInstaller() override;
  BraveSodaInstaller(const BraveSodaInstaller&) = delete;
  BraveSodaInstaller& operator=(const BraveSodaInstaller&) = delete;

  // SodaInstaller:
  void InstallLanguage(std::string_view language,
                       PrefService* global_prefs) override;
  std::vector<std::string> GetLiveCaptionEnabledLanguages() const override;
  std::vector<std::string> GetAvailableLanguages() const override;

  // Empty paths. Brave ships no SODA library and no language pack, and only
  // Live Caption and ChromeOS ask for them.
  base::FilePath GetSodaBinaryPath() const override;
  base::FilePath GetLanguagePath(std::string_view language) const override;

  // No-ops. SODA's own install and uninstall lifecycle acts on component ids,
  // directories and language pack prefs Brave does not have.
  void Init(PrefService* profile_prefs, PrefService* global_prefs) override {}
  void UninstallLanguage(std::string_view language,
                         PrefService* global_prefs) override {}
  void RegisterLanguage(std::string_view language,
                        PrefService* global_prefs) override {}
  void UnregisterLanguage(std::string_view language,
                          PrefService* global_prefs) override {}

 protected:
  // SodaInstaller:
  void InstallSoda(PrefService* global_prefs) override {}
  void UninstallSoda(PrefService* global_prefs) override {}

 private:
  // local_ai::OnDeviceSpeechModelsState::Observer:
  void OnSpeechModelDirChanged(local_ai::OnDeviceSpeechModel model,
                               const base::FilePath& model_dir) override;

  // Asks for the enabled model to install `locale`. Reports an install error
  // for `locale` instead when the enabled model does not serve `locale`, or is
  // `failed_model`, the model that the last request for `locale` did not
  // install.
  void RequestEnabledModel(
      const local_ai::OnDeviceSpeechLocale& locale,
      std::optional<local_ai::OnDeviceSpeechModel> failed_model);

  // Called when the request of `RequestEnabledModel` for `model` and `locale`
  // ends. `success` is whether `model` is installed.
  void OnSpeechModelInstallFinished(
      const local_ai::OnDeviceSpeechLocale& locale,
      local_ai::OnDeviceSpeechModel model,
      bool success);

  base::WeakPtrFactory<BraveSodaInstaller> weak_factory_{this};
};

}  // namespace speech

#endif  // BRAVE_BROWSER_SPEECH_BRAVE_SODA_INSTALLER_H_
