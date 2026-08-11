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
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "components/component_updater/component_updater_service.h"
#include "components/soda/soda_installer.h"
#include "components/update_client/update_client_errors.h"

class PrefService;

namespace update_client {
struct CrxUpdateItem;
}  // namespace update_client

namespace speech {

// The global `SodaInstaller`, replaced so that installing on-device speech
// recognition installs Brave's own model rather than SODA.
//
// `OnDeviceSpeechRecognitionImpl::Install()` is left alone: it parks the reply,
// calls `InstallSoda` then `InstallLanguage`, and settles it from this class's
// `OnSodaInstalled` / `OnSodaInstallError`, so only where the bytes come from
// changes.
//
// Availability is not answered here. It stays with
// `GetBraveOnDeviceSpeechAvailability`.
//
// The base class drives SODA's own install and uninstall lifecycle on SODA's
// component ids, directories and language pack prefs, none of which Brave has,
// so those overrides no-op and the whole install runs from `InstallLanguage`.
class BraveSodaInstaller
    : public SodaInstaller,
      public component_updater::ServiceObserver,
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
  base::FilePath GetSodaBinaryPath() const override;
  base::FilePath GetLanguagePath(std::string_view language) const override;
  void Init(PrefService* profile_prefs, PrefService* global_prefs) override;
  void UninstallLanguage(std::string_view language,
                         PrefService* global_prefs) override;
  void RegisterLanguage(std::string_view language,
                        PrefService* global_prefs) override;
  void UnregisterLanguage(std::string_view language,
                          PrefService* global_prefs) override;

 protected:
  // SodaInstaller:
  void InstallSoda(PrefService* global_prefs) override;
  void UninstallSoda(PrefService* global_prefs) override;

 private:
  // local_ai::OnDeviceSpeechModelsState::Observer:
  void OnSpeechModelInstalledChanged(bool installed) override;

  // component_updater::ServiceObserver:
  // Reports a failed download, including one this installer did not start.
  // `OnComponentUpdaterResult` answers only for its own request, so a download
  // already in flight is heard about here alone.
  void OnEvent(const update_client::CrxUpdateItem& item) override;

  // Outcome of the request `InstallLanguage` made. Only a failure is reported,
  // because a success arrives as the install dir being published.
  void OnComponentUpdaterResult(update_client::Error error);

  base::ScopedObservation<component_updater::ComponentUpdateService,
                          component_updater::ComponentUpdateService::Observer>
      component_updater_observation_{this};

  base::WeakPtrFactory<BraveSodaInstaller> weak_factory_{this};
};

}  // namespace speech

#endif  // BRAVE_BROWSER_SPEECH_BRAVE_SODA_INSTALLER_H_
