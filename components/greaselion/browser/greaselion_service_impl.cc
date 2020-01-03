/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/greaselion/browser/greaselion_service_impl.h"

#include <stddef.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_file_value_serializer.h"
#include "base/one_shot_event.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "base/values.h"
#include "brave/common/network_constants.h"
#include "brave/components/greaselion/browser/greaselion_download_service.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/common/chrome_paths.h"
#include "crypto/sha2.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/file_util.h"
#include "extensions/common/manifest_constants.h"
#include "url/gurl.h"

using extensions::Extension;
using extensions::Manifest;

namespace {

// Wraps a Greaselion rule in a component. The component is stored as an
// unpacked extension in the system temp dir. Returns a valid extension that the
// caller should take ownership of, or nullptr.
//
// NOTE: This function does file IO and should not be called on the UI thread.
// NOTE: The caller takes ownership of the directory at extension->path() on the
// returned object.
scoped_refptr<Extension> ConvertGreaselionRuleToExtensionOnTaskRunner(
    greaselion::GreaselionRule* rule,
    const base::FilePath& extensions_dir) {
  base::FilePath install_temp_dir =
      extensions::file_util::GetInstallTempDir(extensions_dir);
  if (install_temp_dir.empty()) {
    LOG(ERROR) << "Could not get path to profile temp directory";
    return nullptr;
  }

  base::ScopedTempDir temp_dir;
  if (!temp_dir.CreateUniqueTempDirUnderPath(install_temp_dir)) {
    LOG(ERROR) << "Could not create Greaselion temp directory";
    return nullptr;
  }

  // Create the manifest
  std::unique_ptr<base::DictionaryValue> root(new base::DictionaryValue);

  // manifest version is always 2
  // see kModernManifestVersion in src/extensions/common/extension.cc
  root->SetIntPath(extensions::manifest_keys::kManifestVersion, 2);

  // Create the public key.
  // Greaselion scripts are not signed, but the public key for an extension
  // doubles as its unique identity, and we need one of those, so we add the
  // rule name to a known Brave domain and hash the result to create a
  // public key.
  char raw[crypto::kSHA256Length] = {0};
  std::string key;
  std::string script_name = rule->name();
  crypto::SHA256HashString(kBraveUpdatesExtensionsEndpoint + script_name, raw,
                           crypto::kSHA256Length);
  base::Base64Encode(base::StringPiece(raw, crypto::kSHA256Length), &key);

  root->SetStringPath(extensions::manifest_keys::kName, script_name);
  root->SetStringPath(extensions::manifest_keys::kVersion, "1.0");
  root->SetStringPath(extensions::manifest_keys::kDescription, "");
  root->SetStringPath(extensions::manifest_keys::kPublicKey, key);

  auto js_files = std::make_unique<base::ListValue>();
  for (auto script : rule->scripts())
    js_files->AppendString(script.BaseName().value());

  auto matches = std::make_unique<base::ListValue>();
  for (auto url_pattern : rule->url_patterns())
    matches->AppendString(url_pattern);

  auto content_script = std::make_unique<base::DictionaryValue>();
  content_script->Set(extensions::manifest_keys::kMatches, std::move(matches));
  content_script->Set(extensions::manifest_keys::kJs, std::move(js_files));
  // All Greaselion scripts run at document end.
  content_script->SetStringPath(extensions::manifest_keys::kRunAt,
                                extensions::manifest_values::kRunAtDocumentEnd);

  auto content_scripts = std::make_unique<base::ListValue>();
  content_scripts->Append(std::move(content_script));

  root->Set(extensions::manifest_keys::kContentScripts,
            std::move(content_scripts));

  base::FilePath manifest_path =
      temp_dir.GetPath().Append(extensions::kManifestFilename);
  JSONFileValueSerializer serializer(manifest_path);
  // If you read the header file for this function, it says not to use it
  // outside unit tests because it writes to disk (which blocks the thread). I
  // just want to assure you that it's okay. We want to write to disk here, and
  // we're already on a task runner specifically for writing extension-related
  // files to disk.
  if (!serializer.Serialize(*root)) {
    LOG(ERROR) << "Could not write Greaselion manifest";
    return nullptr;
  }

  // Copy the script files to our extension directory.
  for (auto script : rule->scripts()) {
    if (!base::CopyFile(script, temp_dir.GetPath().Append(script.BaseName()))) {
      LOG(ERROR) << "Could not copy Greaselion script";
      return nullptr;
    }
  }

  std::string error;
  scoped_refptr<Extension> extension = extensions::file_util::LoadExtension(
      temp_dir.GetPath(), Manifest::COMPONENT, Extension::NO_FLAGS, &error);
  if (!extension.get()) {
    LOG(ERROR) << "Could not load Greaselion extension";
    LOG(ERROR) << error;
    return nullptr;
  }

  temp_dir.Take();  // The caller takes ownership of the directory.
  return extension;
}
}  // namespace

