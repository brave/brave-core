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
#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/path_service.h"
#include "base/values.h"
#include "base/version.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/component_updater/component_updater_service.h"
#include "components/update_client/update_client.h"
#include "crypto/sha2.h"

namespace local_ai {

namespace {
constexpr base::FilePath::CharType kComponentInstallDir[] =
    FILE_PATH_LITERAL("BraveOnDeviceSpeechModels");
constexpr char kComponentName[] = "Brave On-Device Speech Models";

// Placeholder public-key SHA256 (and the component id derived from its first
// 16 bytes) used while the feature is developed via local side-loading. It is
// NOT a real, provisioned Brave component, so on-demand network updates are
// intentionally not requested - see RegisterOnDeviceSpeechModelsComponent.
// Replace with the real key once Brave infra provisions the component.
constexpr uint8_t kPublicKeySHA256[32] = {
    0x53, 0x70, 0x65, 0x65, 0x63, 0x68, 0x4d, 0x6f, 0x64, 0x65, 0x6c,
    0x73, 0x44, 0x65, 0x76, 0x50, 0x6c, 0x61, 0x63, 0x65, 0x68, 0x6f,
    0x6c, 0x64, 0x65, 0x72, 0x4b, 0x65, 0x79, 0x30, 0x30, 0x31};
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
  // Register only: that schedules StartRegistration, which discovers any
  // locally-installed/side-loaded copy on disk and fires ComponentReady. We
  // do NOT call BraveOnDemandUpdater::EnsureInstalled because this component
  // is not yet provisioned for network delivery (placeholder key). Add the
  // EnsureInstalled(kComponentId) on-demand fetch here once a real key lands.
  installer->Register(cus, base::OnceClosure());
}

}  // namespace local_ai
