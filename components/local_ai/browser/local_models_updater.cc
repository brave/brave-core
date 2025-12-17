/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/browser/local_models_updater.h"

#include <array>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/values.h"
#include "base/version.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/local_ai/common/features.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/component_updater/component_updater_service.h"
#include "components/update_client/update_client.h"
#include "crypto/sha2.h"

namespace local_ai {

namespace {
constexpr base::FilePath::CharType kComponentInstallDir[] =
    FILE_PATH_LITERAL("BraveLocalAIModels");
constexpr char kComponentName[] = "Brave Local AI Models Updater";
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

void DeleteComponentDirectory() {
  base::DeletePathRecursively(GetComponentDir());
}

}  // namespace

LocalModelsComponentInstallerPolicy::LocalModelsComponentInstallerPolicy() =
    default;
LocalModelsComponentInstallerPolicy::~LocalModelsComponentInstallerPolicy() =
    default;

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
  hash->assign(std::begin(kPublicKeySHA256), std::end(kPublicKeySHA256));
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

void LocalModelsUpdaterState::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);

  // If component is already ready, notify immediately
  if (!install_dir_.empty()) {
    observer->OnComponentReady(install_dir_);
  }
}

void LocalModelsUpdaterState::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void LocalModelsUpdaterState::SetInstallDir(const base::FilePath& install_dir) {
  if (install_dir.empty()) {
    return;
  }
  install_dir_ = install_dir;
  embeddinggemma_model_dir_ = install_dir_.AppendASCII(kEmbeddingGemmaModelDir);
  embeddinggemma_model_path_ =
      embeddinggemma_model_dir_.AppendASCII(kEmbeddingGemmaModelFile);
  embeddinggemma_dense1_path_ =
      embeddinggemma_model_dir_.AppendASCII(kEmbeddingGemmaDense1Dir)
          .AppendASCII(kEmbeddingGemmaDenseModelFile);
  embeddinggemma_dense2_path_ =
      embeddinggemma_model_dir_.AppendASCII(kEmbeddingGemmaDense2Dir)
          .AppendASCII(kEmbeddingGemmaDenseModelFile);
  embeddinggemma_config_path_ =
      embeddinggemma_model_dir_.AppendASCII(kEmbeddingGemmaConfigFile);
  embeddinggemma_tokenizer_path_ =
      embeddinggemma_model_dir_.AppendASCII(kEmbeddingGemmaTokenizerFile);

  // Notify all observers that the component is ready
  for (auto& observer : observers_) {
    observer.OnComponentReady(install_dir_);
  }
}

const base::FilePath& LocalModelsUpdaterState::GetInstallDir() const {
  return install_dir_;
}

const base::FilePath& LocalModelsUpdaterState::GetEmbeddingGemmaModelDir()
    const {
  return embeddinggemma_model_dir_;
}

const base::FilePath& LocalModelsUpdaterState::GetEmbeddingGemmaModel() const {
  return embeddinggemma_model_path_;
}

const base::FilePath& LocalModelsUpdaterState::GetEmbeddingGemmaDense1() const {
  return embeddinggemma_dense1_path_;
}

const base::FilePath& LocalModelsUpdaterState::GetEmbeddingGemmaDense2() const {
  return embeddinggemma_dense2_path_;
}

const base::FilePath& LocalModelsUpdaterState::GetEmbeddingGemmaConfig() const {
  return embeddinggemma_config_path_;
}

const base::FilePath& LocalModelsUpdaterState::GetEmbeddingGemmaTokenizer()
    const {
  return embeddinggemma_tokenizer_path_;
}

LocalModelsUpdaterState::LocalModelsUpdaterState() = default;
LocalModelsUpdaterState::~LocalModelsUpdaterState() = default;

void ManageLocalModelsComponentRegistration(
    component_updater::ComponentUpdateService* cus) {
  if (!base::FeatureList::IsEnabled(local_ai::features::kLocalAIModels) ||
      !cus) {
    DeleteComponentDirectory();
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

}  // namespace local_ai
