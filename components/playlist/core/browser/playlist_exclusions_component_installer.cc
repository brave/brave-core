/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/core/browser/playlist_exclusions_component_installer.h"

#include <stdint.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/playlist/core/browser/playlist_exclusions.h"
#include "brave/components/playlist/core/common/constants.h"
#include "brave/components/playlist/core/common/features.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"

using brave_component_updater::BraveOnDemandUpdater;

namespace playlist {

namespace {

inline constexpr std::array<uint8_t, 32>
    kPlaylistExclusionsComponentPublicKeySHA256 = {
        0xa0, 0x92, 0x8e, 0x87, 0x35, 0xdd, 0xa3, 0x95, 0xf2, 0x88, 0x3d,
        0xc7, 0x2a, 0x04, 0x2b, 0xc9, 0xff, 0x4b, 0xcc, 0xfb, 0x77, 0xb6,
        0xd4, 0xc8, 0x89, 0x68, 0x28, 0xf7, 0xda, 0x08, 0x03, 0xb4};

class PlaylistExclusionsComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  PlaylistExclusionsComponentInstallerPolicy();
  ~PlaylistExclusionsComponentInstallerPolicy() override;

  PlaylistExclusionsComponentInstallerPolicy(
      const PlaylistExclusionsComponentInstallerPolicy&) = delete;
  PlaylistExclusionsComponentInstallerPolicy& operator=(
      const PlaylistExclusionsComponentInstallerPolicy&) = delete;

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
};

PlaylistExclusionsComponentInstallerPolicy::
    PlaylistExclusionsComponentInstallerPolicy() {
  CHECK(base::FeatureList::IsEnabled(features::kPlaylist));
}

PlaylistExclusionsComponentInstallerPolicy::
    ~PlaylistExclusionsComponentInstallerPolicy() = default;

bool PlaylistExclusionsComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return true;
}

bool PlaylistExclusionsComponentInstallerPolicy::RequiresNetworkEncryption()
    const {
  return false;
}

update_client::CrxInstaller::Result
PlaylistExclusionsComponentInstallerPolicy::OnCustomInstall(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void PlaylistExclusionsComponentInstallerPolicy::OnCustomUninstall() {}

void PlaylistExclusionsComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& path,
    base::DictValue manifest) {
  PlaylistExclusions::GetInstance()->LoadPlaylistExclusions(
      path.AppendASCII(kPlaylistExclusionsJsonFile), base::DoNothing());
}

bool PlaylistExclusionsComponentInstallerPolicy::VerifyInstallation(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) const {
  return base::PathExists(install_dir.AppendASCII(kPlaylistExclusionsJsonFile));
}

base::FilePath
PlaylistExclusionsComponentInstallerPolicy::GetRelativeInstallDir() const {
  return base::FilePath::FromUTF8Unsafe(kPlaylistExclusionsComponentId);
}

void PlaylistExclusionsComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(kPlaylistExclusionsComponentPublicKeySHA256.begin(),
               kPlaylistExclusionsComponentPublicKeySHA256.end());
}

std::string PlaylistExclusionsComponentInstallerPolicy::GetName() const {
  return kPlaylistExclusionsComponentName;
}

update_client::InstallerAttributes
PlaylistExclusionsComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

bool PlaylistExclusionsComponentInstallerPolicy::IsBraveComponent() const {
  return true;
}

}  // namespace

void MaybeRegisterPlaylistExclusionsComponent(
    component_updater::ComponentUpdateService* cus) {
  if (!base::FeatureList::IsEnabled(playlist::features::kPlaylist) || !cus) {
    return;
  }

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<PlaylistExclusionsComponentInstallerPolicy>());
  installer->Register(cus, base::BindOnce([]() {
                        BraveOnDemandUpdater::GetInstance()->EnsureInstalled(
                            kPlaylistExclusionsComponentId);
                      }));
}

}  // namespace playlist
