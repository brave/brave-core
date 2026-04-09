// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_QUERY_FILTER_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_QUERY_FILTER_COMPONENT_INSTALLER_H_

#include <stdint.h>

#include <array>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/version.h"
#include "base/values.h"
#include "components/component_updater/component_installer.h"
#include "crypto/sha2.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace query_filter {

inline constexpr char kQueryFilterComponentName[] = "Query Filter";

// Component id and public key must match the signing key used in
// brave-core-crx-packager for this component. 
// TODO(brave/brave-browser/issues/10188): Add the actual component id and public key.
inline constexpr char kQueryFilterComponentId[] = "dummy-component-id";
inline constexpr char kQueryFilterComponentBase64PublicKey[] = "dummy-public-key";
inline constexpr uint8_t kQueryFilterComponentPublicKey[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

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
                      const base::FilePath& path,
                      base::DictValue manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;
  bool IsBraveComponent() const override;

 private:
  const std::string component_id_;
  const std::string component_name_;
  std::array<uint8_t, crypto::kSHA256Length> component_hash_;
};

void RegisterQueryFilterComponent(
    component_updater::ComponentUpdateService* cus);

}  // namespace query_filter

#endif  // BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_QUERY_FILTER_COMPONENT_INSTALLER_H_
