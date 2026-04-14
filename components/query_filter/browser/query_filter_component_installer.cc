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
#include "base/functional/callback.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/query_filter/common/features.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "crypto/sha2.h"

namespace query_filter {

namespace {

inline constexpr char kQueryFilterComponentName[] = "Query Filter";

// Component id and public key must match the signing key used in
// brave-core-crx-packager for this component.
inline constexpr char kQueryFilterComponentId[] =
    "cemdlagocoimleflkfkjoihojfainiho";

inline constexpr uint8_t kQueryFilterComponentPublicKey[] = {
    0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86,
    0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0f, 0x00,
    0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01, 0x00, 0xc3, 0x48, 0x3a,
    0xc0, 0x64, 0xc2, 0x96, 0x17, 0x4d, 0xbb, 0x91, 0x27, 0x60, 0xa4, 0xca,
    0xd7, 0x4b, 0x00, 0x64, 0x7e, 0x25, 0xf5, 0x02, 0x04, 0x39, 0x7d, 0xc0,
    0xf3, 0xae, 0xa0, 0x55, 0xf6, 0xb7, 0x84, 0x5a, 0xd7, 0x91, 0x27, 0x55,
    0x12, 0xda, 0x2a, 0x5b, 0xe8, 0xd8, 0xfe, 0xf6, 0x50, 0x0e, 0xe6, 0x5b,
    0xcc, 0xc2, 0xb4, 0xdc, 0x09, 0x40, 0xcb, 0xb5, 0x8f, 0xa2, 0xb8, 0xb2,
    0x2e, 0x77, 0x52, 0x09, 0xa2, 0xd0, 0xf7, 0x1a, 0x5e, 0xa4, 0xf0, 0x33,
    0x63, 0x79, 0xa6, 0x01, 0x1f, 0x0a, 0x9e, 0x6e, 0x0b, 0x7e, 0x6f, 0xdc,
    0x83, 0x2a, 0x08, 0xdd, 0x7d, 0x35, 0x1c, 0x54, 0xce, 0x58, 0xa7, 0xd9,
    0x93, 0xfc, 0x48, 0x42, 0x13, 0xb2, 0xbb, 0xb2, 0x06, 0x49, 0x69, 0xbb,
    0x2e, 0x97, 0x2d, 0x11, 0x64, 0x5f, 0x46, 0x8b, 0xaf, 0x5d, 0x2e, 0xc9,
    0x63, 0x81, 0x9f, 0x20, 0x7f, 0x4c, 0xe9, 0x1b, 0x0c, 0x2b, 0xff, 0x01,
    0xbc, 0x34, 0x33, 0xa0, 0x43, 0xf5, 0x63, 0xd3, 0x9a, 0x49, 0xf3, 0x47,
    0xd1, 0xcd, 0x3f, 0xc9, 0x9e, 0x00, 0xd0, 0xb6, 0x5e, 0x8b, 0xa2, 0x37,
    0xc3, 0x1d, 0x34, 0x0b, 0x29, 0x06, 0x5e, 0x58, 0x10, 0x79, 0x5c, 0x31,
    0x6d, 0x09, 0x12, 0xd1, 0x42, 0x94, 0xf2, 0xd2, 0x86, 0x52, 0x14, 0x1d,
    0x83, 0xaf, 0x04, 0x3a, 0x9d, 0x96, 0xe7, 0x29, 0xa6, 0x30, 0x5c, 0xf5,
    0x19, 0xfd, 0xe2, 0x9a, 0x80, 0xe2, 0x8a, 0x77, 0xea, 0x57, 0x19, 0x18,
    0x17, 0x5c, 0xfb, 0xf8, 0x1b, 0xa5, 0xbb, 0x42, 0x15, 0x28, 0x5a, 0xe6,
    0xa1, 0x6f, 0xb5, 0x0b, 0x26, 0xb1, 0x76, 0x38, 0x23, 0x88, 0x74, 0x6f,
    0xc3, 0xd9, 0x9f, 0x22, 0x83, 0xc9, 0x83, 0x0f, 0x9a, 0x46, 0x0d, 0x85,
    0x80, 0x0b, 0x12, 0x69, 0x71, 0x75, 0x13, 0x5b, 0x02, 0x22, 0xa6, 0x52,
    0x1d, 0x02, 0x03, 0x01, 0x00, 0x01};

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
  std::array<uint8_t, crypto::kSHA256Length> component_hash_;
};

QueryFilterComponentInstallerPolicy::QueryFilterComponentInstallerPolicy() {
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
  // TODO(https://github.com/brave/brave-browser/issues/54395): Pass the path to
  // query filter service when it is implemented.
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
  hash->assign(component_hash_.begin(), component_hash_.end());
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

}  // namespace

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
