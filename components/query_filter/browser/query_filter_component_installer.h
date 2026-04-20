// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_QUERY_FILTER_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_QUERY_FILTER_COMPONENT_INSTALLER_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "components/component_updater/component_installer.h"
#include "components/update_client/update_client.h"

namespace base {
class FilePath;
class Version;
}  // namespace base

namespace component_updater {
class ComponentUpdateService;

class QueryFilterComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  QueryFilterComponentInstallerPolicy();
  ~QueryFilterComponentInstallerPolicy() override;

  QueryFilterComponentInstallerPolicy(
      const QueryFilterComponentInstallerPolicy&) = delete;
  QueryFilterComponentInstallerPolicy& operator=(
      const QueryFilterComponentInstallerPolicy&) = delete;

  // component_updater::ComponentInstallerPolicy
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
      const base::DictValue& manifest,
      const base::FilePath& install_dir) override;
  void OnCustomUninstall() override;
  bool VerifyInstallation(const base::DictValue& manifest,
                          const base::FilePath& install_dir) const override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& install_dir,
                      base::DictValue manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;
  bool IsBraveComponent() const override;
};

// Registers the Query Filter component with the component updater.
void RegisterQueryFilterComponent(
    component_updater::ComponentUpdateService* cus);
}  // namespace component_updater

#endif  // BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_QUERY_FILTER_COMPONENT_INSTALLER_H_
