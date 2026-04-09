// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/query_filter_component_installer.h"

#include <stdint.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/query_filter/browser/query_filter_service.h"
#include "brave/components/query_filter/common/features.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"

namespace query_filter {

namespace {

inline constexpr char kQueryFilterComponentName[] = "Query Filter";

// Component id and public key must match the signing key used in
// brave-core-crx-packager for query-filter component.
inline constexpr char kQueryFilterComponentId[] =
    "cemdlagocoimleflkfkjoihojfainiho";

// This is the SHA-256 of the query-filter component's public key.
inline constexpr std::array<uint8_t, 32> kQueryFilterComponentPublicKeySHA256 =
    {0x24, 0xc3, 0xb0, 0x6e, 0x2e, 0x8c, 0xb4, 0x5b, 0xa5, 0xa9, 0xe8,
     0x7e, 0x95, 0x08, 0xd8, 0x7e, 0xf0, 0x7d, 0xdb, 0x29, 0x5f, 0xe0,
     0x2d, 0xeb, 0x55, 0x74, 0x59, 0x40, 0x0f, 0x17, 0x00, 0x51};

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
};

QueryFilterComponentInstallerPolicy::QueryFilterComponentInstallerPolicy() =
    default;

QueryFilterComponentInstallerPolicy::~QueryFilterComponentInstallerPolicy() =
    default;

bool QueryFilterComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return true;
}

bool QueryFilterComponentInstallerPolicy::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
QueryFilterComponentInstallerPolicy::OnCustomInstall(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void QueryFilterComponentInstallerPolicy::OnCustomUninstall() {}

void QueryFilterComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& path,
    base::DictValue manifest) {
  if (QueryFilterService* service = QueryFilterService::GetInstance()) {
    service->OnComponentReady(path);
  }
}

bool QueryFilterComponentInstallerPolicy::VerifyInstallation(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

base::FilePath QueryFilterComponentInstallerPolicy::GetRelativeInstallDir()
    const {
  return base::FilePath::FromUTF8Unsafe(kQueryFilterComponentId);
}

void QueryFilterComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(kQueryFilterComponentPublicKeySHA256.begin(),
               kQueryFilterComponentPublicKeySHA256.end());
}

std::string QueryFilterComponentInstallerPolicy::GetName() const {
  return kQueryFilterComponentName;
}

update_client::InstallerAttributes
QueryFilterComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

bool QueryFilterComponentInstallerPolicy::IsBraveComponent() const {
  return true;
}

void RegisterQueryFilterComponent(
    component_updater::ComponentUpdateService* cus) {
  if (!base::FeatureList::IsEnabled(features::kQueryFilterComponent) || !cus) {
    return;
  }

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<QueryFilterComponentInstallerPolicy>());
  installer->Register(
      cus, base::BindOnce([]() {
        brave_component_updater::BraveOnDemandUpdater::GetInstance()
            ->EnsureInstalled(kQueryFilterComponentId);
      }));
}

}  // namespace query_filter
