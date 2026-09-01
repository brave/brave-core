/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SPEECH_BRAVE_SODA_INSTALLER_H_
#define BRAVE_BROWSER_SPEECH_BRAVE_SODA_INSTALLER_H_

#include <string>
#include <string_view>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "components/soda/soda_installer.h"

class PrefService;

namespace speech {

// The global `SodaInstaller`, replaced so that `SpeechRecognition.install()`
// installs Brave's own model rather than SODA. Availability is answered
// elsewhere, by `GetBraveOnDeviceSpeechAvailability`, so nothing here is
// consulted for it.
//
// `OnDeviceSpeechRecognitionImpl::Install` reads this class to decide whether
// to act and whether the work is already done, then parks a reply settled by
// `OnSodaInstalled` or `OnSodaInstallError`.
class BraveSodaInstaller
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

  // For testing: allows setting a callback that will be run when
  // `InstallLanguage` requests the model component.
  void SetModelInstallRequestedCallbackForTesting(
      base::RepeatingClosure callback);

  // For testing: allows setting a callback that will be run once that request
  // has been answered, after everything the answer reports.
  void SetModelInstallFinishedCallbackForTesting(
      base::RepeatingCallback<void(bool)> callback);

 protected:
  // SodaInstaller:
  void InstallSoda(PrefService* global_prefs) override {}
  void UninstallSoda(PrefService* global_prefs) override {}

 private:
  // local_ai::OnDeviceSpeechModelsState::Observer:
  void OnSpeechModelDirChanged(const base::FilePath& model_dir) override;

  // Called when the install `InstallLanguage` asked for finishes. `success` is
  // whether a model ended up installed.
  void OnSpeechModelInstallFinished(bool success);

  base::RepeatingClosure model_install_requested_for_testing_;
  base::RepeatingCallback<void(bool)> model_install_finished_for_testing_;

  base::WeakPtrFactory<BraveSodaInstaller> weak_factory_{this};
};

}  // namespace speech

#endif  // BRAVE_BROWSER_SPEECH_BRAVE_SODA_INSTALLER_H_
