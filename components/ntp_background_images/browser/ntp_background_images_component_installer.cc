/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_component_installer.h"

#include <memory>
#include <string>
#include <vector>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/ntp_background_images/browser/sponsored_images_component_data.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "crypto/sha2.h"

using brave_component_updater::BraveOnDemandUpdater;

namespace ntp_background_images {

namespace {

constexpr size_t kHashSize = 32;
constexpr char kNTPBIComponentPublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4L9XGAiVhCL8oi5aQhFrVllsw6VebX"
    "igTj5ow3e0fYeEztjM9FOgqMD6pl0AB8u05xKUPcdpIZqCguEzXyXh5vn+"
    "BWoEGtVezEEfjd33T4drJAYwEBvgWcFVVLNWku1/53f6TZp8IiiaOhKIANUtn/Zvw/"
    "0nUYa10nwxK4P3he4Ahj0CO6HVeu9zNRCdZFSkYdMnPnNYTU+qN88OT1DBsV1xQgd3qK+"
    "MkzPDF1okHi9a+IXiHa3FVY++QmtSrMgetJnS/"
    "qBt6VsZcejcQCd1KIpgHNyoVl5rodtBRj25o48SxYePrssMRTv9vAQmRUZZukOIL/"
    "HdeqjCHIOSQTrFEQIDAQAB";  // NOLINT
constexpr char kNTPBIComponentID[] = "aoojcmojmmcbpfgoecoadbdpnagfchel";

class NTPBackgroundImagesComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  explicit NTPBackgroundImagesComponentInstallerPolicy(
      const std::string& component_public_key,
      const std::string& component_id,
      const std::string& component_name,
      OnComponentReadyCallback callback);
  ~NTPBackgroundImagesComponentInstallerPolicy() override;

  NTPBackgroundImagesComponentInstallerPolicy(
      const NTPBackgroundImagesComponentInstallerPolicy&) = delete;
  NTPBackgroundImagesComponentInstallerPolicy& operator=(
      const NTPBackgroundImagesComponentInstallerPolicy&) = delete;

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

NTPBackgroundImagesComponentInstallerPolicy::
NTPBackgroundImagesComponentInstallerPolicy(
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

NTPBackgroundImagesComponentInstallerPolicy::
~NTPBackgroundImagesComponentInstallerPolicy() = default;

bool NTPBackgroundImagesComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return true;
}

bool NTPBackgroundImagesComponentInstallerPolicy::
    RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
NTPBackgroundImagesComponentInstallerPolicy::OnCustomInstall(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void NTPBackgroundImagesComponentInstallerPolicy::OnCustomUninstall() {}

void NTPBackgroundImagesComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& path,
    base::Value::Dict manifest) {
  ready_callback_.Run(path);
}

bool NTPBackgroundImagesComponentInstallerPolicy::VerifyInstallation(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

base::FilePath NTPBackgroundImagesComponentInstallerPolicy::
    GetRelativeInstallDir() const {
  return base::FilePath::FromUTF8Unsafe(component_id_);
}

void NTPBackgroundImagesComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(component_hash_, UNSAFE_TODO(component_hash_ + kHashSize));
}

std::string NTPBackgroundImagesComponentInstallerPolicy::GetName() const {
  return component_name_;
}

update_client::InstallerAttributes
NTPBackgroundImagesComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

bool NTPBackgroundImagesComponentInstallerPolicy::IsBraveComponent() const {
  return true;
}

void OnRegistered(const std::string& component_id) {
  BraveOnDemandUpdater::GetInstance()->EnsureInstalled(component_id);
}

}  // namespace

void RegisterNTPBackgroundImagesComponent(
    component_updater::ComponentUpdateService* cus,
    OnComponentReadyCallback callback) {
  // In test, |cus| could be nullptr.
  if (!cus)
    return;

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<NTPBackgroundImagesComponentInstallerPolicy>(
          kNTPBIComponentPublicKey, kNTPBIComponentID, "NTP Background Images",
          callback));
  installer->Register(cus, base::BindOnce(&OnRegistered, kNTPBIComponentID));
}

void RegisterNTPSponsoredImagesComponent(
    component_updater::ComponentUpdateService* cus,
    const std::string& component_public_key,
    const std::string& component_id,
    const std::string& component_name,
    OnComponentReadyCallback callback) {
  // In test, |cus| could be nullptr.
  if (!cus)
    return;

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<NTPBackgroundImagesComponentInstallerPolicy>(
          component_public_key,
          component_id,
          component_name,
          callback));
  installer->Register(cus,
                      base::BindOnce(&OnRegistered, component_id));
}

}  // namespace ntp_background_images
