/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_MODELS_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_MODELS_COMPONENT_INSTALLER_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/values.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "components/update_client/update_client.h"

namespace base {
class Version;
}  // namespace base

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace local_ai {

// Component Updater policy for Brave's on-device speech recognition model.
// Exposed for testing - follows upstream Chromium pattern.
class OnDeviceSpeechModelsComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  OnDeviceSpeechModelsComponentInstallerPolicy();
  ~OnDeviceSpeechModelsComponentInstallerPolicy() override;

  OnDeviceSpeechModelsComponentInstallerPolicy(
      const OnDeviceSpeechModelsComponentInstallerPolicy&) = delete;
  OnDeviceSpeechModelsComponentInstallerPolicy& operator=(
      const OnDeviceSpeechModelsComponentInstallerPolicy&) = delete;

  // component_updater::ComponentInstallerPolicy:
  bool VerifyInstallation(const base::DictValue& manifest,
                          const base::FilePath& install_dir) const override;
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
      const base::DictValue& manifest,
      const base::FilePath& install_dir) override;
  void OnCustomUninstall() override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& install_dir,
                      base::DictValue manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;
  bool IsBraveComponent() const override;
};

// Registers the on-device speech models component when the feature is enabled
// AND on-device speech has been activated (`activated`, i.e. the user has
// invoked `SpeechRecognition.install()` at least once), otherwise removes any
// previously installed copy. Registration alone makes the component updater
// fetch the model on its own cycle, so gating on activation here is what keeps
// the download from happening for users who never use on-device speech.
//
// `callback` reports the download that registration triggers. It also runs,
// with `Error::INVALID_ARGUMENT`, on every path that registers nothing at all
// (feature off, not activated, or `cus` null), so a caller waiting on the
// install is never left waiting on a download that will not start.
void RegisterOnDeviceSpeechModelsComponent(
    component_updater::ComponentUpdateService* cus,
    bool activated,
    component_updater::Callback callback = base::DoNothing());

// Requests an on-demand download/install of the on-device speech model,
// triggered from `SpeechRecognition.install()`. Requires the component to have
// been registered first, and reports `Error::INVALID_ARGUMENT` through
// `callback` when it was not. `callback` runs exactly once, for every outcome,
// which is how terminal download failures reach
// `OnDeviceSpeechModelsState::OnInstallFailed`.
void EnsureOnDeviceSpeechModelInstalled(
    component_updater::Callback callback = base::DoNothing());

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_MODELS_COMPONENT_INSTALLER_H_
