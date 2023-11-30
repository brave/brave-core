// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_player/core/browser/brave_player_component_installer.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "brave/components/brave_player/core/browser/brave_player_service.h"
#include "brave/components/brave_player/core/common/features.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "crypto/sha2.h"

namespace brave_player {

namespace {

constexpr size_t kHashSize = 32;
const char kBravePlayerComponentName[] = "Brave Player Files";
const char kBravePlayerComponentId[] = "mgnejbocgjhepgaficdckaljcojnbeha";
const char kBravePlayerComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2DxY0UhAdz0JjOOZo+"
    "NH67etkNOyHsHA5nS+IUHZP3+zBzEYL9EHuxlOkEWsGwvUpluhdhR58HF+PxU/KHEivWjy/"
    "vV30k0ST98mO2Vp2hjoBXU5lluhoZJSDRaBQ6S2zwGZmIHGfQvE0bNJD2esZx0hRiZT79swcy/"
    "9NOq6nhxbaRwx//qWjaWsYg5RAP257XjpWpXTl+Ncg61zCDTrM/rHzDVKD+d6MODiGZ7Ytwf/"
    "95VDPN+"
    "XNmHlmcLh8ilNU4uK4qDFNcEPH0FqLvkM8NBWb4fKecl5OVUjIfzgkwnKaIatoSsPCkEotO2w+"
    "L6nwzsdgm0Tr+CqvHrCqTRo8QIDAQAB";

}  // namespace

class BraveViewerComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  explicit BraveViewerComponentInstallerPolicy(
      base::OnceCallback<void(const base::FilePath&)> on_ready);

  BraveViewerComponentInstallerPolicy(
      const BraveViewerComponentInstallerPolicy&) = delete;
  BraveViewerComponentInstallerPolicy& operator=(
      const BraveViewerComponentInstallerPolicy&) = delete;

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

 private:
  base::OnceCallback<void(const base::FilePath&)> on_ready_;

  const std::string component_id_;
  const std::string component_name_;
  uint8_t component_hash_[kHashSize];
};

BraveViewerComponentInstallerPolicy::BraveViewerComponentInstallerPolicy(
    base::OnceCallback<void(const base::FilePath&)> on_ready)
    : on_ready_(std::move(on_ready)),
      component_id_(kBravePlayerComponentId),
      component_name_(kBravePlayerComponentName) {
  // Generate Player from public key.
  std::string decoded_public_key;
  base::Base64Decode(kBravePlayerComponentBase64PublicKey, &decoded_public_key);
  crypto::SHA256HashString(decoded_public_key, component_hash_, kHashSize);
}

bool BraveViewerComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return true;
}

bool BraveViewerComponentInstallerPolicy::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
BraveViewerComponentInstallerPolicy::OnCustomInstall(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void BraveViewerComponentInstallerPolicy::OnCustomUninstall() {}

void BraveViewerComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& path,
    base::Value::Dict manifest) {
  std::move(on_ready_).Run(path);
}

bool BraveViewerComponentInstallerPolicy::VerifyInstallation(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

base::FilePath BraveViewerComponentInstallerPolicy::GetRelativeInstallDir()
    const {
  return base::FilePath::FromUTF8Unsafe(component_id_);
}

void BraveViewerComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(component_hash_, component_hash_ + kHashSize);
}

std::string BraveViewerComponentInstallerPolicy::GetName() const {
  return component_name_;
}

update_client::InstallerAttributes
BraveViewerComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

void RegisterBraveViewerComponent(
    component_updater::ComponentUpdateService* cus,
    base::OnceCallback<void(const base::FilePath&)> on_ready,
    base::OnceCallback<void(const std::string&)> on_registered) {
  if (!base::FeatureList::IsEnabled(brave_player::features::kBravePlayer) ||
      !cus) {
    // In test, |cus| could be nullptr.
    return;
  }

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<BraveViewerComponentInstallerPolicy>(
          std::move(on_ready)));
  installer->Register(
      cus, base::BindOnce(std::move(on_registered), kBravePlayerComponentId));
}

}  // namespace brave_player
