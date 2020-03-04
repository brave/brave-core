/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_component_installer.h"

#include <memory>
#include <string>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_ads/browser/locale_helper.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/ntp_sponsored_images/browser/regional_component_data.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "crypto/sha2.h"

using brave_component_updater::BraveOnDemandUpdater;

namespace {

constexpr char kNTPSponsoredImagesDisplayName[] = "NTP sponsored images";
constexpr size_t kHashSize = 32;

class NTPSponsoredImagesComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  explicit NTPSponsoredImagesComponentInstallerPolicy(
      const RegionalComponentData& regional_component_data,
      OnComponentReadyCallback callback);
  ~NTPSponsoredImagesComponentInstallerPolicy() override;

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
  const RegionalComponentData data_;
  OnComponentReadyCallback ready_callback_;
  uint8_t component_hash_[kHashSize];

  DISALLOW_COPY_AND_ASSIGN(NTPSponsoredImagesComponentInstallerPolicy);
};

NTPSponsoredImagesComponentInstallerPolicy::
NTPSponsoredImagesComponentInstallerPolicy(
    const RegionalComponentData& data, OnComponentReadyCallback callback)
    : data_(data),
      ready_callback_(callback) {
  // Generate hash from public key.
  std::string decoded_public_key;
  base::Base64Decode(data.component_base64_public_key, &decoded_public_key);
  crypto::SHA256HashString(decoded_public_key, component_hash_, kHashSize);
}

NTPSponsoredImagesComponentInstallerPolicy::
~NTPSponsoredImagesComponentInstallerPolicy() {}

bool NTPSponsoredImagesComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return true;
}

bool NTPSponsoredImagesComponentInstallerPolicy::
    RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
NTPSponsoredImagesComponentInstallerPolicy::OnCustomInstall(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void NTPSponsoredImagesComponentInstallerPolicy::OnCustomUninstall() {}

void NTPSponsoredImagesComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& path,
    std::unique_ptr<base::DictionaryValue> manifest) {
  ready_callback_.Run(path);
}

bool NTPSponsoredImagesComponentInstallerPolicy::VerifyInstallation(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

base::FilePath NTPSponsoredImagesComponentInstallerPolicy::
    GetRelativeInstallDir() const {
  return base::FilePath::FromUTF8Unsafe(data_.component_id);
}

void NTPSponsoredImagesComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(component_hash_, component_hash_ + kHashSize);
}

std::string NTPSponsoredImagesComponentInstallerPolicy::GetName() const {
  return base::StringPrintf(
      "%s (%s)", kNTPSponsoredImagesDisplayName, data_.region.c_str());
}

update_client::InstallerAttributes
NTPSponsoredImagesComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

std::vector<std::string>
    NTPSponsoredImagesComponentInstallerPolicy::GetMimeTypes() const {
  return std::vector<std::string>();
}

void OnRegistered(const std::string& component_id) {
  BraveOnDemandUpdater::GetInstance()->OnDemandUpdate(component_id);
}

}  // namespace

void RegisterNTPSponsoredImagesComponent(
    component_updater::ComponentUpdateService* cus,
    const RegionalComponentData& data,
    OnComponentReadyCallback callback) {
  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<NTPSponsoredImagesComponentInstallerPolicy>(
          data, callback));
  installer->Register(cus,
                      base::BindOnce(&OnRegistered, data.component_id));
}
