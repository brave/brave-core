/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/on_device_speech_models_component_installer.h"

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
#include "base/path_service.h"
#include "base/values.h"
#include "base/version.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/component_updater/component_updater_service.h"
#include "components/update_client/update_client.h"
#include "components/update_client/update_client_errors.h"
#include "crypto/sha2.h"

namespace local_ai {

namespace {

constexpr base::FilePath::CharType kComponentInstallDir[] =
    FILE_PATH_LITERAL("BraveOnDeviceSpeechModels");
constexpr char kComponentName[] = "Brave On-Device Speech Models";
constexpr char kComponentId[] = "nhkekccefdppopbldokibkoegppanbba";

// SHA256 of the provisioned component's public key.
constexpr uint8_t kPublicKeySHA256[32] = {
    0xd7, 0xa4, 0xa2, 0x24, 0x53, 0xff, 0xef, 0x1b, 0x3e, 0xa8, 0x1a,
    0xe4, 0x6f, 0xf0, 0xd1, 0x10, 0xac, 0xaa, 0x39, 0x3b, 0x03, 0xcd,
    0xf1, 0x10, 0x04, 0x5e, 0xf9, 0x33, 0xf5, 0xe9, 0x6c, 0x4d};
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

OnDeviceSpeechModelsComponentInstallerPolicy::
    OnDeviceSpeechModelsComponentInstallerPolicy() = default;
OnDeviceSpeechModelsComponentInstallerPolicy::
    ~OnDeviceSpeechModelsComponentInstallerPolicy() = default;

bool OnDeviceSpeechModelsComponentInstallerPolicy::VerifyInstallation(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

bool OnDeviceSpeechModelsComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return false;
}

bool OnDeviceSpeechModelsComponentInstallerPolicy::RequiresNetworkEncryption()
    const {
  return false;
}

update_client::CrxInstaller::Result
OnDeviceSpeechModelsComponentInstallerPolicy::OnCustomInstall(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(update_client::InstallError::NONE);
}

void OnDeviceSpeechModelsComponentInstallerPolicy::OnCustomUninstall() {}

void OnDeviceSpeechModelsComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    base::DictValue manifest) {
  if (install_dir.empty()) {
    return;
  }
  OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(install_dir);
}

base::FilePath
OnDeviceSpeechModelsComponentInstallerPolicy::GetRelativeInstallDir() const {
  return base::FilePath(kComponentInstallDir);
}

void OnDeviceSpeechModelsComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(std::begin(kPublicKeySHA256), std::end(kPublicKeySHA256));
}

std::string OnDeviceSpeechModelsComponentInstallerPolicy::GetName() const {
  return kComponentName;
}

update_client::InstallerAttributes
OnDeviceSpeechModelsComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

bool OnDeviceSpeechModelsComponentInstallerPolicy::IsBraveComponent() const {
  return true;
}

void RegisterOnDeviceSpeechModelsComponent(
    component_updater::ComponentUpdateService* cus) {
  if (!base::FeatureList::IsEnabled(kBraveOnDeviceSpeechRecognition) || !cus) {
    DeleteComponentDirectory();
    return;
  }

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<OnDeviceSpeechModelsComponentInstallerPolicy>());
  installer->Register(
      cus, base::BindOnce([]() {
        brave_component_updater::BraveOnDemandUpdater::GetInstance()
            ->EnsureInstalled(kComponentId);
      }));
}

}  // namespace local_ai
