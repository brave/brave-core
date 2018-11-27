/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/component_updater/brave_component_installer.h"

#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_string_value_serializer.h"
#include "base/macros.h"
#include "base/values.h"
#include "base/version.h"
#include "components/component_updater/component_updater_service.h"
#include "components/crx_file/id_util.h"
#include "components/update_client/update_client.h"
#include "components/update_client/update_client_errors.h"
#include "components/update_client/utils.h"
#include "crypto/sha2.h"
#include "extensions/common/constants.h"
#include "extensions/common/manifest_constants.h"

namespace {
using Result = update_client::CrxInstaller::Result;
using InstallError = update_client::InstallError;
}  // namespace

namespace {
bool RewriteManifestFile(
    const base::FilePath& extension_root,
    const base::DictionaryValue& manifest,
    const std::string &public_key) {

  // Add the public key
  DCHECK(!public_key.empty());

  std::unique_ptr<base::DictionaryValue> final_manifest(manifest.DeepCopy());
  final_manifest->SetString(extensions::manifest_keys::kPublicKey, public_key);

  std::string manifest_json;
  JSONStringValueSerializer serializer(&manifest_json);
  serializer.set_pretty_print(true);
  if (!serializer.Serialize(*final_manifest)) {
    return false;
  }

  base::FilePath manifest_path =
    extension_root.Append(extensions::kManifestFilename);
  int size = base::checked_cast<int>(manifest_json.size());
  if (base::WriteFile(manifest_path, manifest_json.data(), size) != size) {
    return false;
  }
  return true;
}

std::string GetManifestString(const base::DictionaryValue& manifest,
    const std::string &public_key) {
  std::unique_ptr<base::DictionaryValue> final_manifest(manifest.DeepCopy());
  final_manifest->SetString(extensions::manifest_keys::kPublicKey, public_key);

  std::string manifest_json;
  JSONStringValueSerializer serializer(&manifest_json);
  serializer.set_pretty_print(true);
  if (!serializer.Serialize(*final_manifest)) {
    return "";
  }
  return manifest_json;
}


}  // namespace

namespace brave {

BraveComponentInstallerPolicy::BraveComponentInstallerPolicy(
    const std::string& name,
    const std::string& base64_public_key,
    const ReadyCallback& ready_callback)
    : name_(name),
      base64_public_key_(base64_public_key),
      ready_callback_(ready_callback) {
  base::Base64Decode(base64_public_key, &public_key_);
}

BraveComponentInstallerPolicy::~BraveComponentInstallerPolicy() {}

bool BraveComponentInstallerPolicy::VerifyInstallation(
    const base::DictionaryValue& manifest,
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

bool BraveComponentInstallerPolicy::SupportsGroupPolicyEnabledComponentUpdates() const {
  return false;
}

bool BraveComponentInstallerPolicy::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result BraveComponentInstallerPolicy::OnCustomInstall(
  const base::DictionaryValue& manifest,
  const base::FilePath& install_dir) {
  return Result(InstallError::NONE);
}

void BraveComponentInstallerPolicy::OnCustomUninstall() {
}

void BraveComponentInstallerPolicy::ComponentReady(
  const base::Version& version,
  const base::FilePath& install_dir,
  std::unique_ptr<base::DictionaryValue> manifest) {
  ready_callback_.Run(install_dir, GetManifestString(*manifest, base64_public_key_));
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

std::vector<std::string> BraveComponentInstallerPolicy::GetMimeTypes() const {
  return std::vector<std::string>();
}

update_client::InstallerAttributes BraveComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

void RegisterComponent(
    component_updater::ComponentUpdateService* cus,
    const std::string& name,
    const std::string& base64_public_key,
    const base::Closure& registered_callback,
    const ReadyCallback& ready_callback) {
  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
    std::make_unique<BraveComponentInstallerPolicy>(name, base64_public_key, ready_callback));
  installer->Register(cus, registered_callback);
}

}  // namespace brave
