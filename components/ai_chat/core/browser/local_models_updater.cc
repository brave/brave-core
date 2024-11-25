/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/local_models_updater.h"

#include <memory>
#include <utility>

#include "base/check_is_test.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "components/component_updater/component_updater_paths.h"
#include "crypto/sha2.h"

namespace ai_chat {

namespace {
constexpr base::FilePath::CharType kComponentInstallDir[] =
    FILE_PATH_LITERAL("AIChatLocalModels");
constexpr base::FilePath::CharType kDeprecatedComponentInstallDir[] =
    FILE_PATH_LITERAL("LeoLocalModels");
constexpr char kComponentName[] = "Leo Local Models Updater";
constexpr char kComponentId[] = "ejhejjmaoaohpghnblcdcjilndkangfe";
constexpr uint8_t kPublicKeySHA256[32] = {
    0x49, 0x74, 0x99, 0xc0, 0xe0, 0xe7, 0xf6, 0x7d, 0x1b, 0x23, 0x29,
    0x8b, 0xd3, 0xa0, 0xd6, 0x54, 0xb6, 0xc3, 0x23, 0x87, 0x75, 0xec,
    0x54, 0x78, 0x1d, 0x83, 0xf4, 0xc3, 0xeb, 0x6d, 0x70, 0xb6};
static_assert(std::size(kPublicKeySHA256) == crypto::kSHA256Length,
              "Wrong hash length");

base::FilePath GetComponentDir() {
  base::FilePath components_dir =
      base::PathService::CheckedGet(component_updater::DIR_COMPONENT_USER);

  return components_dir.Append(kComponentInstallDir);
}

base::FilePath GetDeprecatedComponentDir() {
  base::FilePath components_dir =
      base::PathService::CheckedGet(component_updater::DIR_COMPONENT_USER);

  return components_dir.Append(kDeprecatedComponentInstallDir);
}

}  // namespace

LocalModelsComponentInstallerPolicy::
    LocalModelsComponentInstallerPolicy() = default;
LocalModelsComponentInstallerPolicy::
    ~LocalModelsComponentInstallerPolicy() = default;

void LocalModelsComponentInstallerPolicy::DeleteComponent() {
  base::DeletePathRecursively(GetComponentDir());
}

void LocalModelsComponentInstallerPolicy::ComponentReadyForTesting(
    const base::Version& version,
    const base::FilePath& install_dir,
    base::Value::Dict manifest) {
  CHECK_IS_TEST();
  ComponentReady(version, install_dir, std::move(manifest));
}

bool LocalModelsComponentInstallerPolicy::VerifyInstallation(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

bool LocalModelsComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return false;
}

bool LocalModelsComponentInstallerPolicy::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
LocalModelsComponentInstallerPolicy::OnCustomInstall(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(update_client::InstallError::NONE);
}

void LocalModelsComponentInstallerPolicy::OnCustomUninstall() {}

void LocalModelsComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    base::Value::Dict manifest) {
  if (install_dir.empty()) {
    return;
  }
  LocalModelsUpdaterState::GetInstance()->SetInstallDir(install_dir);
}

base::FilePath LocalModelsComponentInstallerPolicy::GetRelativeInstallDir()
    const {
  return base::FilePath(kComponentInstallDir);
}

void LocalModelsComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(kPublicKeySHA256,
               UNSAFE_TODO(kPublicKeySHA256 + std::size(kPublicKeySHA256)));
}

std::string LocalModelsComponentInstallerPolicy::GetName() const {
  return kComponentName;
}

update_client::InstallerAttributes
LocalModelsComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

bool LocalModelsComponentInstallerPolicy::IsBraveComponent() const {
  return true;
}

LocalModelsUpdaterState* LocalModelsUpdaterState::GetInstance() {
  static base::NoDestructor<LocalModelsUpdaterState> instance;
  return instance.get();
}

void LocalModelsUpdaterState::SetInstallDir(
    const base::FilePath& install_dir) {
  if (install_dir.empty()) {
    return;
  }
  install_dir_ = install_dir;
  universal_qa_model_path_ = install_dir_.AppendASCII(kUniversalQAModelName);
}

const base::FilePath& LocalModelsUpdaterState::GetInstallDir() const {
  return install_dir_;
}

const base::FilePath& LocalModelsUpdaterState::GetUniversalQAModel() const {
  return universal_qa_model_path_;
}

LocalModelsUpdaterState::LocalModelsUpdaterState() = default;
LocalModelsUpdaterState::~LocalModelsUpdaterState() = default;

void ManageLocalModelsComponentRegistration(
    component_updater::ComponentUpdateService* cus) {
  // Migrate deprecated component dir
  if (base::PathExists(GetDeprecatedComponentDir())) {
    base::DeletePathRecursively(GetDeprecatedComponentDir());
  }
  if (!ai_chat::features::IsAIChatEnabled() ||
      !ai_chat::features::IsPageContentRefineEnabled() || !cus) {
    LocalModelsComponentInstallerPolicy::DeleteComponent();
    return;
  }

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<LocalModelsComponentInstallerPolicy>());
  installer->Register(
      // After Register, run the callback with component id.
      cus, base::BindOnce([]() {
        brave_component_updater::BraveOnDemandUpdater::GetInstance()
            ->EnsureInstalled(kComponentId);
      }));
}

}  // namespace ai_chat
