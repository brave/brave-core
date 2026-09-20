/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/sponsored_content/ntp_sponsored_images_component_installer.h"

#include <memory>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/containers/to_vector.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_update_util.h"
#include "components/component_updater/component_updater_service.h"

using brave_component_updater::BraveOnDemandUpdater;

namespace ntp_background_images {

namespace {

void RegisterNTPSponsoredImagesComponentCallback(
    const std::string& component_id) {
  // Unlike other components that are only installed during registration,
  // we always update the sponsored images component upon registration.
  CheckAndUpdateSponsoredImagesComponent(component_id);
}

}  // namespace

NTPSponsoredImagesComponentInstallerPolicy::
    NTPSponsoredImagesComponentInstallerPolicy(
        const std::string& component_public_key,
        const std::string& component_id,
        const std::string& component_name,
        ComponentReadyCallback callback)
    : component_id_(component_id),
      component_name_(component_name),
      ready_callback_(std::move(callback)) {
  // Generate hash from public key.
  auto decoded_public_key = base::Base64Decode(component_public_key);
  CHECK(decoded_public_key);
  component_hash_ = crypto::SHA256Hash(*decoded_public_key);
}

NTPSponsoredImagesComponentInstallerPolicy::
    ~NTPSponsoredImagesComponentInstallerPolicy() = default;

bool NTPSponsoredImagesComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return true;
}

bool NTPSponsoredImagesComponentInstallerPolicy::RequiresNetworkEncryption()
    const {
  return false;
}

update_client::CrxInstaller::Result
NTPSponsoredImagesComponentInstallerPolicy::OnCustomInstall(
    const base::DictValue& /*manifest*/,
    const base::FilePath& /*install_dir*/) {
  return update_client::CrxInstaller::Result(0);
}

void NTPSponsoredImagesComponentInstallerPolicy::OnCustomUninstall() {}

void NTPSponsoredImagesComponentInstallerPolicy::ComponentReady(
    const base::Version& /*version*/,
    const base::FilePath& path,
    base::DictValue /*manifest*/) {
  ready_callback_.Run(path);
}

bool NTPSponsoredImagesComponentInstallerPolicy::VerifyInstallation(
    const base::DictValue& /*manifest*/,
    const base::FilePath& /*install_dir*/) const {
  return true;
}

base::FilePath
NTPSponsoredImagesComponentInstallerPolicy::GetRelativeInstallDir() const {
  return base::FilePath::FromUTF8Unsafe(component_id_);
}

void NTPSponsoredImagesComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  *hash = base::ToVector(component_hash_);
}

std::string NTPSponsoredImagesComponentInstallerPolicy::GetName() const {
  return component_name_;
}

update_client::InstallerAttributes
NTPSponsoredImagesComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

bool NTPSponsoredImagesComponentInstallerPolicy::IsBraveComponent() const {
  return true;
}

void RegisterNTPSponsoredImagesComponent(
    component_updater::ComponentUpdateService* component_update_service,
    const std::string& component_public_key,
    const std::string& component_id,
    const std::string& component_name,
    ComponentReadyCallback callback) {
  if (!component_update_service ||
      BraveOnDemandUpdater::GetInstance()->is_component_update_disabled()) {
    // In test, `component_update_service` could be nullptr.
    return;
  }

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<NTPSponsoredImagesComponentInstallerPolicy>(
          component_public_key, component_id, component_name,
          std::move(callback)));
  installer->Register(
      component_update_service,
      base::BindOnce(&RegisterNTPSponsoredImagesComponentCallback,
                     component_id));
}

}  // namespace ntp_background_images
