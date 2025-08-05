// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/extensions/manifest_v2/brave_extensions_manifest_v2_migrator.h"

#include <utility>

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/string_util.h"
#include "brave/browser/extensions/manifest_v2/brave_hosted_extensions.h"
#include "brave/browser/extensions/manifest_v2/features.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/disable_reason.h"
#include "extensions/browser/extension_file_task_runner.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_prefs_factory.h"
#include "extensions/browser/extension_registrar.h"
#include "extensions/browser/extension_registrar_factory.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_registry_factory.h"
#include "extensions/common/constants.h"

namespace {

constexpr base::FilePath::CharType kExtensionMV2BackupDir[] =
    FILE_PATH_LITERAL("MV2Backup");

constexpr base::FilePath::CharType kIndexedDBDir[] =
    FILE_PATH_LITERAL("IndexedDB");

base::FilePath::StringType ASCIIToPathStringType(std::string_view str) {
  return base::FilePath::FromASCII(str).value();
}

base::FilePath GetLocalSettings(const extensions::ExtensionId& extension_id,
                                const base::FilePath& profile_dir) {
  const auto path =
      profile_dir.Append(extensions::kLocalExtensionSettingsDirectoryName)
          .AppendASCII(extension_id);
  return !base::IsDirectoryEmpty(path) ? path : base::FilePath();
}

base::FilePath GetLocalSettingsForImport(
    const extensions::ExtensionId& brave_hosted_extension_id,
    const base::FilePath& profile_dir) {
  CHECK(extensions_mv2::IsKnownBraveHostedExtension(brave_hosted_extension_id));
  const auto webstore_extension_id =
      extensions_mv2::GetWebStoreHostedExtensionId(brave_hosted_extension_id);
  if (!webstore_extension_id) {
    return {};
  }
  const auto backup =
      profile_dir.Append(kExtensionMV2BackupDir)
          .AppendASCII(*webstore_extension_id)
          .Append(extensions::kLocalExtensionSettingsDirectoryName)
          .AppendASCII(*webstore_extension_id);
  return !base::IsDirectoryEmpty(backup) ? backup : base::FilePath();
}

base::FileEnumerator GetIndexedSettings(
    const extensions::ExtensionId& extension_id,
    const base::FilePath& profile_dir) {
  const auto pattern = ASCIIToPathStringType(
      base::StrCat({"chrome-extension_", extension_id, "_*indexeddb*"}));
  return base::FileEnumerator(profile_dir.Append(kIndexedDBDir), false,
                              base::FileEnumerator::DIRECTORIES, pattern);
}

base::FileEnumerator GetIndexedSettingsForImport(
    const extensions::ExtensionId& brave_hosted_extension_id,
    const base::FilePath& profile_dir) {
  CHECK(extensions_mv2::IsKnownBraveHostedExtension(brave_hosted_extension_id));
  const auto webstore_extension_id =
      extensions_mv2::GetWebStoreHostedExtensionId(brave_hosted_extension_id);
  CHECK(webstore_extension_id);

  const auto pattern = ASCIIToPathStringType(base::StrCat(
      {"chrome-extension_", *webstore_extension_id, "_*indexeddb*"}));
  return base::FileEnumerator(profile_dir.Append(kExtensionMV2BackupDir)
                                  .AppendASCII(*webstore_extension_id)
                                  .Append(kIndexedDBDir),
                              false, base::FileEnumerator::DIRECTORIES,
                              pattern);
}

bool IsBackupAvailableFor(
    const extensions::ExtensionId& brave_hosted_extension_id,
    const base::FilePath& profile_dir) {
  CHECK(extensions_mv2::IsKnownBraveHostedExtension(brave_hosted_extension_id));

  const auto webstore_extension_id =
      extensions_mv2::GetWebStoreHostedExtensionId(brave_hosted_extension_id);
  if (!webstore_extension_id) {
    return false;
  }

  if (!base::PathExists(profile_dir.Append(kExtensionMV2BackupDir)
                            .AppendASCII(*webstore_extension_id)
                            .AppendASCII("version"))) {
    return false;
  }

  if (!GetLocalSettingsForImport(brave_hosted_extension_id, profile_dir)
           .empty()) {
    return true;
  }

  auto indexed =
      GetIndexedSettingsForImport(brave_hosted_extension_id, profile_dir);
  if (!indexed.Next().empty()) {
    return true;
  }
  return false;
}

base::Version GetBackupVersion(
    const extensions::ExtensionId& webstore_extension_id,
    const base::FilePath& profile_dir) {
  const auto version_path = profile_dir.Append(kExtensionMV2BackupDir)
                                .AppendASCII(webstore_extension_id)
                                .AppendASCII("version");
  if (std::string content;
      base::ReadFileToStringWithMaxSize(version_path, &content, 256)) {
    return base::Version(content);
  }
  return base::Version();
}

base::Version BackupExtensionSettingsOnFileThread(
    const extensions::ExtensionId& webstore_extension_id,
    const base::Version& version,
    const base::FilePath& profile_dir) {
  CHECK(extensions_mv2::IsKnownWebStoreHostedExtension(webstore_extension_id));

  const auto backup_path = profile_dir.Append(kExtensionMV2BackupDir)
                               .AppendASCII(webstore_extension_id);

  if (base::PathExists(backup_path.AppendASCII("version"))) {
    // We already have backup for this extension.
    return GetBackupVersion(webstore_extension_id, profile_dir);
  }

  base::ScopedClosureRunner remove_backup_on_error(
      base::GetDeletePathRecursivelyCallback(backup_path));

  const auto local_settings_path =
      profile_dir.Append(extensions::kLocalExtensionSettingsDirectoryName)
          .AppendASCII(webstore_extension_id);
  if (base::PathExists(local_settings_path)) {
    const auto local_settings_backup_path =
        backup_path.Append(extensions::kLocalExtensionSettingsDirectoryName);
    if (!base::DeletePathRecursively(local_settings_backup_path) ||
        !base::CreateDirectory(local_settings_backup_path) ||
        !base::CopyDirectory(local_settings_path, local_settings_backup_path,
                             true)) {
      return base::Version();
    }
  }

  const auto indexeddb_settings_backup_path = backup_path.Append(kIndexedDBDir);
  if (!base::CreateDirectory(indexeddb_settings_backup_path)) {
    return base::Version();
  }

  bool error = false;
  GetIndexedSettings(webstore_extension_id, profile_dir)
      .ForEach([&error,
                &indexeddb_settings_backup_path](const base::FilePath& path) {
        if (error) {
          return;
        }
        const auto destination =
            indexeddb_settings_backup_path.Append(path.BaseName());
        if (!base::DeletePathRecursively(destination) ||
            !base::CopyDirectory(path, destination, true)) {
          error = true;
        }
      });
  if (error || !base::WriteFile(backup_path.AppendASCII("version"),
                                version.GetString())) {
    return base::Version();
  }

  remove_backup_on_error.ReplaceClosure(base::DoNothing());
  return version;
}

bool ClearExtensionSettingsOnFileThread(
    const extensions::ExtensionId& brave_hosted_extension_id,
    const base::FilePath& profile_dir) {
  CHECK(extensions_mv2::IsKnownBraveHostedExtension(brave_hosted_extension_id));

  const auto local_settings =
      GetLocalSettings(brave_hosted_extension_id, profile_dir);
  if (!base::DeletePathRecursively(local_settings)) {
    return false;
  }

  auto indexeddb_settings =
      GetIndexedSettings(brave_hosted_extension_id, profile_dir);
  bool error = false;
  indexeddb_settings.ForEach([&error](const base::FilePath& path) {
    if (!error && !base::DeletePathRecursively(path)) {
      error = true;
    }
  });
  return !error;
}

void ImportExtensionSettingsOnFileThread(
    const extensions::ExtensionId& brave_hosted_extension_id,
    const base::Version& brave_hosted_extension_version,
    const base::FilePath& profile_dir) {
  if (!IsBackupAvailableFor(brave_hosted_extension_id, profile_dir)) {
    return;
  }

  const auto webstore_extension_id =
      *extensions_mv2::GetWebStoreHostedExtensionId(brave_hosted_extension_id);

  const auto webstore_extension_version =
      GetBackupVersion(webstore_extension_id, profile_dir);

  const auto backup_path = profile_dir.Append(kExtensionMV2BackupDir)
                               .AppendASCII(webstore_extension_id);
  // Remove backup in any case.
  base::ScopedClosureRunner remove_backup(
      base::GetDeletePathRecursivelyCallback(backup_path));

  if (webstore_extension_version != brave_hosted_extension_version) {
    return;
  }

  base::ScopedClosureRunner clear_settings_on_error(base::BindOnce(
      [](const extensions::ExtensionId& brave_hosted_extension_id,
         const base::FilePath& profile_dir) {
        ClearExtensionSettingsOnFileThread(brave_hosted_extension_id,
                                           profile_dir);
      },
      brave_hosted_extension_id, profile_dir));

  if (!ClearExtensionSettingsOnFileThread(brave_hosted_extension_id,
                                          profile_dir)) {
    return;
  }

  const auto local_settings_backup =
      GetLocalSettingsForImport(brave_hosted_extension_id, profile_dir);
  if (!local_settings_backup.empty()) {
    if (!base::Move(
            local_settings_backup,
            profile_dir.Append(extensions::kLocalExtensionSettingsDirectoryName)
                .AppendASCII(brave_hosted_extension_id))) {
      return;
    }
  }

  bool error = false;
  GetIndexedSettingsForImport(brave_hosted_extension_id, profile_dir)
      .ForEach([&error, &profile_dir, &brave_hosted_extension_id,
                &webstore_extension_id](const base::FilePath& path) {
        if (error) {
          return;
        }
        auto name = path.BaseName().value();
        base::ReplaceFirstSubstringAfterOffset(
            &name, 0, ASCIIToPathStringType(webstore_extension_id),
            ASCIIToPathStringType(brave_hosted_extension_id));
        if (!base::Move(path, profile_dir.Append(kIndexedDBDir).Append(name))) {
          error = true;
        }
      });
  if (!error) {
    clear_settings_on_error.ReplaceClosure(base::DoNothing());
  }
}

}  // namespace

