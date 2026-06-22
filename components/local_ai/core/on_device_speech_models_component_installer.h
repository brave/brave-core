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
#include "base/values.h"
#include "components/component_updater/component_installer.h"
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

// Registers the on-device speech models component when the feature is enabled,
// otherwise removes any previously installed copy. No-op if `cus` is null.
void RegisterOnDeviceSpeechModelsComponent(
    component_updater::ComponentUpdateService* cus);

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_MODELS_COMPONENT_INSTALLER_H_
