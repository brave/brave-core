/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sponsored_content/sponsored_content_component_installer.h"

#include <memory>
#include <utility>

#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/sponsored_content/sponsored_content_on_demand_update.h"
#include "components/component_updater/component_updater_service.h"
#include "components/crx_file/id_util.h"

namespace sponsored_content {

namespace {

void RegisterSponsoredContentComponentInstallerCallback(
    const std::string& component_id) {
  // Unlike most components, sponsored content is expected to change
  // frequently, so check for an update right after registration.
  QueueSponsoredContentComponentUpdate(component_id);
}

}  // namespace

SponsoredContentComponentInstallerPolicy::
    SponsoredContentComponentInstallerPolicy(
        base::span<const uint8_t, crypto::hash::kSha256Size> public_key_sha256,
        const std::string& component_name,
        ComponentCallback callback)
    : component_name_(component_name),
      component_callback_(std::move(callback)) {
  base::span(public_key_sha256_).copy_from(public_key_sha256);
}

SponsoredContentComponentInstallerPolicy::
    ~SponsoredContentComponentInstallerPolicy() = default;

bool SponsoredContentComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return true;
}

bool SponsoredContentComponentInstallerPolicy::RequiresNetworkEncryption()
    const {
  return false;
}

update_client::CrxInstaller::Result
SponsoredContentComponentInstallerPolicy::OnCustomInstall(
    const base::DictValue& /*manifest*/,
    const base::FilePath& /*install_dir*/) {
  return update_client::CrxInstaller::Result(0);
}

void SponsoredContentComponentInstallerPolicy::OnCustomUninstall() {}

void SponsoredContentComponentInstallerPolicy::ComponentReady(
    const base::Version& /*version*/,
    const base::FilePath& path,
    base::DictValue /*manifest*/) {
  component_callback_.Run(path);
}

bool SponsoredContentComponentInstallerPolicy::VerifyInstallation(
    const base::DictValue& /*manifest*/,
    const base::FilePath& /*install_dir*/) const {
  return true;
}

base::FilePath SponsoredContentComponentInstallerPolicy::GetRelativeInstallDir()
    const {
  return base::FilePath::FromUTF8Unsafe(
      crx_file::id_util::GenerateIdFromHash(public_key_sha256_));
}

void SponsoredContentComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign_range(public_key_sha256_);
}

std::string SponsoredContentComponentInstallerPolicy::GetName() const {
  return component_name_;
}

update_client::InstallerAttributes
SponsoredContentComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

bool SponsoredContentComponentInstallerPolicy::IsBraveComponent() const {
  return true;
}

void RegisterSponsoredContentComponentInstaller(
    component_updater::ComponentUpdateService* component_update_service,
    base::span<const uint8_t, crypto::hash::kSha256Size> public_key_sha256,
    const std::string& component_name,
    ComponentCallback callback) {
  if (!component_update_service ||
      brave_component_updater::BraveOnDemandUpdater::GetInstance()
          ->is_component_update_disabled()) {
    // In test, `component_update_service` could be nullptr.
    return;
  }

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<SponsoredContentComponentInstallerPolicy>(
          public_key_sha256, component_name, std::move(callback)));
  installer->Register(
      component_update_service,
      base::BindOnce(&RegisterSponsoredContentComponentInstallerCallback,
                     crx_file::id_util::GenerateIdFromHash(public_key_sha256)));
}

}  // namespace sponsored_content
