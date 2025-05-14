// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_user_agent/browser/brave_user_agent_component_installer.h"

#include <memory>
#include <string>
#include <vector>

#include "base/base64.h"
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
  bool IsBraveComponent() const override;

 private:
  const std::string component_id_;
  const std::string component_name_;
  std::array<uint8_t, crypto::kSHA256Length> component_hash_;
};

BraveUserAgentComponentInstallerPolicy::BraveUserAgentComponentInstallerPolicy()
    : component_id_(kBraveUserAgentExceptionsComponentId),
      component_name_(kBraveUserAgentExceptionsComponentName) {
  // Generate hash from public key.
  auto decoded_public_key =
      base::Base64Decode(kBraveUserAgentExceptionsComponentBase64PublicKey);
  CHECK(decoded_public_key);
  component_hash_ = crypto::SHA256Hash(*decoded_public_key);
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
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void BraveUserAgentComponentInstallerPolicy::OnCustomUninstall() {}

void BraveUserAgentComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& path,
    base::Value::Dict manifest) {
  BraveUserAgentExceptions::GetInstance()->OnComponentReady(path);
}

bool BraveUserAgentComponentInstallerPolicy::VerifyInstallation(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

base::FilePath BraveUserAgentComponentInstallerPolicy::GetRelativeInstallDir()
    const {
  return base::FilePath::FromUTF8Unsafe(component_id_);
}

void BraveUserAgentComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(component_hash_.begin(), component_hash_.end());
}

std::string BraveUserAgentComponentInstallerPolicy::GetName() const {
  return component_name_;
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
