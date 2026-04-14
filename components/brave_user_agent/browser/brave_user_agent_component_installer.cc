// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_user_agent/browser/brave_user_agent_component_installer.h"

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <stdint.h>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/brave_user_agent/browser/brave_user_agent_exceptions.h"
#include "brave/components/brave_user_agent/common/features.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "crypto/sha2.h"

using brave_component_updater::BraveOnDemandUpdater;

namespace brave_user_agent {

namespace {

inline constexpr char kBraveUserAgentExceptionsComponentName[] =
    "Brave User Agent Exceptions";
inline constexpr char kBraveUserAgentExceptionsComponentId[] =
    "nlpaeekllejnmhoonlpcefpfnpbajbpe";
inline constexpr uint8_t kBraveUserAgentExceptionsComponentPublicKey[] = {
    0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86,
    0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0f, 0x00,
    0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01, 0x00, 0x9d, 0x93, 0x39,
    0xce, 0x5a, 0x2c, 0x16, 0xa2, 0x7e, 0x48, 0x9f, 0x39, 0x2b, 0x5f, 0xb9,
    0xc9, 0x9c, 0xee, 0xbd, 0x39, 0x31, 0x28, 0x34, 0x50, 0xde, 0x8f, 0x8c,
    0x05, 0x10, 0xa6, 0x46, 0x92, 0x2b, 0x4f, 0x80, 0xe1, 0x62, 0xe8, 0x58,
    0xf3, 0xd5, 0xfe, 0xd0, 0x3b, 0x20, 0xb1, 0xb5, 0x63, 0x30, 0xea, 0xfc,
    0x57, 0x71, 0x9e, 0x03, 0x36, 0x59, 0x27, 0x5d, 0x49, 0xbd, 0x05, 0xa3,
    0x93, 0xb2, 0x25, 0x30, 0x06, 0x91, 0x8f, 0x1e, 0x07, 0xad, 0x60, 0xc3,
    0xc7, 0xb0, 0x25, 0x38, 0xe6, 0xd7, 0x36, 0x5a, 0x5c, 0xa3, 0xa1, 0x36,
    0xbe, 0xea, 0x95, 0x20, 0xc0, 0xec, 0xc3, 0x54, 0xdb, 0x85, 0xa0, 0x05,
    0x64, 0xda, 0xac, 0xe4, 0xcd, 0x2f, 0x51, 0x48, 0x31, 0xbf, 0xdf, 0x48,
    0x6c, 0x60, 0xf6, 0x0d, 0xbe, 0xa5, 0xd9, 0xf4, 0x38, 0x72, 0xbc, 0xf4,
    0xd4, 0xec, 0x1c, 0x14, 0xad, 0xa0, 0x3b, 0xd1, 0x0f, 0xa8, 0x58, 0x11,
    0x4f, 0x7d, 0xdc, 0xc2, 0x19, 0x6e, 0xd5, 0x49, 0xdb, 0xca, 0x67, 0x95,
    0xe2, 0x07, 0xd1, 0xe0, 0x0f, 0xa8, 0xa9, 0xef, 0x2d, 0x88, 0x22, 0x41,
    0x64, 0xdd, 0x67, 0xd2, 0xeb, 0x03, 0xb8, 0xe5, 0x76, 0xff, 0x13, 0xdc,
    0xd9, 0x00, 0x09, 0xc4, 0x23, 0xe4, 0xe0, 0x03, 0x15, 0x67, 0x76, 0xe2,
    0xd0, 0xdd, 0x06, 0x8d, 0x5c, 0x32, 0xad, 0xd3, 0xc7, 0x8a, 0xe3, 0xed,
    0xb6, 0x78, 0x85, 0xf3, 0x77, 0xf1, 0xa6, 0x31, 0x90, 0x1b, 0x66, 0x99,
    0x14, 0x37, 0xd1, 0xbf, 0xc2, 0x24, 0x5f, 0x45, 0x49, 0xf6, 0x96, 0x17,
    0x90, 0x87, 0x19, 0x69, 0xcd, 0x46, 0xa6, 0x8f, 0x2f, 0xb2, 0x71, 0xbb,
    0x7c, 0x74, 0xa6, 0xe8, 0xd3, 0x7f, 0x83, 0xb2, 0xdd, 0x26, 0xe1, 0xbb,
    0x34, 0xc4, 0x54, 0x14, 0xce, 0x9d, 0xce, 0x02, 0xb5, 0x1f, 0x05, 0xb2,
    0x13, 0x02, 0x03, 0x01, 0x00, 0x01,
};

class BraveUserAgentComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  BraveUserAgentComponentInstallerPolicy();
  ~BraveUserAgentComponentInstallerPolicy() override;

  BraveUserAgentComponentInstallerPolicy(
      const BraveUserAgentComponentInstallerPolicy&) = delete;
  BraveUserAgentComponentInstallerPolicy& operator=(
      const BraveUserAgentComponentInstallerPolicy&) = delete;

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

BraveUserAgentComponentInstallerPolicy::
    BraveUserAgentComponentInstallerPolicy() {
  // Generate hash from public key.
  component_hash_ =
      crypto::SHA256Hash(kBraveUserAgentExceptionsComponentPublicKey);
}

BraveUserAgentComponentInstallerPolicy::
    ~BraveUserAgentComponentInstallerPolicy() = default;

bool BraveUserAgentComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return true;
}

bool BraveUserAgentComponentInstallerPolicy::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
BraveUserAgentComponentInstallerPolicy::OnCustomInstall(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void BraveUserAgentComponentInstallerPolicy::OnCustomUninstall() {}

void BraveUserAgentComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& path,
    base::DictValue manifest) {
  BraveUserAgentExceptions::GetInstance()->OnComponentReady(path);
}

bool BraveUserAgentComponentInstallerPolicy::VerifyInstallation(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

base::FilePath BraveUserAgentComponentInstallerPolicy::GetRelativeInstallDir()
    const {
  return base::FilePath::FromUTF8Unsafe(kBraveUserAgentExceptionsComponentId);
}

void BraveUserAgentComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(component_hash_.begin(), component_hash_.end());
}

std::string BraveUserAgentComponentInstallerPolicy::GetName() const {
  return kBraveUserAgentExceptionsComponentName;
}

update_client::InstallerAttributes
BraveUserAgentComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

bool BraveUserAgentComponentInstallerPolicy::IsBraveComponent() const {
  return true;
}

}  // namespace

void RegisterBraveUserAgentComponent(
    component_updater::ComponentUpdateService* cus) {
  // In test, |cus| could be nullptr.
  if (!base::FeatureList::IsEnabled(
          brave_user_agent::features::kUseBraveUserAgent) ||
      !cus) {
    return;
  }

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<BraveUserAgentComponentInstallerPolicy>());
  installer->Register(
      // After Register, run the callback with component id.
      cus, base::BindOnce([]() {
        brave_component_updater::BraveOnDemandUpdater::GetInstance()
            ->EnsureInstalled(kBraveUserAgentExceptionsComponentId);
      }));
}

}  // namespace brave_user_agent
