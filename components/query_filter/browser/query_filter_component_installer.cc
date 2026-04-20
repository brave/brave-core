// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/query_filter_component_installer.h"

#include <stdint.h>

#include <array>
#include <vector>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/query_filter/browser/query_filter_data.h"
#include "brave/components/query_filter/common/features.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"

namespace {
constexpr char kQueryFilterJsonFile[] = "query-filter.json";
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

void LoadParseRulesFromDisk(const base::Version& version,
                            const base::FilePath& install_dir) {
  if (install_dir.empty()) {
    return;
  }

  const base::FilePath& query_filter_path =
      install_dir.Append(kQueryFilterJsonFile);

  std::string query_filter_json_data;
  if (!base::ReadFileToString(query_filter_path, &query_filter_json_data)) {
    LOG(WARNING) << "Failed reading from " << query_filter_path.value();
    return;
  }

  auto* query_filter_data = query_filter::QueryFilterData::GetInstance();
  if (!query_filter_data) {
    LOG(WARNING) << "QueryFilterData instance is not available";
    return;
  }

  if (!query_filter_data->PopulateDataFromComponent(query_filter_json_data)) {
    LOG(WARNING) << "Failed to populate data from component";
  }

  query_filter_data->UpdateVersion(version);
}

}  // namespace

namespace component_updater {

QueryFilterComponentInstallerPolicy::QueryFilterComponentInstallerPolicy() =
    default;

QueryFilterComponentInstallerPolicy::~QueryFilterComponentInstallerPolicy() =
    default;

bool QueryFilterComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return false;
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
    const base::FilePath& install_dir,
    base::DictValue manifest) {
  VLOG(1) << "Component ready, version " << version.GetString() << " in "
          << install_dir.value();

  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BEST_EFFORT},
      base::BindOnce(&LoadParseRulesFromDisk, version, install_dir));
}

bool QueryFilterComponentInstallerPolicy::VerifyInstallation(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) const {
  return base::PathExists(install_dir.Append(kQueryFilterJsonFile));
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
  if (!base::FeatureList::IsEnabled(
          query_filter::features::kQueryFilterComponent) ||
      !cus) {
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

}  // namespace component_updater
