/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_youtubedown_component_installer.h"

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

constexpr char kComponentPublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA0l8glPqaai2KyD+R2KoJaaWv7Lafg2"
    "aWijf78E7i5ta4AxL5hMEIXlXA1bJupyDuPWOXH8LAItlgdbJh8xiDzrX7uj4Nr+UiWOrQwd6Y"
    "orvnqHRDzN1NEQBI2gL6IuA22/vNsXKAemu0lS2Gd3FkShuKUJPljdjAskfgn/NHnDUWqxESb3"
    "N6d+shcJw53Tm+nwcxdyDOet6p+VMugIMiUAbb+EhfEmx4iEhJC9XTpl6yjRNzCwaNhcsXrO9U"
    "pdaxZYSYceCm/BKd5TyxNr2MVjGYWKdA1nemhXdz1zvy76ZAUCYPLcSyyKgx5KiJnB8mhtXUWF"
    "Xw5qMzxOoIzAjHeQIDAQAB";
constexpr char kComponentID[] = "jccpmjhflblpphnhgemhlllckflnipjn";
constexpr char kComponentName[] = "youtubedown.js";

constexpr size_t kHashSize = 32;

class PlaylistYoutubeDownComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  explicit PlaylistYoutubeDownComponentInstallerPolicy(
      const std::string& component_public_key,
      const std::string& component_id,
      const std::string& component_name,
      OnComponentReadyCallback callback);
  ~PlaylistYoutubeDownComponentInstallerPolicy() override;

  PlaylistYoutubeDownComponentInstallerPolicy(
      const PlaylistYoutubeDownComponentInstallerPolicy&) = delete;
  PlaylistYoutubeDownComponentInstallerPolicy& operator=(
      const PlaylistYoutubeDownComponentInstallerPolicy&) = delete;

  // component_updater::ComponentInstallerPolicy
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

 private:
  const std::string component_id_;
  const std::string component_name_;
  OnComponentReadyCallback ready_callback_;
  uint8_t component_hash_[kHashSize];
};

PlaylistYoutubeDownComponentInstallerPolicy::
    PlaylistYoutubeDownComponentInstallerPolicy(
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

PlaylistYoutubeDownComponentInstallerPolicy::
    ~PlaylistYoutubeDownComponentInstallerPolicy() = default;

bool PlaylistYoutubeDownComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return true;
}

bool PlaylistYoutubeDownComponentInstallerPolicy::RequiresNetworkEncryption()
    const {
  return false;
}

update_client::CrxInstaller::Result
PlaylistYoutubeDownComponentInstallerPolicy::OnCustomInstall(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void PlaylistYoutubeDownComponentInstallerPolicy::OnCustomUninstall() {}

void PlaylistYoutubeDownComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& path,
    std::unique_ptr<base::DictionaryValue> manifest) {
  ready_callback_.Run(path);
}

bool PlaylistYoutubeDownComponentInstallerPolicy::VerifyInstallation(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

base::FilePath
PlaylistYoutubeDownComponentInstallerPolicy::GetRelativeInstallDir() const {
  return base::FilePath::FromUTF8Unsafe(component_id_);
}

void PlaylistYoutubeDownComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(component_hash_, component_hash_ + kHashSize);
}

std::string PlaylistYoutubeDownComponentInstallerPolicy::GetName() const {
  return component_name_;
}

update_client::InstallerAttributes
PlaylistYoutubeDownComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

std::vector<std::string>
PlaylistYoutubeDownComponentInstallerPolicy::GetMimeTypes() const {
  return std::vector<std::string>();
}

void OnRegistered(const std::string& component_id) {
  BraveOnDemandUpdater::GetInstance()->OnDemandUpdate(component_id);
}

}  // namespace

void RegisterPlaylistYoutubeDownComponent(
    component_updater::ComponentUpdateService* cus,
    OnComponentReadyCallback callback) {
  // In test, |cus| could be nullptr.
  if (!cus)
    return;

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<PlaylistYoutubeDownComponentInstallerPolicy>(
          kComponentPublicKey, kComponentID, kComponentName, callback));
  installer->Register(cus, base::BindOnce(&OnRegistered, kComponentID));
}

}  // namespace playlist
