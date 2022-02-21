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
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_file_value_serializer.h"
#include "base/one_shot_event.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task/task_runner_util.h"
#include "base/values.h"
#include "base/version.h"
#include "brave/components/brave_component_updater/browser/features.h"
#include "brave/components/brave_component_updater/browser/switches.h"
#include "brave/components/greaselion/browser/greaselion_download_service.h"
#include "brave/components/version_info//version_info.h"
#include "chrome/browser/extensions/extension_service.h"
#include "components/version_info/version_info.h"
#include "crypto/sha2.h"
#include "extensions/browser/computed_hashes.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/api/content_scripts.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/file_util.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/mojom/manifest.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

using extensions::Extension;
using extensions::mojom::ManifestLocation;

namespace {

constexpr char kRunAtDocumentStart[] = "document_start";

bool ShouldComputeHashesForResource(
    const base::FilePath& relative_resource_path) {
  std::vector<base::FilePath::StringType> components =
      relative_resource_path.GetComponents();
  return !components.empty() && components[0] != extensions::kMetadataFolder;
}

// Wraps a Greaselion rule in a component. The component is stored as
// an unpacked extension in the user data dir. Returns a valid
// extension that the caller should take ownership of, or nullptr.
//
// NOTE: This function does file IO and should not be called on the UI thread.
// NOTE: The caller takes ownership of the directory at extension->path() on the
// returned object.
absl::optional<greaselion::GreaselionServiceImpl::GreaselionConvertedExtension>
ConvertGreaselionRuleToExtensionOnTaskRunner(
    const greaselion::GreaselionRule& rule,
    const base::FilePath& install_dir) {
  base::FilePath install_temp_dir =
      extensions::file_util::GetInstallTempDir(install_dir);
  if (install_temp_dir.empty()) {
    LOG(ERROR) << "Could not get path to profile temp directory";
    return absl::nullopt;
  }

  base::ScopedTempDir temp_dir;
  if (!temp_dir.CreateUniqueTempDirUnderPath(install_temp_dir)) {
    LOG(ERROR) << "Could not create Greaselion temp directory";
    return absl::nullopt;
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
  std::string script_name = rule.name();
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (!command_line.HasSwitch(brave_component_updater::kUseGoUpdateDev) &&
      !base::FeatureList::IsEnabled(
          brave_component_updater::kUseDevUpdaterUrl)) {
    crypto::SHA256HashString(UPDATER_DEV_ENDPOINT + script_name,
                             raw,
                             crypto::kSHA256Length);
  } else {
    crypto::SHA256HashString(UPDATER_PROD_ENDPOINT + script_name,
                             raw,
                             crypto::kSHA256Length);
  }
  base::Base64Encode(base::StringPiece(raw, crypto::kSHA256Length), &key);

  root->SetStringPath(extensions::manifest_keys::kName, script_name);
  root->SetStringPath(extensions::manifest_keys::kVersion, "1.0");
  root->SetStringPath(extensions::manifest_keys::kDescription, "");
  root->SetStringPath(extensions::manifest_keys::kPublicKey, key);
  root->SetStringPath("incognito",
                      extensions::manifest_values::kIncognitoNotAllowed);

  std::vector<std::string> matches;
  matches.reserve(rule.url_patterns().size());
  for (auto url_pattern : rule.url_patterns())
    matches.push_back(url_pattern);

  extensions::api::content_scripts::ContentScript content_script;
  content_script.matches = std::move(matches);

  content_script.js = std::make_unique<std::vector<std::string>>();
  for (auto script : rule.scripts())
    content_script.js->push_back(script.BaseName().AsUTF8Unsafe());

  // All Greaselion scripts default to document end.
  content_script.run_at =
      rule.run_at() == kRunAtDocumentStart
          ? extensions::api::content_scripts::RUN_AT_DOCUMENT_START
          : extensions::api::content_scripts::RUN_AT_DOCUMENT_END;

  if (!rule.messages().empty()) {
    root->SetStringPath(extensions::manifest_keys::kDefaultLocale, "en_US");
  }

  auto content_scripts = std::make_unique<base::ListValue>();
  content_scripts->Append(content_script.ToValue());

  root->Set(extensions::api::content_scripts::ManifestKeys::kContentScripts,
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
    return absl::nullopt;
  }

  // Copy the messages directory to our extension directory.
  if (!rule.messages().empty()) {
    if (!base::CopyDirectory(
            rule.messages(),
            temp_dir.GetPath().AppendASCII("_locales"), true)) {
      LOG(ERROR) << "Could not copy Greaselion messages directory at path: "
                 << rule.messages().LossyDisplayName();
      return absl::nullopt;
    }
  }

  // Copy the script files to our extension directory.
  for (auto script : rule.scripts()) {
    if (!base::CopyFile(script,
                        temp_dir.GetPath().Append(script.BaseName()))) {
      LOG(ERROR) << "Could not copy Greaselion script at path: "
          << script.LossyDisplayName();
      return absl::nullopt;
    }
  }

  std::string error;
  scoped_refptr<Extension> extension = extensions::file_util::LoadExtension(
      temp_dir.GetPath(), ManifestLocation::kComponent, Extension::NO_FLAGS,
      &error);
  if (!extension.get()) {
    LOG(ERROR) << "Could not load Greaselion extension";
    LOG(ERROR) << error;
    return absl::nullopt;
  }

  // Calculate and write computed hashes.
  absl::optional<extensions::ComputedHashes::Data> computed_hashes_data =
      extensions::ComputedHashes::Compute(
          extension->path(),
          extension_misc::kContentVerificationDefaultBlockSize,
          extensions::IsCancelledCallback(),
          base::BindRepeating(&ShouldComputeHashesForResource));
  if (computed_hashes_data) {
    extensions::ComputedHashes(std::move(*computed_hashes_data))
        .WriteToFile(
            extensions::file_util::GetComputedHashesPath(extension->path()));
  }

  // Take ownership of this temporary directory so it's deleted when
  // the service exits
  return std::make_pair(extension, std::move(temp_dir));
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
      update_pending_(false),
      pending_installs_(0),
      task_runner_(std::move(task_runner)),
      extension_dirs_(new std::vector<base::ScopedTempDir>,
                      base::OnTaskRunnerDeleter(task_runner_)),
      browser_version_(
          version_info::GetBraveVersionWithoutChromiumMajorVersion()),
      weak_factory_(this) {
  download_service_->AddObserver(this);
  extension_registry_->AddObserver(this);
  for (int i = FIRST_FEATURE; i != LAST_FEATURE; i++)
    state_[static_cast<GreaselionFeature>(i)] = false;
  // Static-value features
  state_[GreaselionFeature::SUPPORTS_MINIMUM_BRAVE_VERSION] = true;
}

GreaselionServiceImpl::~GreaselionServiceImpl() {
  download_service_->RemoveObserver(this);
  extension_registry_->RemoveObserver(this);
}

bool GreaselionServiceImpl::IsGreaselionExtension(const std::string& id) {
  return std::find(greaselion_extensions_.begin(), greaselion_extensions_.end(),
                   id) != greaselion_extensions_.end();
}

std::vector<extensions::ExtensionId>
GreaselionServiceImpl::GetExtensionIdsForTesting() {
  return greaselion_extensions_;
}

void GreaselionServiceImpl::UpdateInstalledExtensions() {
  if (update_in_progress_) {
    update_pending_ = true;
    return;
  }
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
    if (rule->Matches(state_, browser_version_) &&
        rule->has_unknown_preconditions() == false) {
      pending_installs_ += 1;
    }
  }
  if (!pending_installs_) {
    // no rules match, nothing else to do
    MaybeNotifyObservers();
    return;
  }
  for (const std::unique_ptr<GreaselionRule>& rule : *rules) {
    if (rule->Matches(state_, browser_version_) &&
        rule->has_unknown_preconditions() == false) {
      // Convert script file to component extension. This must run on extension
      // file task runner, which was passed in in the constructor.
      GreaselionRule rule_copy(*rule);
      base::PostTaskAndReplyWithResult(
          task_runner_.get(), FROM_HERE,
          base::BindOnce(&ConvertGreaselionRuleToExtensionOnTaskRunner,
                         rule_copy, install_directory_),
          base::BindOnce(&GreaselionServiceImpl::PostConvert,
                         weak_factory_.GetWeakPtr()));
    }
  }
}

void GreaselionServiceImpl::PostConvert(
    absl::optional<GreaselionConvertedExtension> converted_extension) {
  if (!converted_extension) {
    all_rules_installed_successfully_ = false;
    pending_installs_ -= 1;
    MaybeNotifyObservers();
    LOG(ERROR) << "Could not load Greaselion script";
  } else {
    greaselion_extensions_.push_back(converted_extension->first->id());
    extension_dirs_->push_back(std::move(converted_extension->second));
    extension_system_->ready().Post(
        FROM_HERE, base::BindOnce(&GreaselionServiceImpl::Install,
                                  weak_factory_.GetWeakPtr(),
                                  std::move(converted_extension->first)));
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

void GreaselionServiceImpl::AddObserver(GreaselionService::Observer* observer) {
  observers_.AddObserver(observer);
}

void GreaselionServiceImpl::RemoveObserver(
    GreaselionService::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void GreaselionServiceImpl::MaybeNotifyObservers() {
  if (!pending_installs_) {
    update_in_progress_ = false;
    if (update_pending_) {
      update_pending_ = false;
      UpdateInstalledExtensions();
    } else {
      for (auto& observer : observers_)
        observer.OnExtensionsReady(this, all_rules_installed_successfully_);
    }
  }
}

void GreaselionServiceImpl::OnRulesReady(
    GreaselionDownloadService* download_service) {
  for (auto& observer : observers_) {
    observer.OnRulesReady(this);
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

void GreaselionServiceImpl::SetBrowserVersionForTesting(
    const base::Version& version) {
  CHECK(version.IsValid());
  browser_version_ = version;
  UpdateInstalledExtensions();
}

}  // namespace greaselion
