/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/media_detector_component_installer.h"

#include <memory>
#include <string>
#include <vector>

#include "base/base64.h"
#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "crypto/sha2.h"

using brave_component_updater::BraveOnDemandUpdater;

namespace playlist {

namespace {

constexpr char kComponentID[] = "jccpmjhflblpphnhgemhlllckflnipjn";

class MediaDetectorComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  explicit MediaDetectorComponentInstallerPolicy(
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
  OnComponentReadyCallback ready_callback_;
  std::array<uint8_t, crypto::kSHA256Length> component_hash_;
};

MediaDetectorComponentInstallerPolicy::MediaDetectorComponentInstallerPolicy(
    OnComponentReadyCallback callback)
    : ready_callback_(callback) {
  // Generate hash from public key.
  constexpr char kComponentPublicKey[] =
      "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA0l8glPqaai2KyD+"
      "R2KoJaaWv7Lafg2"
      "aWijf78E7i5ta4AxL5hMEIXlXA1bJupyDuPWOXH8LAItlgdbJh8xiDzrX7uj4Nr+"
      "UiWOrQwd6Y"
      "orvnqHRDzN1NEQBI2gL6IuA22/vNsXKAemu0lS2Gd3FkShuKUJPljdjAskfgn/"
      "NHnDUWqxESb3"
      "N6d+shcJw53Tm+nwcxdyDOet6p+VMugIMiUAbb+"
      "EhfEmx4iEhJC9XTpl6yjRNzCwaNhcsXrO9U"
      "pdaxZYSYceCm/"
      "BKd5TyxNr2MVjGYWKdA1nemhXdz1zvy76ZAUCYPLcSyyKgx5KiJnB8mhtXUWF"
      "Xw5qMzxOoIzAjHeQIDAQAB";

  auto decoded_public_key = base::Base64Decode(kComponentPublicKey);
  CHECK(decoded_public_key);
  component_hash_ = crypto::SHA256Hash(*decoded_public_key);
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
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void MediaDetectorComponentInstallerPolicy::OnCustomUninstall() {}

void MediaDetectorComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& path,
    base::Value::Dict manifest) {
  ready_callback_.Run(path);
}

bool MediaDetectorComponentInstallerPolicy::VerifyInstallation(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

base::FilePath MediaDetectorComponentInstallerPolicy::GetRelativeInstallDir()
    const {
  return base::FilePath::FromUTF8Unsafe(kComponentID);
}

void MediaDetectorComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  *hash = base::ToVector(component_hash_);
}

std::string MediaDetectorComponentInstallerPolicy::GetName() const {
  return "playlist-component";
}

update_client::InstallerAttributes
MediaDetectorComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

bool MediaDetectorComponentInstallerPolicy::IsBraveComponent() const {
  return true;
}

void OnRegisteredToComponentUpdateService() {
  BraveOnDemandUpdater::GetInstance()->EnsureInstalled(kComponentID);
}

}  // namespace

void RegisterMediaDetectorComponent(
    component_updater::ComponentUpdateService* cus,
    OnComponentReadyCallback callback) {
  // In test, |cus| could be nullptr.
  if (!cus)
    return;

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<MediaDetectorComponentInstallerPolicy>(callback));
  installer->Register(cus,
                      base::BindOnce(OnRegisteredToComponentUpdateService));
}

}  // namespace playlist
