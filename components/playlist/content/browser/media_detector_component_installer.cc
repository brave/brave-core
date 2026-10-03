/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/media_detector_component_installer.h"

#include <iterator>
#include <memory>
#include <string>
#include <vector>

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
constexpr uint8_t kComponentPublicKeySHA256[32] = {
    0x92, 0x2f, 0xc9, 0x75, 0xb1, 0xbf, 0xf7, 0xd7, 0x64, 0xc7, 0xbb,
    0xb2, 0xa5, 0xbd, 0x8f, 0x9d, 0xf7, 0xa0, 0x02, 0x2d, 0x65, 0x2d,
    0x1e, 0x00, 0xa2, 0x43, 0xf3, 0x0a, 0x06, 0x2b, 0x40, 0x15};
static_assert(std::size(kComponentPublicKeySHA256) == crypto::kSHA256Length);

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
  OnComponentReadyCallback ready_callback_;
};

MediaDetectorComponentInstallerPolicy::MediaDetectorComponentInstallerPolicy(
    OnComponentReadyCallback callback)
    : ready_callback_(callback) {}

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
    const base::DictValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void MediaDetectorComponentInstallerPolicy::OnCustomUninstall() {}

void MediaDetectorComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& path,
    base::DictValue manifest) {
  ready_callback_.Run(path);
}

bool MediaDetectorComponentInstallerPolicy::VerifyInstallation(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

base::FilePath MediaDetectorComponentInstallerPolicy::GetRelativeInstallDir()
    const {
  return base::FilePath::FromUTF8Unsafe(kComponentID);
}

void MediaDetectorComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign_range(kComponentPublicKeySHA256);
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
  if (!cus ||
      BraveOnDemandUpdater::GetInstance()->is_component_update_disabled()) {
    return;
  }

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<MediaDetectorComponentInstallerPolicy>(callback));
  installer->Register(cus,
                      base::BindOnce(OnRegisteredToComponentUpdateService));
}

}  // namespace playlist
