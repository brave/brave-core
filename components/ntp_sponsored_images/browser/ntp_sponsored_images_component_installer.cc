/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_component_installer.h"

#include "base/base64.h"
#include "base/bind.h"
#include "base/callback.h"
#include "brave/components/brave_ads/browser/locale_helper.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_component_manager.h"
#include "brave/components/ntp_sponsored_images/browser/regional_component_data.h"
#include "brave/vendor/bat-native-ads/src/bat/ads/internal/locale_helper.h"
#include "components/component_updater/component_installer.h"
#include "components/crx_file/id_util.h"
#include "crypto/sha2.h"

namespace {

constexpr char kNTPSponsoredImagesDisplayName[] = "NTP sponsored images";
constexpr char kNTPSponsoredImagesBaseDirectory[] = "NTPSponsoredImages";

class NTPSponsoredImagesComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  explicit NTPSponsoredImagesComponentInstallerPolicy(
      NTPSponsoredImagesComponentManager* manager);
  ~NTPSponsoredImagesComponentInstallerPolicy() override {}

 private:
  // The following methods override ComponentInstallerPolicy.
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

  NTPSponsoredImagesComponentManager* manager_;

  DISALLOW_COPY_AND_ASSIGN(NTPSponsoredImagesComponentInstallerPolicy);
};

NTPSponsoredImagesComponentInstallerPolicy::
    NTPSponsoredImagesComponentInstallerPolicy(
        NTPSponsoredImagesComponentManager* manager)
        : manager_(manager) {}

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
  manager_->OnComponentReady(path);
}

bool NTPSponsoredImagesComponentInstallerPolicy::VerifyInstallation(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

base::FilePath NTPSponsoredImagesComponentInstallerPolicy::
    GetRelativeInstallDir() const {
  return base::FilePath::FromUTF8Unsafe(kNTPSponsoredImagesBaseDirectory);
}

void NTPSponsoredImagesComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  const std::string locale =
      brave_ads::LocaleHelper::GetInstance()->GetLocale();
  if (const auto& data = GetRegionalComponentData(
          helper::Locale::GetRegionCode(locale))) {
    // Generate hash from public key.
    std::string decoded_public_key;
    base::Base64Decode(data->component_base64_public_key, &decoded_public_key);
    const size_t kIdSize = 32;
    uint8_t component_hash[kIdSize];
    crypto::SHA256HashString(decoded_public_key, component_hash, kIdSize);
    hash->assign(component_hash,
                 component_hash + kIdSize);
  }
}

std::string NTPSponsoredImagesComponentInstallerPolicy::GetName() const {
  return kNTPSponsoredImagesDisplayName;
}

update_client::InstallerAttributes
NTPSponsoredImagesComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

std::vector<std::string>
    NTPSponsoredImagesComponentInstallerPolicy::GetMimeTypes() const {
  return std::vector<std::string>();
}

}  // namespace

void RegisterNTPSponsoredImagesComponent(
    component_updater::ComponentUpdateService* cus,
    NTPSponsoredImagesComponentManager* manager) {
  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<NTPSponsoredImagesComponentInstallerPolicy>(manager));
  installer->Register(cus, base::OnceClosure());
}