namespace extensions_mv2 {

ExtensionsManifestV2Migrator::ExtensionsManifestV2Migrator(Profile* profile)
    : profile_(profile) {
  auto* registry = extensions::ExtensionRegistry::Get(profile_);
  auto* extension_prefs = extensions::ExtensionPrefs::Get(profile_);

  prefs_observation_.Observe(extension_prefs);
  registry_observation_.Observe(registry);

  for (const auto webstore_extension : kWebStoreHosted) {
    const auto disable_reasons = extension_prefs->GetDisableReasons(
        extensions::ExtensionId(webstore_extension.first));
    if (disable_reasons.contains(
            extensions::disable_reason::DISABLE_UNSUPPORTED_MANIFEST_VERSION)) {
      OnExtensionDisableReasonsChanged(
          extensions::ExtensionId(webstore_extension.first), disable_reasons);
    }
  }
}

ExtensionsManifestV2Migrator::~ExtensionsManifestV2Migrator() {
  CHECK(!prefs_observation_.IsObserving());
  CHECK(!registry_observation_.IsObserving());
}

void ExtensionsManifestV2Migrator::Shutdown() {
  prefs_observation_.Reset();
  registry_observation_.Reset();
}

void ExtensionsManifestV2Migrator::OnExtensionPrefsWillBeDestroyed(
    extensions::ExtensionPrefs* prefs) {
  prefs_observation_.Reset();
}

void ExtensionsManifestV2Migrator::OnExtensionDisableReasonsChanged(
    const extensions::ExtensionId& extension_id,
    extensions::DisableReasonSet disabled_reasons) {
  if (!features::IsSettingsBackupEnabled() ||
      !IsKnownWebStoreHostedExtension(extension_id) ||
      !disabled_reasons.contains(
          extensions::disable_reason::DISABLE_UNSUPPORTED_MANIFEST_VERSION)) {
    return;
  }

  BackupExtensionSettings(extension_id);
}

void ExtensionsManifestV2Migrator::OnShutdown(
    extensions::ExtensionRegistry* registry) {
  registry_observation_.Reset();
}

void ExtensionsManifestV2Migrator::OnExtensionInstalled(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    bool is_updates) {
  if (!features::IsSettingsImportEnabled()) {
    return;
  }
  if (is_updates ||
      !extensions_mv2::IsKnownBraveHostedExtension(extension->id())) {
    return;
  }

  extensions::ExtensionRegistrar::Get(profile_)->DisableExtension(
      extension->id(), {extensions::disable_reason::DISABLE_RELOAD});
  extensions::GetExtensionFileTaskRunner()->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&ImportExtensionSettingsOnFileThread, extension->id(),
                     extension->version(), profile_->GetPath()),
      base::BindOnce(&ExtensionsManifestV2Migrator::OnSettingsImported,
                     weak_factory_.GetWeakPtr(), extension->id()));
}

