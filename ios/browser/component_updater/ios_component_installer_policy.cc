/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/component_updater/ios_component_installer_policy.h"

#include <sys/qos.h>
#include <string>
#include "crypto/sha2.h"

#include "base/base64.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/json/json_string_value_serializer.h"
#include "base/task/thread_pool.h"

#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "components/crx_file/id_util.h"

using brave_component_updater::BraveComponent;

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
  if (!base::WriteFile(manifest_path, manifest_json)) {
    return false;
  }
  return true;
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

namespace component_updater {
// IOSComponentInstallerPolicy

IOSComponentInstallerPolicy::IOSComponentInstallerPolicy(
    const std::string& component_public_key,
    const std::string& component_name,
    BraveComponent::ReadyCallback callback)
    : component_name_(component_name),
      base64_public_key_(component_public_key),
      ready_callback_(callback) {
  base::Base64Decode(component_public_key, &public_key_);
}

IOSComponentInstallerPolicy::~IOSComponentInstallerPolicy() = default;

bool IOSComponentInstallerPolicy::SupportsGroupPolicyEnabledComponentUpdates()
    const {
  return true;
}

bool IOSComponentInstallerPolicy::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
IOSComponentInstallerPolicy::OnCustomInstall(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void IOSComponentInstallerPolicy::OnCustomUninstall() {}

void IOSComponentInstallerPolicy::ComponentReady(const base::Version& version,
                                                 const base::FilePath& path,
                                                 base::Value::Dict manifest) {
  ready_callback_.Run(path, GetManifestString(&manifest, base64_public_key_));
}

bool IOSComponentInstallerPolicy::VerifyInstallation(
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

base::FilePath IOSComponentInstallerPolicy::GetRelativeInstallDir() const {
  // Get the extension ID from the public key
  std::string extension_id = crx_file::id_util::GenerateId(public_key_);
  return base::FilePath(
      // Convert to wstring or string depending on OS
      base::FilePath::StringType(extension_id.begin(), extension_id.end()));
}

void IOSComponentInstallerPolicy::GetHash(std::vector<uint8_t>* hash) const {
  DCHECK(hash);
  const std::string public_key_sha256 = crypto::SHA256HashString(public_key_);
  hash->assign(public_key_sha256.begin(), public_key_sha256.end());
}

std::string IOSComponentInstallerPolicy::GetName() const {
  return component_name_;
}

update_client::InstallerAttributes
IOSComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}
}  // namespace component_updater
