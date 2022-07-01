/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/media_detector_component_installer.h"

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

namespace playlist {

namespace {

constexpr size_t kHashSize = 32;
class MediaDetectorComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  explicit MediaDetectorComponentInstallerPolicy(
      const std::string& component_public_key,
      const std::string& component_id,
      const std::string& component_name,
      OnComponentReadyCallback callback);
  ~MediaDetectorComponentInstallerPolicy() override;

  MediaDetectorComponentInstallerPolicy(
      const MediaDetectorComponentInstallerPolicy&) = delete;
  MediaDetectorComponentInstallerPolicy& operator=(
      const MediaDetectorComponentInstallerPolicy&) = delete;

  // component_updater::ComponentInstallerPolicy
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
      const base::Value& manifest,
      const base::FilePath& install_dir) override;
  void OnCustomUninstall() override;
  bool VerifyInstallation(const base::Value& manifest,
                          const base::FilePath& install_dir) const override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& path,
                      base::Value manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;

 private:
  const std::string component_id_;
  const std::string component_name_;
  OnComponentReadyCallback ready_callback_;
  uint8_t component_hash_[kHashSize];
};

MediaDetectorComponentInstallerPolicy::MediaDetectorComponentInstallerPolicy(
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

MediaDetectorComponentInstallerPolicy::
    ~MediaDetectorComponentInstallerPolicy() = default;

bool MediaDetectorComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return true;
}

bool MediaDetectorComponentInstallerPolicy::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
MediaDetectorComponentInstallerPolicy::OnCustomInstall(
    const base::Value& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void MediaDetectorComponentInstallerPolicy::OnCustomUninstall() {}

void MediaDetectorComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& path,
    base::Value manifest) {
  ready_callback_.Run(path);
}

bool MediaDetectorComponentInstallerPolicy::VerifyInstallation(
    const base::Value& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

base::FilePath MediaDetectorComponentInstallerPolicy::GetRelativeInstallDir()
    const {
  return base::FilePath::FromUTF8Unsafe(component_id_);
}

void MediaDetectorComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(component_hash_, component_hash_ + kHashSize);
}

std::string MediaDetectorComponentInstallerPolicy::GetName() const {
  return component_name_;
}

update_client::InstallerAttributes
MediaDetectorComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

}  // namespace

void RegisterMediaDetectorComponent(
    component_updater::ComponentUpdateService* cus,
    OnComponentReadyCallback callback) {
  // In test, |cus| could be nullptr.
  if (!cus)
    return;

  NOTIMPLEMENTED();
}

}  // namespace playlist
