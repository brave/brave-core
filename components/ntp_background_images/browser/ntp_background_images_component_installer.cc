/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_component_installer.h"

#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "components/component_updater/component_updater_service.h"
#include "crypto/sha2.h"

using brave_component_updater::BraveOnDemandUpdater;

namespace ntp_background_images {

namespace {

constexpr uint8_t kNTPBackgroundImagesComponentPublicKeySHA256[32] = {
    0x0e, 0xe9, 0x2c, 0xe9, 0xcc, 0x21, 0xf5, 0x6e, 0x42, 0xe0, 0x31,
    0x3f, 0xd0, 0x65, 0x27, 0x4b, 0x69, 0x12, 0xae, 0xea, 0xdf, 0xce,
    0xce, 0xe0, 0x89, 0x83, 0xdd, 0x25, 0x3c, 0x4c, 0x8a, 0xc8};
static_assert(std::size(kNTPBackgroundImagesComponentPublicKeySHA256) ==
              crypto::kSHA256Length);
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
    : ready_callback_(std::move(callback)) {}

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
  hash->assign_range(kNTPBackgroundImagesComponentPublicKeySHA256);
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
