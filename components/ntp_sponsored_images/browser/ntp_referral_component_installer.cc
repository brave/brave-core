/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_sponsored_images/browser/ntp_referral_component_installer.h"

#include <memory>
#include <string>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "crypto/sha2.h"

using brave_component_updater::BraveOnDemandUpdater;

namespace {

constexpr char kNTPReferralComponentName[] = "NTP Referral component";

constexpr size_t kHashSize = 32;

class NTPReferralComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  NTPReferralComponentInstallerPolicy(
      const std::string& component_public_key,
      const std::string& component_id,
      const std::string& company_name,
      OnReferralComponentReadyCallback callback);
  ~NTPReferralComponentInstallerPolicy() override;

  // component_updater::ComponentInstallerPolicy
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
      const base::DictionaryValue& manifest,
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
  std::vector<std::string> GetMimeTypes() const override;

 private:
  const std::string component_public_key_;
  const std::string component_id_;
  const std::string company_name_;
  OnReferralComponentReadyCallback ready_callback_;
  uint8_t component_hash_[kHashSize];

  DISALLOW_COPY_AND_ASSIGN(NTPReferralComponentInstallerPolicy);
};

NTPReferralComponentInstallerPolicy::
NTPReferralComponentInstallerPolicy(
    const std::string& component_public_key,
    const std::string& component_id,
    const std::string& company_name,
    OnReferralComponentReadyCallback callback)
    : component_public_key_(component_public_key),
      component_id_(component_id),
      company_name_(company_name),
      ready_callback_(callback) {
  // Generate hash from public key.
  std::string decoded_public_key;
  base::Base64Decode(component_public_key_, &decoded_public_key);
  crypto::SHA256HashString(decoded_public_key, component_hash_, kHashSize);
}

NTPReferralComponentInstallerPolicy::
~NTPReferralComponentInstallerPolicy() = default;

bool NTPReferralComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return true;
}

bool NTPReferralComponentInstallerPolicy::
    RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
NTPReferralComponentInstallerPolicy::OnCustomInstall(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void NTPReferralComponentInstallerPolicy::OnCustomUninstall() {}

void NTPReferralComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& path,
    std::unique_ptr<base::DictionaryValue> manifest) {
  ready_callback_.Run(path);
}

bool NTPReferralComponentInstallerPolicy::VerifyInstallation(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

base::FilePath NTPReferralComponentInstallerPolicy::
    GetRelativeInstallDir() const {
  return base::FilePath::FromUTF8Unsafe(component_id_);
}

void NTPReferralComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(component_hash_, component_hash_ + kHashSize);
}

std::string NTPReferralComponentInstallerPolicy::GetName() const {
  return base::StringPrintf(
      "%s (%s)", kNTPReferralComponentName, company_name_.c_str());
}

update_client::InstallerAttributes
NTPReferralComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

std::vector<std::string>
    NTPReferralComponentInstallerPolicy::GetMimeTypes() const {
  return std::vector<std::string>();
}

void OnRegistered(const std::string& component_id) {
  BraveOnDemandUpdater::GetInstance()->OnDemandUpdate(component_id);
}

}  // namespace

void RegisterNTPReferralComponent(
    component_updater::ComponentUpdateService* cus,
    const std::string& component_public_key,
    const std::string& component_id,
    const std::string& company_name,
    OnReferralComponentReadyCallback callback) {
  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<NTPReferralComponentInstallerPolicy>(
          component_public_key,
          component_id,
          company_name,
          callback));
  installer->Register(cus, base::BindOnce(&OnRegistered, component_id));
}
