/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/leo_local_models_updater.h"

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
    FILE_PATH_LITERAL("LeoLocalModels");
constexpr const char kComponentName[] = "Leo Local Models Updater";
constexpr const char kComponentId[] = "ejhejjmaoaohpghnblcdcjilndkangfe";
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

}  // namespace

constexpr const char kUniversalQAModelName[] =
    "universal_sentence_encoder_qa_with_metadata.tflite";

LeoLocalModelsComponentInstallerPolicy::
    LeoLocalModelsComponentInstallerPolicy() = default;
LeoLocalModelsComponentInstallerPolicy::
    ~LeoLocalModelsComponentInstallerPolicy() = default;

void LeoLocalModelsComponentInstallerPolicy::DeleteComponent() {
  base::DeletePathRecursively(GetComponentDir());
}

void LeoLocalModelsComponentInstallerPolicy::ComponentReadyForTesting(
    const base::Version& version,
    const base::FilePath& install_dir,
    base::Value::Dict manifest) {
  CHECK_IS_TEST();
  ComponentReady(version, install_dir, std::move(manifest));
}

bool LeoLocalModelsComponentInstallerPolicy::VerifyInstallation(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

bool LeoLocalModelsComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return false;
}

bool LeoLocalModelsComponentInstallerPolicy::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
LeoLocalModelsComponentInstallerPolicy::OnCustomInstall(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(update_client::InstallError::NONE);
}

void LeoLocalModelsComponentInstallerPolicy::OnCustomUninstall() {}

void LeoLocalModelsComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    base::Value::Dict manifest) {
  if (install_dir.empty()) {
    return;
  }
  LeoLocalModelsUpdaterState::GetInstance()->SetInstallDir(install_dir);
}

base::FilePath LeoLocalModelsComponentInstallerPolicy::GetRelativeInstallDir()
    const {
  return base::FilePath(kComponentInstallDir);
}

void LeoLocalModelsComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(kPublicKeySHA256,
               kPublicKeySHA256 + std::size(kPublicKeySHA256));
}

std::string LeoLocalModelsComponentInstallerPolicy::GetName() const {
  return kComponentName;
}

update_client::InstallerAttributes
LeoLocalModelsComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

bool LeoLocalModelsComponentInstallerPolicy::IsBraveComponent() const {
  return true;
}

LeoLocalModelsUpdaterState* LeoLocalModelsUpdaterState::GetInstance() {
  static base::NoDestructor<LeoLocalModelsUpdaterState> instance;
  return instance.get();
}

void LeoLocalModelsUpdaterState::SetInstallDir(
    const base::FilePath& install_dir) {
  if (install_dir.empty()) {
    return;
  }
  install_dir_ = install_dir;
  universal_qa_model_path_ = install_dir_.AppendASCII(kUniversalQAModelName);
}

const base::FilePath& LeoLocalModelsUpdaterState::GetInstallDir() const {
  return install_dir_;
}

const base::FilePath& LeoLocalModelsUpdaterState::GetUniversalQAModel() const {
  return universal_qa_model_path_;
}

LeoLocalModelsUpdaterState::LeoLocalModelsUpdaterState() = default;
LeoLocalModelsUpdaterState::~LeoLocalModelsUpdaterState() = default;

void ManageLeoLocalModelsComponentRegistration(
    component_updater::ComponentUpdateService* cus) {
  if (!ai_chat::features::IsAIChatEnabled() ||
      !ai_chat::features::IsPageContentRefineEnabled() || !cus) {
    LeoLocalModelsComponentInstallerPolicy::DeleteComponent();
    return;
  }

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<LeoLocalModelsComponentInstallerPolicy>());
  installer->Register(
      // After Register, run the callback with component id.
      cus, base::BindOnce([]() {
        brave_component_updater::BraveOnDemandUpdater::GetInstance()
            ->EnsureInstalled(kComponentId);
      }));
}

}  // namespace ai_chat
