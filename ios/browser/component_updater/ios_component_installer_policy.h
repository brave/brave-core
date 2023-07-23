/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_COMPONENT_UPDATER_IOS_COMPONENT_INSTALLER_POLICY_H_
#define BRAVE_IOS_BROWSER_COMPONENT_UPDATER_IOS_COMPONENT_INSTALLER_POLICY_H_

#include <string>
#include <vector>
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"

using brave_component_updater::BraveComponent;

namespace component_updater {
class IOSComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  explicit IOSComponentInstallerPolicy(const std::string& component_public_key,
                                       const std::string& component_id,
                                       const std::string& component_name,
                                       BraveComponent::ReadyCallback callback);
  ~IOSComponentInstallerPolicy() override;

  IOSComponentInstallerPolicy(const IOSComponentInstallerPolicy&) = delete;
  IOSComponentInstallerPolicy& operator=(const IOSComponentInstallerPolicy&) =
      delete;

  // component_updater::ComponentInstallerPolicy
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
      const base::Value::Dict& manifest,
      const base::FilePath& install_dir) override;
  void OnCustomUninstall() override;
  bool VerifyInstallation(const base::Value::Dict& manifest,
                          const base::FilePath& install_dir) const override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& path,
                      base::Value::Dict manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;

 private:
  const std::string component_id_;
  const std::string component_name_;
  const std::string base64_public_key_;
  std::string public_key_;
  BraveComponent::ReadyCallback ready_callback_;
};
}  // namespace component_updater

#endif  // BRAVE_IOS_BROWSER_COMPONENT_UPDATER_IOS_COMPONENT_INSTALLER_POLICY_H_
