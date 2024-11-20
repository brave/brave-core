// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/youtube_script_injector/browser/core/youtube_component_installer.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/no_destructor.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/youtube_script_injector/browser/core/youtube_registry.h"
#include "brave/components/youtube_script_injector/common/features.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "crypto/sha2.h"

namespace youtube_script_injector {

namespace {

// Directory structure of YouTube Script Injector component:
// lhhcaamjbmbijmjbnnodjaknblkiagon/<component version>/
//  |_ manifest.json
//  |_ youtube.json
//  |_ scripts/
//    |_ keep-playing-audio.js
//    |_ fullscreen.js
// See youtube_json.cc for the format of youtube.json.

constexpr size_t kHashSize = 32;
constexpr char kYouTubeComponentName[] =
    "Brave YouTube Injector";
constexpr char kYouTubeComponentId[] = "lhhcaamjbmbijmjbnnodjaknblkiagon";
constexpr char kYouTubeComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAphUFFHyK+"
    "qUOXSw3OJXRQwKs79bt7zqnmkeFp/szXmmhj6/"
    "i4fmNiXVaxFuVOryM9OiaVxBIGHjN1BWYCQdylgbmgVTqLWpJAy/AAKEH9/"
    "Q68yWfQnN5sg1miNir+0I1SpCiT/Dx2N7s28WNnzD2e6/"
    "7Umx+zRXkRtoPX0xAecgUeyOZcrpZXJ4CG8dTJInhv7Fly/U8V/KZhm6ydKlibwsh2CB588/"
    "FlvQUzi5ZykXnPfzlsNLyyQ8fy6/+8hzSE5x4HTW5fy3TIRvmDi/"
    "7HmW+evvuMIPl1gtVe4HKOZ7G8UaznjXBfspszHU1fqTiZWeCPb53uemo1a+rdnSHXwIDAQAB";

}  // namespace

class YouTubeComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  YouTubeComponentInstallerPolicy();

  YouTubeComponentInstallerPolicy(const YouTubeComponentInstallerPolicy&) = delete;
  YouTubeComponentInstallerPolicy& operator=(const YouTubeComponentInstallerPolicy&) =
      delete;

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
  const std::string component_id_;
  const std::string component_name_;
  uint8_t component_hash_[kHashSize];
};

YouTubeComponentInstallerPolicy::YouTubeComponentInstallerPolicy()
    : component_id_(kYouTubeComponentId), component_name_(kYouTubeComponentName) {
  // Generate hash from public key.
  std::string decoded_public_key;
  base::Base64Decode(kYouTubeComponentBase64PublicKey, &decoded_public_key);
  crypto::SHA256HashString(decoded_public_key, component_hash_, kHashSize);
}

bool YouTubeComponentInstallerPolicy::SupportsGroupPolicyEnabledComponentUpdates()
    const {
  return true;
}

bool YouTubeComponentInstallerPolicy::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
YouTubeComponentInstallerPolicy::OnCustomInstall(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void YouTubeComponentInstallerPolicy::OnCustomUninstall() {}

void YouTubeComponentInstallerPolicy::ComponentReady(const base::Version& version,
                                                  const base::FilePath& path,
                                                  base::Value::Dict manifest) {
  YouTubeRegistry::GetInstance()->LoadScripts(path);
}

bool YouTubeComponentInstallerPolicy::VerifyInstallation(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

base::FilePath YouTubeComponentInstallerPolicy::GetRelativeInstallDir() const {
  return base::FilePath::FromUTF8Unsafe(component_id_);
}

void YouTubeComponentInstallerPolicy::GetHash(std::vector<uint8_t>* hash) const {
  hash->assign(component_hash_, UNSAFE_TODO(component_hash_ + kHashSize));
}

std::string YouTubeComponentInstallerPolicy::GetName() const {
  return component_name_;
}

update_client::InstallerAttributes
YouTubeComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

bool YouTubeComponentInstallerPolicy::IsBraveComponent() const {
  return true;
}

void RegisterYouTubeComponent(component_updater::ComponentUpdateService* cus) {
  if (!base::FeatureList::IsEnabled(youtube_script_injector::features::kBraveYouTubeScriptInjector) || !cus) {
    // In test, |cus| could be nullptr.
    return;
  }

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<YouTubeComponentInstallerPolicy>());
  installer->Register(
      // After Register, run the callback with component id.
      cus, base::BindOnce([]() {
        brave_component_updater::BraveOnDemandUpdater::GetInstance()
            ->EnsureInstalled(kYouTubeComponentId);
      }));
}

}  // namespace youtube_script_injector
