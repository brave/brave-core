/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/https_everywhere_component_installer.h"

#include <memory>
#include <string>
#include <vector>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "crypto/sha2.h"

using brave_component_updater::BraveOnDemandUpdater;

namespace brave_shields {

namespace {

constexpr size_t kHashSize = 32;
const char kHTTPSEverywhereComponentName[] = "Brave HTTPS Everywhere Updater";
const char kHTTPSEverywhereComponentId[] = "oofiananboodjbbmdelgdommihjbkfag";
const char kHTTPSEverywhereComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvn9zSMjTmhkQyrZu5UdN"
    "350nPqLoSeCYngcC7yDFwaUHjoBQXCZqGeDC69ciCQ2mlRhcV2nxXqlUDkiC6+7m"
    "651nI+gi4oVqHagc7EFUyGA0yuIk7qIMvCBdH7wbET27de0rzbRzRht9EKzEjIhC"
    "BtoPnmyrO/8qPrH4XR4cPfnFPuJssBBxC1B35H7rh0Br9qePhPDDe9OjyqYxPuio"
    "+YcC9obL4g5krVrfrlKLfFNpIewUcJyBpSlCgfxEyEhgDkK9cILTMUi5vC7GxS3P"
    "OtZqgfRg8Da4i+NwmjQqrz0JFtPMMSyUnmeMj+mSOL4xZVWr8fU2/GOCXs9gczDp"
    "JwIDAQAB";

class HTTPSEverywhereComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  explicit HTTPSEverywhereComponentInstallerPolicy(
      const std::string& component_public_key,
      const std::string& component_id,
      const std::string& component_name,
      OnComponentReadyCallback callback);
  ~HTTPSEverywhereComponentInstallerPolicy() override;

  HTTPSEverywhereComponentInstallerPolicy(
      const HTTPSEverywhereComponentInstallerPolicy&) = delete;
  HTTPSEverywhereComponentInstallerPolicy& operator=(
      const HTTPSEverywhereComponentInstallerPolicy&) = delete;

  // component_updater::ComponentInstallerPolicy
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
      const base::Value::Dict& manifest,
      const base::FilePath& install_dir) override;
  void OnCustomUninstall() override;
  bool VerifyInstallation(const base::DictionaryValue& manifest,
                          const base::FilePath& install_dir) const override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& path,
                      std::unique_ptr<base::DictionaryValue> manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;

 private:
  const std::string component_id_;
  const std::string component_name_;
  OnComponentReadyCallback ready_callback_;
  uint8_t component_hash_[kHashSize];
};

HTTPSEverywhereComponentInstallerPolicy::
    HTTPSEverywhereComponentInstallerPolicy(
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

HTTPSEverywhereComponentInstallerPolicy::
    ~HTTPSEverywhereComponentInstallerPolicy() = default;

bool HTTPSEverywhereComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return true;
}

bool HTTPSEverywhereComponentInstallerPolicy::RequiresNetworkEncryption()
    const {
  return false;
}

update_client::CrxInstaller::Result
HTTPSEverywhereComponentInstallerPolicy::OnCustomInstall(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void HTTPSEverywhereComponentInstallerPolicy::OnCustomUninstall() {}

void HTTPSEverywhereComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& path,
    std::unique_ptr<base::DictionaryValue> manifest) {
  ready_callback_.Run(path);
}

bool HTTPSEverywhereComponentInstallerPolicy::VerifyInstallation(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

base::FilePath HTTPSEverywhereComponentInstallerPolicy::GetRelativeInstallDir()
    const {
  return base::FilePath::FromUTF8Unsafe(component_id_);
}

void HTTPSEverywhereComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(component_hash_, component_hash_ + kHashSize);
}

std::string HTTPSEverywhereComponentInstallerPolicy::GetName() const {
  return component_name_;
}

update_client::InstallerAttributes
HTTPSEverywhereComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

void OnRegistered(const std::string& component_id) {
  BraveOnDemandUpdater::GetInstance()->OnDemandUpdate(component_id);
}

}  // namespace

void RegisterHTTPSEverywhereComponent(
    component_updater::ComponentUpdateService* cus,
    OnComponentReadyCallback callback) {
  // In test, |cus| could be nullptr.
  if (!cus)
    return;

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<HTTPSEverywhereComponentInstallerPolicy>(
          kHTTPSEverywhereComponentBase64PublicKey, kHTTPSEverywhereComponentId,
          kHTTPSEverywhereComponentName, callback));
  installer->Register(
      cus, base::BindOnce(&OnRegistered, kHTTPSEverywhereComponentId));
}

}  // namespace brave_shields
