/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_sponsored_images/browser/ntp_referral_mapper_component_installer.h"

#include <memory>
#include <string>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/callback.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "crypto/sha2.h"

using brave_component_updater::BraveOnDemandUpdater;

namespace {

constexpr char kNTPReferralMapperComponentName[] =
    "NTP referral component mapping table";
// TODO(simonhong): generate mapper component id.
constexpr char kNTPReferralMapperComponentPublicKey[] = "";
constexpr char kNTPReferralMapperComponentID[] = "";

constexpr size_t kHashSize = 32;

class NTPReferralMapperComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  explicit NTPReferralMapperComponentInstallerPolicy(
      OnMapperComponentReadyCallback callback);
  ~NTPReferralMapperComponentInstallerPolicy() override;

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
  OnMapperComponentReadyCallback ready_callback_;
  uint8_t component_hash_[kHashSize];

  DISALLOW_COPY_AND_ASSIGN(NTPReferralMapperComponentInstallerPolicy);
};

NTPReferralMapperComponentInstallerPolicy::
NTPReferralMapperComponentInstallerPolicy(
    OnMapperComponentReadyCallback callback)
    : ready_callback_(callback) {
  // Generate hash from public key.
  std::string decoded_public_key;
  base::Base64Decode(kNTPReferralMapperComponentPublicKey, &decoded_public_key);
  crypto::SHA256HashString(decoded_public_key, component_hash_, kHashSize);
}

NTPReferralMapperComponentInstallerPolicy::
~NTPReferralMapperComponentInstallerPolicy() = default;

bool NTPReferralMapperComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return true;
}

bool NTPReferralMapperComponentInstallerPolicy::
    RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
NTPReferralMapperComponentInstallerPolicy::OnCustomInstall(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void NTPReferralMapperComponentInstallerPolicy::OnCustomUninstall() {}

void NTPReferralMapperComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& path,
    std::unique_ptr<base::DictionaryValue> manifest) {
  ready_callback_.Run(path);
}

bool NTPReferralMapperComponentInstallerPolicy::VerifyInstallation(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

base::FilePath NTPReferralMapperComponentInstallerPolicy::
    GetRelativeInstallDir() const {
  return base::FilePath::FromUTF8Unsafe(kNTPReferralMapperComponentID);
}

void NTPReferralMapperComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(component_hash_, component_hash_ + kHashSize);
}

std::string NTPReferralMapperComponentInstallerPolicy::GetName() const {
  return kNTPReferralMapperComponentName;
}

update_client::InstallerAttributes
NTPReferralMapperComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

std::vector<std::string>
    NTPReferralMapperComponentInstallerPolicy::GetMimeTypes() const {
  return std::vector<std::string>();
}

void OnRegistered() {
  BraveOnDemandUpdater::GetInstance()->OnDemandUpdate(
      kNTPReferralMapperComponentID);
}

}  // namespace

void RegisterNTPReferralMapperComponent(
    component_updater::ComponentUpdateService* cus,
    OnMapperComponentReadyCallback callback) {
  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<NTPReferralMapperComponentInstallerPolicy>(callback));
  installer->Register(cus, base::BindOnce(&OnRegistered));
}
