// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/extensions/manifest_v2/brave_extensions_manifest_v2_migrator.h"

#include <algorithm>
#include <utility>

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/browser/extensions/manifest_v2/brave_extensions_manifest_v2_installer.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/disable_reason.h"
#include "extensions/browser/extension_file_task_runner.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_prefs_factory.h"
#include "extensions/browser/extension_registrar.h"
#include "extensions/browser/extension_registrar_factory.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_registry_factory.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_features.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {

constexpr char kVersion[] = "version";

constexpr base::FilePath::CharType kExtensionMV2BackupDir[] =
    FILE_PATH_LITERAL("MV2Backup");

constexpr base::FilePath::CharType kIndexedDBDir[] =
    FILE_PATH_LITERAL("IndexedDB");

base::FilePath::StringType ASCIIToPathStringType(std::string_view str) {
  return base::FilePath::FromASCII(str).value();
}

base::FilePath GetLocalSettings(const extensions::ExtensionId& extension_id,
                                const base::FilePath& source_path) {
  const auto path =
      source_path.Append(extensions::kLocalExtensionSettingsDirectoryName)
          .AppendASCII(extension_id);
  return !base::IsDirectoryEmpty(path) ? path : base::FilePath();
}

base::FileEnumerator GetIndexedSettings(
    const extensions::ExtensionId& extension_id,
    const base::FilePath& source_path) {
  const auto pattern = ASCIIToPathStringType(
      base::StrCat({"chrome-extension_", extension_id, "_*indexeddb*"}));
  return base::FileEnumerator(source_path.Append(kIndexedDBDir),
                              /*recursive=*/false,
                              base::FileEnumerator::DIRECTORIES, pattern);
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

  const auto backup_path = profile_dir.Append(kExtensionMV2BackupDir)
                               .AppendASCII(*webstore_extension_id);

  if (!base::PathExists(backup_path.AppendASCII(kVersion))) {
    return false;
  }

  if (!GetLocalSettings(*webstore_extension_id, backup_path).empty()) {
    return true;
  }

  auto indexed = GetIndexedSettings(*webstore_extension_id, backup_path);
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
                                .AppendASCII(kVersion);
  if (std::string content;
      base::ReadFileToStringWithMaxSize(version_path, &content, 256)) {
    return base::Version(content);
  }
  return base::Version();
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

bool MoveExtensionSettings(const extensions::ExtensionId& source_extension_id,
                           const base::FilePath& source_dir,
                           const extensions::ExtensionId& target_extension_id,
                           const base::FilePath& target_dir,
                           bool delete_source) {
  const bool change_name = source_extension_id != target_extension_id;

  base::ScopedClosureRunner remove_source(
      (delete_source ? base::GetDeletePathRecursivelyCallback(source_dir)
                     : base::DoNothing()));

  const auto local_settings_source_path =
      source_dir.Append(extensions::kLocalExtensionSettingsDirectoryName)
          .AppendASCII(source_extension_id);
  if (base::PathExists(local_settings_source_path)) {
    const auto local_settings_target_path =
        target_dir.Append(extensions::kLocalExtensionSettingsDirectoryName)
            .AppendASCII(target_extension_id);
    if (!base::DeletePathRecursively(local_settings_target_path) ||
        !base::CreateDirectory(local_settings_target_path.DirName()) ||
        !base::CopyDirectory(local_settings_source_path,
                             local_settings_target_path,
                             /*recursive=*/true)) {
      return false;
    }
  }

  const auto indexeddb_settings_target_path = target_dir.Append(kIndexedDBDir);
  if (!base::CreateDirectory(indexeddb_settings_target_path)) {
    return false;
  }

  bool error = false;
  GetIndexedSettings(source_extension_id, source_dir)
      .ForEach([&error, change_name, &source_extension_id, &target_extension_id,
                &indexeddb_settings_target_path](const base::FilePath& path) {
        if (error) {
          return;
        }

        auto name = path.BaseName().value();
        if (change_name) {
          base::ReplaceFirstSubstringAfterOffset(
              &name, 0, ASCIIToPathStringType(source_extension_id),
              ASCIIToPathStringType(target_extension_id));
        }
        const auto destination = indexeddb_settings_target_path.Append(name);
        if (!base::DeletePathRecursively(destination) ||
            !base::CopyDirectory(path, destination, /*recursive=*/true)) {
          error = true;
        }
      });
  return error;
}

base::Version BackupExtensionSettingsOnFileThread(
    const extensions::ExtensionId& webstore_extension_id,
    const base::Version& version,
    const base::FilePath& profile_dir) {
  CHECK(extensions_mv2::IsKnownWebStoreHostedExtension(webstore_extension_id));

  const auto backup_path = profile_dir.Append(kExtensionMV2BackupDir)
                               .AppendASCII(webstore_extension_id);

  if (base::PathExists(backup_path.AppendASCII(kVersion))) {
    // We already have backup for this extension.
    return GetBackupVersion(webstore_extension_id, profile_dir);
  }

  base::ScopedClosureRunner remove_backup_on_error(
      base::GetDeletePathRecursivelyCallback(backup_path));

  const bool error =
      MoveExtensionSettings(webstore_extension_id, profile_dir,
                            webstore_extension_id, backup_path, false);

  if (error || !base::WriteFile(backup_path.AppendASCII(kVersion),
                                version.GetString())) {
    return base::Version();
  }

  remove_backup_on_error.ReplaceClosure(base::DoNothing());
  return version;
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

  const auto backup_path = profile_dir.Append(kExtensionMV2BackupDir)
                               .AppendASCII(webstore_extension_id);

  base::ScopedClosureRunner clear_settings_on_error(base::BindOnce(
      [](const extensions::ExtensionId& brave_hosted_extension_id,
         const base::FilePath& profile_dir) {
        ClearExtensionSettingsOnFileThread(brave_hosted_extension_id,
                                           profile_dir);
      },
      brave_hosted_extension_id, profile_dir));

  const bool error = MoveExtensionSettings(webstore_extension_id, backup_path,
                                           brave_hosted_extension_id,
                                           profile_dir, /*delete_source=*/true);

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

  // Since cr151 Brave re-allows MV2 extensions, Chromium no longer disables
  // known WebStore-hosted MV2 extensions with
  // DISABLE_UNSUPPORTED_MANIFEST_VERSION. Detect any that are already installed
  // by presence and start their migration. Extensions installed while running
  // are handled in OnExtensionInstalled(), and any that still get disabled for
  // the unsupported-manifest reason are handled in
  // OnExtensionDisableReasonsChanged().
  for (const auto webstore_extension : kWebStoreHosted) {
    const extensions::ExtensionId webstore_extension_id(
        webstore_extension.first);
    if (extension_prefs->GetInstalledExtensionInfo(webstore_extension_id)) {
      MaybeBackupWebStoreExtension(webstore_extension_id);
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
  if (!disabled_reasons.contains(
          extensions::disable_reason::DISABLE_UNSUPPORTED_MANIFEST_VERSION)) {
    return;
  }

  MaybeBackupWebStoreExtension(extension_id);
}

void ExtensionsManifestV2Migrator::OnShutdown(
    extensions::ExtensionRegistry* registry) {
  registry_observation_.Reset();
}

void ExtensionsManifestV2Migrator::OnExtensionInstalled(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    bool is_updates) {
  if (is_updates) {
    return;
  }

  // A known WebStore-hosted MV2 extension was just installed by the user. Back
  // up its settings and replace it with the Brave-hosted equivalent.
  if (extensions_mv2::IsKnownWebStoreHostedExtension(extension->id())) {
    MaybeBackupWebStoreExtension(extension->id());
    return;
  }

  // The Brave-hosted replacement finished installing. Import the backed-up
  // settings into it.
  if (!features::IsSettingsImportEnabled() ||
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

void ExtensionsManifestV2Migrator::MaybeBackupWebStoreExtension(
    const extensions::ExtensionId& webstore_extension_id) {
  if (!features::IsSettingsBackupEnabled() ||
      !IsKnownWebStoreHostedExtension(webstore_extension_id)) {
    return;
  }

  BackupExtensionSettings(webstore_extension_id);
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
      base::BindOnce(&ExtensionsManifestV2Migrator::OnBackupSettingsCompleted,
                     weak_factory_.GetWeakPtr(), webstore_extension_id));
}

void ExtensionsManifestV2Migrator::OnBackupSettingsCompleted(
    const extensions::ExtensionId& webstore_extension_id,
    const base::Version& backup_version) {
  if (!features::IsExtensionReplacementEnabled()) {
    return;
  }
  const auto brave_hosted_extension_id =
      GetBraveHostedExtensionId(webstore_extension_id);
  if (!brave_hosted_extension_id) {
    return;
  }
  auto* registry = extensions::ExtensionRegistry::Get(profile_);
  const auto* extension =
      registry->GetInstalledExtension(*brave_hosted_extension_id);
  if (extension) {
    // Already installed.
    return;
  }
  // The migration for a WebStore-hosted extension can be triggered more than
  // once (e.g. both on install and on disable), so a silent install for the
  // Brave-hosted equivalent may already be in flight. Don't start a duplicate.
  if (std::ranges::any_of(silent_installers_, [&brave_hosted_extension_id](
                                                  const auto& installer) {
        return installer->extension_id() == *brave_hosted_extension_id;
      })) {
    return;
  }
  auto installer = ExtensionManifestV2Installer::CreateSilent(
      *brave_hosted_extension_id, profile_, profile_->GetURLLoaderFactory(),
      base::BindOnce(&ExtensionsManifestV2Migrator::OnSilentInstall,
                     weak_factory_.GetWeakPtr(), *brave_hosted_extension_id));
  installer->BeginInstall();
  silent_installers_.push_back(std::move(installer));
}

void ExtensionsManifestV2Migrator::OnSettingsImported(
    const extensions::ExtensionId& brave_hosted_extension_id) {
  CHECK(extensions_mv2::IsKnownBraveHostedExtension(brave_hosted_extension_id));

  extensions::ExtensionRegistrar::Get(profile_)
      ->RemoveDisableReasonAndMaybeEnable(
          brave_hosted_extension_id,
          extensions::disable_reason::DISABLE_RELOAD);
}

void ExtensionsManifestV2Migrator::OnSilentInstall(
    const extensions::ExtensionId& extension_id,
    bool success,
    const std::string& error,
    extensions::webstore_install::Result result) {
  auto installer =
      std::ranges::find_if(silent_installers_, [&extension_id](const auto& i) {
        return i->extension_id() == extension_id;
      });
  if (installer != silent_installers_.end()) {
    silent_installers_.erase(installer);
  }

  if (success) {
    const auto webstore_extension_id =
        GetWebStoreHostedExtensionId(extension_id);
    // Only uninstall the WebStore-hosted extension if it's still installed. A
    // duplicate migration pass may have already removed it, and calling
    // UninstallExtension() for a missing extension hits a CHECK.
    if (webstore_extension_id &&
        extensions::ExtensionRegistry::Get(profile_)->GetInstalledExtension(
            *webstore_extension_id)) {
      extensions::ExtensionRegistrar::Get(profile_)->UninstallExtension(
          *webstore_extension_id,
          extensions::UninstallReason::UNINSTALL_REASON_INTERNAL_MANAGEMENT,
          nullptr);
    }
  }
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
