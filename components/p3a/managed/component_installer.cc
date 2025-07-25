/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/component_installer.h"

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/p3a/managed/remote_config_manager.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"

using component_updater::ComponentUpdateService;

namespace p3a {

namespace {

constexpr char kP3AComponentName[] = "P3A Configuration";
constexpr char kP3AComponentId[] = "memmkfnkoigleebghlpgeiecaddoblcl";
constexpr base::FilePath::CharType kComponentInstallDir[] =
    FILE_PATH_LITERAL("P3AConfig");
// public key hash:
// c4cca5dae86b44167bf64842033e1b2b5c908bd6abc30e32d08b12bf50b2cc03
const uint8_t kP3AComponentHash[32] = {
    0xc4, 0xcc, 0xa5, 0xda, 0xe8, 0x6b, 0x44, 0x16, 0x7b, 0xf6, 0x48,
    0x42, 0x03, 0x3e, 0x1b, 0x2b, 0x5c, 0x90, 0x8b, 0xd6, 0xab, 0xc3,
    0x0e, 0x32, 0xd0, 0x8b, 0x12, 0xbf, 0x50, 0xb2, 0xcc, 0x03};

class P3AComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  explicit P3AComponentInstallerPolicy(
      base::WeakPtr<RemoteConfigManager> remote_config_manager)
      : remote_config_manager_(remote_config_manager) {}
  ~P3AComponentInstallerPolicy() override = default;

  P3AComponentInstallerPolicy(const P3AComponentInstallerPolicy&) = delete;
  P3AComponentInstallerPolicy& operator=(const P3AComponentInstallerPolicy&) =
      delete;

 private:
  // component_updater::ComponentInstallerPolicy:
  bool SupportsGroupPolicyEnabledComponentUpdates() const override {
    return false;
  }

  bool RequiresNetworkEncryption() const override { return false; }

  update_client::CrxInstaller::Result OnCustomInstall(
      const base::Value::Dict& manifest,
      const base::FilePath& install_dir) override {
    return update_client::CrxInstaller::Result(0);
  }

  void OnCustomUninstall() override {}

  bool VerifyInstallation(const base::Value::Dict& manifest,
                          const base::FilePath& install_dir) const override {
    return base::PathExists(install_dir.Append(kP3AManifestFileName));
  }

  void ComponentReady(const base::Version& version,
                      const base::FilePath& install_dir,
                      base::Value::Dict manifest) override {
    if (remote_config_manager_) {
      remote_config_manager_->LoadRemoteConfig(install_dir);
    }
  }

  base::FilePath GetRelativeInstallDir() const override {
    return base::FilePath(kComponentInstallDir);
  }

  void GetHash(std::vector<uint8_t>* hash) const override {
    hash->assign(std::begin(kP3AComponentHash), std::end(kP3AComponentHash));
  }

  std::string GetName() const override { return kP3AComponentName; }

  update_client::InstallerAttributes GetInstallerAttributes() const override {
    return update_client::InstallerAttributes();
  }

  bool IsBraveComponent() const override { return true; }

  base::WeakPtr<RemoteConfigManager> remote_config_manager_;
};

}  // namespace

void RegisterP3AComponent(
    ComponentUpdateService* cus,
    base::WeakPtr<RemoteConfigManager> remote_config_manager) {
  if (!cus || !remote_config_manager) {
    return;
  }

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<P3AComponentInstallerPolicy>(remote_config_manager));
  installer->Register(
      cus, base::BindOnce([]() {
        brave_component_updater::BraveOnDemandUpdater::GetInstance()
            ->EnsureInstalled(kP3AComponentId);
      }));
}

}  // namespace p3a