namespace greaselion {

GreaselionServiceImpl::GreaselionServiceImpl(
    GreaselionDownloadService* download_service,
    const base::FilePath& install_directory,
    extensions::ExtensionSystem* extension_system,
    extensions::ExtensionRegistry* extension_registry,
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : download_service_(download_service),
      install_directory_(install_directory),
      extension_system_(extension_system),
      extension_service_(extension_system->extension_service()),
      extension_registry_(extension_registry),
      all_rules_installed_successfully_(true),
      update_in_progress_(false),
      pending_installs_(0),
      task_runner_(std::move(task_runner)),
      weak_factory_(this) {
  extension_registry_->AddObserver(this);
  for (int i = FIRST_FEATURE; i != LAST_FEATURE; i++)
    state_[static_cast<GreaselionFeature>(i)] = false;
}

GreaselionServiceImpl::~GreaselionServiceImpl() {
  extension_registry_->RemoveObserver(this);
}

void GreaselionServiceImpl::UpdateInstalledExtensions() {
  if (update_in_progress_)
    return;
  update_in_progress_ = true;
  if (greaselion_extensions_.empty()) {
    // No Greaselion extensions are currently installed, so we can move on to
    // the install phase immediately.
    CreateAndInstallExtensions();
    return;
  }

  // Make a copy of greaselion_extensions_ to iterate while the original vector
  // changes.
  std::vector<extensions::ExtensionId> extensions = greaselion_extensions_;
  for (auto id : extensions) {
    // We need to unload all the Greaselion extensions that are already
    // installed. OnExtensionUnloaded will be called on each extension, where we
    // will update the greaselion_extensions_ set. Once it's empty, that
    // callback will call CreateAndInstallExtensions().
    extension_service_->UnloadExtension(
        id, extensions::UnloadedExtensionReason::UPDATE);
  }
}

void GreaselionServiceImpl::CreateAndInstallExtensions() {
  DCHECK(greaselion_extensions_.empty());
  DCHECK(update_in_progress_);
  all_rules_installed_successfully_ = true;
  pending_installs_ = 0;
  std::vector<std::unique_ptr<GreaselionRule>>* rules =
      download_service_->rules();
  for (const std::unique_ptr<GreaselionRule>& rule : *rules) {
    if (rule->Matches(state_)) {
      pending_installs_ += 1;
    }
  }
  if (!pending_installs_) {
    // no rules match, nothing else to do
    MaybeNotifyObservers();
    return;
  }
  for (const std::unique_ptr<GreaselionRule>& rule : *rules) {
    if (rule->Matches(state_)) {
      // Convert script file to component extension. This must run on extension
      // file task runner, which was passed in in the constructor.
      base::PostTaskAndReplyWithResult(
          task_runner_.get(), FROM_HERE,
          base::BindOnce(&ConvertGreaselionRuleToExtensionOnTaskRunner,
                         rule.get(), install_directory_),
          base::BindOnce(&GreaselionServiceImpl::PostConvert,
                         weak_factory_.GetWeakPtr()));
    }
  }
}

void GreaselionServiceImpl::PostConvert(
    scoped_refptr<extensions::Extension> extension) {
  if (!extension.get()) {
    all_rules_installed_successfully_ = false;
    pending_installs_ -= 1;
    MaybeNotifyObservers();
    LOG(ERROR) << "Could not load Greaselion script";
  } else {
    greaselion_extensions_.push_back(extension->id());
    extension_system_->ready().Post(
        FROM_HERE,
        base::BindOnce(&GreaselionServiceImpl::Install,
                       weak_factory_.GetWeakPtr(), base::Passed(&extension)));
  }
}

void GreaselionServiceImpl::Install(
    scoped_refptr<extensions::Extension> extension) {
  extension_service_->AddExtension(extension.get());
}

void GreaselionServiceImpl::OnExtensionReady(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension) {
  auto index = std::find(greaselion_extensions_.begin(),
                         greaselion_extensions_.end(), extension->id());
  if (index == greaselion_extensions_.end()) {
    // not one of ours
    return;
  }

  pending_installs_ -= 1;
  MaybeNotifyObservers();
}

void GreaselionServiceImpl::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UnloadedExtensionReason reason) {
  auto index = std::find(greaselion_extensions_.begin(),
                         greaselion_extensions_.end(), extension->id());
  if (index == greaselion_extensions_.end()) {
    // not one of ours
    return;
  }
  greaselion_extensions_.erase(index);
  if (update_in_progress_ && greaselion_extensions_.empty()) {
    // It's time!
    CreateAndInstallExtensions();
  }
}

void GreaselionServiceImpl::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void GreaselionServiceImpl::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void GreaselionServiceImpl::MaybeNotifyObservers() {
  if (!pending_installs_) {
    update_in_progress_ = false;
    for (Observer& observer : observers_)
      observer.OnExtensionsReady(this, all_rules_installed_successfully_);
  }
}

void GreaselionServiceImpl::SetFeatureEnabled(GreaselionFeature feature,
                                              bool enabled) {
  DCHECK(feature >= 0 && feature < LAST_FEATURE);
  state_[feature] = enabled;
  UpdateInstalledExtensions();
}

bool GreaselionServiceImpl::ready() {
  return !update_in_progress_;
}

}  // namespace greaselion