void ExtensionsManifestV2Migrator::BackupExtensionSettings(
    const extensions::ExtensionId& webstore_extension_id) {
  auto* prefs = extensions::ExtensionPrefs::Get(profile_);
  const base::Version webstore_extension_version(
      prefs->GetVersionString(webstore_extension_id));

  extensions::GetExtensionFileTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&BackupExtensionSettingsOnFileThread,
                     webstore_extension_id, webstore_extension_version,
                     profile_->GetPath()),
      base::BindOnce([](const base::Version&) {}));
}

void ExtensionsManifestV2Migrator::OnSettingsImported(
    const extensions::ExtensionId& brave_hosted_extension_id) {
  CHECK(extensions_mv2::IsKnownBraveHostedExtension(brave_hosted_extension_id));
  extensions::ExtensionRegistrar::Get(profile_)->EnableExtension(
      brave_hosted_extension_id);
}

//-----------------------------------------------------------------------------

ExtensionsManifestV2MigratorFactory::ExtensionsManifestV2MigratorFactory()
    : ProfileKeyedServiceFactory("ExtensionsManifestV2Migrator",
                                 ProfileSelections::BuildForRegularProfile()) {
  DependsOn(extensions::ExtensionPrefsFactory::GetInstance());
  DependsOn(extensions::ExtensionRegistryFactory::GetInstance());
  DependsOn(extensions::ExtensionRegistrarFactory::GetInstance());
}

ExtensionsManifestV2MigratorFactory::~ExtensionsManifestV2MigratorFactory() =
    default;

// static
ExtensionsManifestV2MigratorFactory*
ExtensionsManifestV2MigratorFactory::GetInstance() {
  return base::Singleton<ExtensionsManifestV2MigratorFactory>::get();
}

//  static
ExtensionsManifestV2Migrator*
ExtensionsManifestV2MigratorFactory::GetForBrowserContextForTesting(
    content::BrowserContext* context) {
  return static_cast<ExtensionsManifestV2Migrator*>(
      GetInstance()->GetServiceForBrowserContext(context, false));
}

std::unique_ptr<KeyedService>
ExtensionsManifestV2MigratorFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  if (!features::IsSettingsBackupEnabled() &&
      !features::IsSettingsImportEnabled() &&
      !features::IsExtensionReplacementEnabled()) {
    return nullptr;
  }
  return std::make_unique<ExtensionsManifestV2Migrator>(
      Profile::FromBrowserContext(context));
}

bool ExtensionsManifestV2MigratorFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

bool ExtensionsManifestV2MigratorFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

}  // namespace extensions_mv2
