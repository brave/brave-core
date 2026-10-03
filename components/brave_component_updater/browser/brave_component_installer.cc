/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_component_updater/browser/brave_component_installer.h"

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_string_value_serializer.h"
#include "base/values.h"
#include "base/version.h"
#include "components/component_updater/component_updater_service.h"
#include "components/crx_file/id_util.h"
#include "components/update_client/update_client.h"
#include "components/update_client/update_client_errors.h"

namespace {
using Result = update_client::CrxInstaller::Result;
using InstallError = update_client::InstallError;
}  // namespace

namespace {

std::string GetManifestString(const base::DictValue& manifest) {
  std::string manifest_json;
  JSONStringValueSerializer serializer(&manifest_json);
  serializer.set_pretty_print(true);
  if (!serializer.Serialize(manifest)) {
    return "";
  }
  return manifest_json;
}

}  // namespace

namespace brave_component_updater {

BraveComponentInstallerPolicy::BraveComponentInstallerPolicy(
    const std::string& name,
    base::span<const uint8_t, crypto::kSHA256Length> public_key_sha256,
    BraveComponent::ReadyCallback ready_callback)
    : name_(name), ready_callback_(std::move(ready_callback)) {
  base::span(public_key_sha256_).copy_from(public_key_sha256);
}

BraveComponentInstallerPolicy::~BraveComponentInstallerPolicy() = default;

bool BraveComponentInstallerPolicy::VerifyInstallation(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) const {
  return base::PathExists(
      install_dir.Append(FILE_PATH_LITERAL("manifest.json")));
}

bool BraveComponentInstallerPolicy::SupportsGroupPolicyEnabledComponentUpdates()
    const {
  return false;
}

bool BraveComponentInstallerPolicy::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
BraveComponentInstallerPolicy::OnCustomInstall(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) {
  return Result(InstallError::NONE);
}

void BraveComponentInstallerPolicy::OnCustomUninstall() {}

void BraveComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    base::DictValue manifest) {
  ready_callback_.Run(install_dir, GetManifestString(manifest));
}

base::FilePath BraveComponentInstallerPolicy::GetRelativeInstallDir() const {
  std::string extension_id =
      crx_file::id_util::GenerateIdFromHash(public_key_sha256_);
  return base::FilePath(
      // Convert to wstring or string depending on OS
      base::FilePath::StringType(extension_id.begin(), extension_id.end()));
}

void BraveComponentInstallerPolicy::GetHash(std::vector<uint8_t>* hash) const {
  hash->assign_range(public_key_sha256_);
}

std::string BraveComponentInstallerPolicy::GetName() const {
  return name_;
}

update_client::InstallerAttributes
BraveComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

bool BraveComponentInstallerPolicy::IsBraveComponent() const {
  return true;
}

}  // namespace brave_component_updater
