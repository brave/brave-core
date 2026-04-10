// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/query_filter_component_installer.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/feature_list.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/query_filter/common/features.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "crypto/sha2.h"

namespace query_filter {

QueryFilterComponentInstallerPolicy::QueryFilterComponentInstallerPolicy()
    : component_id_(kQueryFilterComponentId),
      component_name_(kQueryFilterComponentName) {
  component_hash_ = crypto::SHA256Hash(kQueryFilterComponentPublicKey);
}

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
  // TODO(https://github.com/brave/brave-browser/issues/54395): Pass the path to query filter service when it is implemented.
}

bool QueryFilterComponentInstallerPolicy::VerifyInstallation(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

base::FilePath QueryFilterComponentInstallerPolicy::GetRelativeInstallDir()
    const {
  return base::FilePath::FromUTF8Unsafe(component_id_);
}

void QueryFilterComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(component_hash_.begin(), component_hash_.end());
}

std::string QueryFilterComponentInstallerPolicy::GetName() const {
  return component_name_;
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
        brave_component_updater::BraveOnDemandUpdater::GetInstance()->EnsureInstalled(
            kQueryFilterComponentId);
      }));
}

}  // namespace query_filter
