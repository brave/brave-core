/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_CONTENT_NTP_SPONSORED_IMAGES_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_CONTENT_NTP_SPONSORED_IMAGES_COMPONENT_INSTALLER_H_

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "brave/components/ntp_background_images/browser/component_ready_callback.h"
#include "components/component_updater/component_installer.h"
#include "crypto/sha2.h"

namespace base {
class FilePath;
}  // namespace base

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace ntp_background_images {

class NTPSponsoredImagesComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  NTPSponsoredImagesComponentInstallerPolicy(
      const std::string& component_public_key,
      const std::string& component_id,
      const std::string& component_name,
      ComponentReadyCallback callback);

  NTPSponsoredImagesComponentInstallerPolicy(
      const NTPSponsoredImagesComponentInstallerPolicy&) = delete;
  NTPSponsoredImagesComponentInstallerPolicy& operator=(
      const NTPSponsoredImagesComponentInstallerPolicy&) = delete;

  ~NTPSponsoredImagesComponentInstallerPolicy() override;

  // component_updater::ComponentInstallerPolicy
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
      const base::DictValue& manifest,
      const base::FilePath& install_dir) override;
  void OnCustomUninstall() override;
  bool VerifyInstallation(const base::DictValue& manifest,
                          const base::FilePath& install_dir) const override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& path,
                      base::DictValue manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;
  bool IsBraveComponent() const override;

 private:
  const std::string component_id_;
  const std::string component_name_;
  ComponentReadyCallback ready_callback_;
  std::array<uint8_t, crypto::kSHA256Length> component_hash_;
};

void RegisterNTPSponsoredImagesComponent(
    component_updater::ComponentUpdateService* component_update_service,
    const std::string& component_public_key,
    const std::string& component_id,
    const std::string& component_name,
    ComponentReadyCallback callback);

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_CONTENT_NTP_SPONSORED_IMAGES_COMPONENT_INSTALLER_H_
