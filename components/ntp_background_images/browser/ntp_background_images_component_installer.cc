/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_component_installer.h"

#include <memory>
#include <string>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/containers/to_vector.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "components/component_updater/component_updater_service.h"

using brave_component_updater::BraveOnDemandUpdater;

namespace ntp_background_images {

namespace {

constexpr char kNTPBackgroundImagesComponentPublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4L9XGAiVhCL8oi5aQhFrVllsw6VebX"
    "igTj5ow3e0fYeEztjM9FOgqMD6pl0AB8u05xKUPcdpIZqCguEzXyXh5vn+"
    "BWoEGtVezEEfjd33T4drJAYwEBvgWcFVVLNWku1/53f6TZp8IiiaOhKIANUtn/Zvw/"
    "0nUYa10nwxK4P3he4Ahj0CO6HVeu9zNRCdZFSkYdMnPnNYTU+qN88OT1DBsV1xQgd3qK+"
    "MkzPDF1okHi9a+IXiHa3FVY++QmtSrMgetJnS/"
    "qBt6VsZcejcQCd1KIpgHNyoVl5rodtBRj25o48SxYePrssMRTv9vAQmRUZZukOIL/"
    "HdeqjCHIOSQTrFEQIDAQAB";  // NOLINT
constexpr char kNTPBackgroundImagesComponentID[] =
    "aoojcmojmmcbpfgoecoadbdpnagfchel";
constexpr char kNTPBackgroundImagesComponentName[] = "NTP Background Images";

void RegisterNTPBackgroundImagesComponentCallback(
    const std::string& component_id) {
  BraveOnDemandUpdater::GetInstance()->EnsureInstalled(component_id);
}

}  // namespace

NTPBackgroundImagesComponentInstallerPolicy::
    NTPBackgroundImagesComponentInstallerPolicy(ComponentReadyCallback callback)
    : ready_callback_(std::move(callback)) {
  // Generate hash from public key.
  auto decoded_public_key =
      base::Base64Decode(kNTPBackgroundImagesComponentPublicKey);
  CHECK(decoded_public_key);
  component_hash_ = crypto::SHA256Hash(*decoded_public_key);
}

NTPBackgroundImagesComponentInstallerPolicy::
    ~NTPBackgroundImagesComponentInstallerPolicy() = default;

bool NTPBackgroundImagesComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return true;
}

bool NTPBackgroundImagesComponentInstallerPolicy::RequiresNetworkEncryption()
    const {
  return false;
}

update_client::CrxInstaller::Result
NTPBackgroundImagesComponentInstallerPolicy::OnCustomInstall(
    const base::DictValue& /*manifest*/,
    const base::FilePath& /*install_dir*/) {
  return update_client::CrxInstaller::Result(0);
}

void NTPBackgroundImagesComponentInstallerPolicy::OnCustomUninstall() {}

void NTPBackgroundImagesComponentInstallerPolicy::ComponentReady(
    const base::Version& /*version*/,
    const base::FilePath& path,
    base::DictValue /*manifest*/) {
  ready_callback_.Run(path);
}

bool NTPBackgroundImagesComponentInstallerPolicy::VerifyInstallation(
    const base::DictValue& /*manifest*/,
    const base::FilePath& /*install_dir*/) const {
  return true;
}

base::FilePath
NTPBackgroundImagesComponentInstallerPolicy::GetRelativeInstallDir() const {
  return base::FilePath::FromUTF8Unsafe(kNTPBackgroundImagesComponentID);
}

void NTPBackgroundImagesComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  *hash = base::ToVector(component_hash_);
}

std::string NTPBackgroundImagesComponentInstallerPolicy::GetName() const {
  return kNTPBackgroundImagesComponentName;
}

update_client::InstallerAttributes
NTPBackgroundImagesComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

bool NTPBackgroundImagesComponentInstallerPolicy::IsBraveComponent() const {
  return true;
}

void RegisterNTPBackgroundImagesComponent(
    component_updater::ComponentUpdateService* component_update_service,
    ComponentReadyCallback callback) {
  if (!component_update_service ||
      BraveOnDemandUpdater::GetInstance()->is_component_update_disabled()) {
    // In test, `component_update_service` could be nullptr.
    return;
  }

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<NTPBackgroundImagesComponentInstallerPolicy>(
          std::move(callback)));
  installer->Register(
      component_update_service,
      base::BindOnce(&RegisterNTPBackgroundImagesComponentCallback,
                     kNTPBackgroundImagesComponentID));
}

}  // namespace ntp_background_images
