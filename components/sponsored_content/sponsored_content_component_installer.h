/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_COMPONENT_INSTALLER_H_

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/sponsored_content/sponsored_content_component_callback.h"
#include "components/component_updater/component_installer.h"
#include "crypto/hash.h"

namespace base {
class FilePath;
}  // namespace base

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace sponsored_content {

class SponsoredContentComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  SponsoredContentComponentInstallerPolicy(
      base::span<const uint8_t, crypto::hash::kSha256Size> public_key_sha256,
      const std::string& component_name,
      ComponentCallback callback);

  SponsoredContentComponentInstallerPolicy(
      const SponsoredContentComponentInstallerPolicy&) = delete;
  SponsoredContentComponentInstallerPolicy& operator=(
      const SponsoredContentComponentInstallerPolicy&) = delete;

  ~SponsoredContentComponentInstallerPolicy() override;

  // component_updater::ComponentInstallerPolicy:
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
  const std::string component_name_;
  ComponentCallback component_callback_;
  std::array<uint8_t, crypto::hash::kSha256Size> public_key_sha256_;
};

void RegisterSponsoredContentComponentInstaller(
    component_updater::ComponentUpdateService* component_update_service,
    base::span<const uint8_t, crypto::hash::kSha256Size> public_key_sha256,
    const std::string& component_name,
    ComponentCallback callback);

}  // namespace sponsored_content

#endif  // BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_COMPONENT_INSTALLER_H_
