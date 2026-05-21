// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_user_agent/browser/brave_user_agent_component_installer.h"

#include <stdint.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/brave_user_agent/browser/brave_user_agent_exceptions.h"
#include "brave/components/brave_user_agent/common/features.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"

using brave_component_updater::BraveOnDemandUpdater;

namespace brave_user_agent {

namespace {

inline constexpr char kBraveUserAgentExceptionsComponentName[] =
    "Brave User Agent Exceptions";
inline constexpr char kBraveUserAgentExceptionsComponentId[] =
    "nlpaeekllejnmhoonlpcefpfnpbajbpe";
// This is the SHA-256 of the brave-user-agent component's public key.
// Derived from crypto::SHA256.
inline constexpr std::array<uint8_t, 32>
    kBraveUserAgentExceptionsComponentPublicKeySHA256 = {
        0xdb, 0xf0, 0x44, 0xab, 0xb4, 0x9d, 0xc7, 0xee, 0xdb, 0xf2, 0x45,
        0xf5, 0xdf, 0x10, 0x91, 0xf4, 0xc0, 0x9c, 0x90, 0x92, 0x52, 0xa0,
        0x93, 0xfa, 0xb2, 0x23, 0x06, 0xb1, 0x96, 0x3b, 0x41, 0x35};

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
};

BraveUserAgentComponentInstallerPolicy::
    BraveUserAgentComponentInstallerPolicy() = default;

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
  hash->assign(kBraveUserAgentExceptionsComponentPublicKeySHA256.begin(),
               kBraveUserAgentExceptionsComponentPublicKeySHA256.end());
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
