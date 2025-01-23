/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_component_updater/browser/brave_component_installer.h"

#include <memory>
#include <utility>

#include "base/base64.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_string_value_serializer.h"
#include "base/values.h"
#include "base/version.h"
#include "components/component_updater/component_updater_service.h"
#include "components/crx_file/id_util.h"
#include "components/update_client/update_client.h"
#include "components/update_client/update_client_errors.h"
#include "crypto/sha2.h"

namespace {
using Result = update_client::CrxInstaller::Result;
using InstallError = update_client::InstallError;
}  // namespace

namespace {
bool RewriteManifestFile(const base::FilePath& extension_root,
                         const base::Value::Dict& manifest,
                         const std::string& public_key) {
  // Add the public key
  DCHECK(!public_key.empty());

  base::Value::Dict final_manifest = manifest.Clone();
  final_manifest.Set("key", public_key);

  std::string manifest_json;
  JSONStringValueSerializer serializer(&manifest_json);
  serializer.set_pretty_print(true);
  if (!serializer.Serialize(final_manifest)) {
    return false;
  }

  base::FilePath manifest_path =
      extension_root.Append(FILE_PATH_LITERAL("manifest.json"));
  return base::WriteFile(manifest_path, manifest_json);
}

std::string GetManifestString(base::Value::Dict* manifest,
                              const std::string& public_key) {
  manifest->Set("key", public_key);

  std::string manifest_json;
  JSONStringValueSerializer serializer(&manifest_json);
  serializer.set_pretty_print(true);
  if (!serializer.Serialize(*manifest)) {
    return "";
  }
  return manifest_json;
}

}  // namespace

namespace brave_component_updater {

BraveComponentInstallerPolicy::BraveComponentInstallerPolicy(
    const std::string& name,
    const std::string& base64_public_key,
    BraveComponent::ReadyCallback ready_callback)
    : name_(name),
      base64_public_key_(base64_public_key),
      ready_callback_(std::move(ready_callback)) {
  base::Base64Decode(base64_public_key, &public_key_);
}

BraveComponentInstallerPolicy::~BraveComponentInstallerPolicy() = default;

bool BraveComponentInstallerPolicy::VerifyInstallation(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) const {
  // The manifest file will generate a random ID if we don't provide one.
  // We want to write one with the actual extensions public key so we get
  // the same extensionID which is generated from the public key.
  if (!RewriteManifestFile(install_dir, manifest, base64_public_key_)) {
    return false;
  }
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
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) {
  return Result(InstallError::NONE);
}

void BraveComponentInstallerPolicy::OnCustomUninstall() {}

void BraveComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    base::Value::Dict manifest) {
  ready_callback_.Run(install_dir,
                      GetManifestString(&manifest, base64_public_key_));
}

base::FilePath BraveComponentInstallerPolicy::GetRelativeInstallDir() const {
  // Get the extension ID from the public key
  std::string extension_id = crx_file::id_util::GenerateId(public_key_);
  return base::FilePath(
      // Convert to wstring or string depending on OS
      base::FilePath::StringType(extension_id.begin(), extension_id.end()));
}

void BraveComponentInstallerPolicy::GetHash(std::vector<uint8_t>* hash) const {
  const std::string public_key_sha256 = crypto::SHA256HashString(public_key_);
  hash->assign(public_key_sha256.begin(), public_key_sha256.end());
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

void RegisterComponent(component_updater::ComponentUpdateService* cus,
                       const std::string& name,
                       const std::string& base64_public_key,
                       base::OnceClosure registered_callback,
                       BraveComponent::ReadyCallback ready_callback) {
  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<BraveComponentInstallerPolicy>(
          name, base64_public_key, std::move(ready_callback)));
  installer->Register(cus, std::move(registered_callback));
}

}  // namespace brave_component_updater
