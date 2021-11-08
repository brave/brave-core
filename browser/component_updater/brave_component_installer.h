/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_COMPONENT_INSTALLER_H_
#define BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_COMPONENT_INSTALLER_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "components/component_updater/component_installer.h"
#include "components/update_client/update_client.h"

using brave_component_updater::BraveComponent;

namespace brave {

class BraveComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  explicit BraveComponentInstallerPolicy(
      const std::string& name,
      const std::string& base64_public_key,
      BraveComponent::ReadyCallback ready_callback);

  BraveComponentInstallerPolicy(const BraveComponentInstallerPolicy&) = delete;
  BraveComponentInstallerPolicy& operator=(
      const BraveComponentInstallerPolicy&) = delete;

  ~BraveComponentInstallerPolicy() override;

 private:
  // The following methods override ComponentInstallerPolicy
  bool VerifyInstallation(const base::Value& manifest,
                          const base::FilePath& install_dir) const override;
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
      const base::Value& manifest,
      const base::FilePath& install_dir) override;
  void OnCustomUninstall() override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& install_dir,
                      base::Value manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;

  std::string name_;
  std::string base64_public_key_;
  std::string public_key_;
  BraveComponent::ReadyCallback ready_callback_;
};

void RegisterComponent(component_updater::ComponentUpdateService* cus,
                       const std::string& name,
                       const std::string& base64_public_key,
                       base::OnceClosure registered_callback,
                       BraveComponent::ReadyCallback ready_callback);

}  // namespace brave

#endif  // BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_COMPONENT_INSTALLER_H_
