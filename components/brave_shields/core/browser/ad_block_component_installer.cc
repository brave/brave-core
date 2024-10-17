// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_component_installer.h"

#include <memory>
#include <string>
#include <vector>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "crypto/sha2.h"

using brave_component_updater::BraveOnDemandUpdater;

namespace brave_shields {

namespace {

constexpr size_t kHashSize = 32;

class AdBlockComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  explicit AdBlockComponentInstallerPolicy(
      const std::string& component_public_key,
      const std::string& component_id,
      const std::string& component_name,
      OnComponentReadyCallback callback);
  ~AdBlockComponentInstallerPolicy() override;

  AdBlockComponentInstallerPolicy(const AdBlockComponentInstallerPolicy&) =
      delete;
  AdBlockComponentInstallerPolicy& operator=(
      const AdBlockComponentInstallerPolicy&) = delete;

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
  OnComponentReadyCallback ready_callback_;
  uint8_t component_hash_[kHashSize];
};

AdBlockComponentInstallerPolicy::AdBlockComponentInstallerPolicy(
    const std::string& component_public_key,
    const std::string& component_id,
    const std::string& component_name,
    OnComponentReadyCallback callback)
    : component_id_(component_id),
      component_name_(component_name),
      ready_callback_(callback) {
  // Generate hash from public key.
  std::string decoded_public_key;
  base::Base64Decode(component_public_key, &decoded_public_key);
  crypto::SHA256HashString(decoded_public_key, component_hash_, kHashSize);
}

AdBlockComponentInstallerPolicy::~AdBlockComponentInstallerPolicy() = default;

bool AdBlockComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return true;
}

bool AdBlockComponentInstallerPolicy::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
AdBlockComponentInstallerPolicy::OnCustomInstall(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void AdBlockComponentInstallerPolicy::OnCustomUninstall() {}

void AdBlockComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& path,
    base::Value::Dict manifest) {
  ready_callback_.Run(path);
}

bool AdBlockComponentInstallerPolicy::VerifyInstallation(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

base::FilePath AdBlockComponentInstallerPolicy::GetRelativeInstallDir() const {
  return base::FilePath::FromUTF8Unsafe(component_id_);
}

void AdBlockComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(component_hash_, UNSAFE_TODO(component_hash_ + kHashSize));
}

std::string AdBlockComponentInstallerPolicy::GetName() const {
  return component_name_;
}

update_client::InstallerAttributes
AdBlockComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

bool AdBlockComponentInstallerPolicy::IsBraveComponent() const {
  return true;
}

void OnRegistered(const std::string& component_id) {
  // Unlike other components, which are only installed but not updated in
  // `OnRegistered`, we do always want to update the ad block component upon
  // registration.
  BraveOnDemandUpdater::GetInstance()->OnDemandUpdate(
      component_id, component_updater::OnDemandUpdater::Priority::FOREGROUND);
}

}  // namespace

void RegisterAdBlockDefaultResourceComponent(
    component_updater::ComponentUpdateService* cus,
    OnComponentReadyCallback callback) {
  // In test, |cus| could be nullptr.
  if (!cus) {
    return;
  }

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<AdBlockComponentInstallerPolicy>(
          kAdBlockResourceComponentBase64PublicKey, kAdBlockResourceComponentId,
          kAdBlockResourceComponentName, callback));
  installer->Register(
      cus, base::BindOnce(&OnRegistered, kAdBlockResourceComponentId));
}

void RegisterAdBlockFilterListCatalogComponent(
    component_updater::ComponentUpdateService* cus,
    OnComponentReadyCallback callback) {
  // In test, |cus| could be nullptr.
  if (!cus) {
    return;
  }

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<AdBlockComponentInstallerPolicy>(
          kAdBlockFilterListCatalogComponentBase64PublicKey,
          kAdBlockFilterListCatalogComponentId,
          kAdBlockFilterListCatalogComponentName, callback));
  installer->Register(
      cus, base::BindOnce(&OnRegistered, kAdBlockFilterListCatalogComponentId));
}

void RegisterAdBlockFiltersComponent(
    component_updater::ComponentUpdateService* cus,
    const std::string& component_public_key,
    const std::string& component_id,
    const std::string& component_name,
    OnComponentReadyCallback callback) {
  // In test, |cus| could be nullptr.
  if (!cus) {
    return;
  }

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<AdBlockComponentInstallerPolicy>(
          component_public_key, component_id, component_name, callback));
  installer->Register(cus, base::BindOnce(&OnRegistered, component_id));
}

}  // namespace brave_shields
