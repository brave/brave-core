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
#include "brave/browser/extensions/manifest_v2/brave_extensions_manifest_v2_installer.h"
#include "brave/browser/extensions/manifest_v2/brave_hosted_extensions.h"
#include "brave/browser/extensions/manifest_v2/features.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/extension_file_task_runner.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_prefs_factory.h"
#include "extensions/browser/extension_registrar.h"
#include "extensions/browser/extension_registrar_factory.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_registry_factory.h"
#include "extensions/common/constants.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {

constexpr base::FilePath::CharType kExtensionMV2BackupDir[] =
    FILE_PATH_LITERAL("ExtensionsMV2Backup");

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
  CHECK(extensions_mv2::IsKnownMV2Extension(brave_hosted_extension_id));
  const auto cws_extensions_id =
      extensions_mv2::GetCwsExtensionId(brave_hosted_extension_id);
  if (!cws_extensions_id) {
    return {};
  }
  const auto backup =
      profile_dir.Append(kExtensionMV2BackupDir)
          .Append(extensions::kLocalExtensionSettingsDirectoryName)
          .AppendASCII(*cws_extensions_id);
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
  CHECK(extensions_mv2::IsKnownMV2Extension(brave_hosted_extension_id));
  const auto cws_extension_id =
      extensions_mv2::GetCwsExtensionId(brave_hosted_extension_id);
  CHECK(cws_extension_id);

  const auto pattern = ASCIIToPathStringType(
      base::StrCat({"chrome-extension_", *cws_extension_id, "_*indexeddb*"}));
  return base::FileEnumerator(
      profile_dir.Append(kExtensionMV2BackupDir).Append(kIndexedDBDir), false,
      base::FileEnumerator::DIRECTORIES, pattern);
}

bool IsBackupAvailableFor(
    const extensions::ExtensionId& brave_hosted_extension_id,
    const base::FilePath& profile_dir) {
  CHECK(extensions_mv2::IsKnownMV2Extension(brave_hosted_extension_id));

  const auto cws_extension_id =
      extensions_mv2::GetCwsExtensionId(brave_hosted_extension_id);
  if (!cws_extension_id) {
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

void BackupExtensionSettingsOnFileThread(
    const extensions::ExtensionId& cws_extension_id,
    const base::FilePath& profile_dir) {
  CHECK(extensions_mv2::IsKnownCwsMV2Extension(cws_extension_id));

  const auto local_settings_path =
      profile_dir.Append(extensions::kLocalExtensionSettingsDirectoryName)
          .AppendASCII(cws_extension_id);
  const auto backup_path = profile_dir.Append(kExtensionMV2BackupDir);
  if (base::PathExists(local_settings_path)) {
    const auto local_settings_backup_path =
        backup_path.Append(extensions::kLocalExtensionSettingsDirectoryName);
    base::DeletePathRecursively(local_settings_backup_path);
    base::CreateDirectory(local_settings_backup_path);
    base::CopyDirectory(local_settings_path, local_settings_backup_path, true);
  }

  const auto indexeddb_settings_backup_path = backup_path.Append(kIndexedDBDir);
  base::CreateDirectory(indexeddb_settings_backup_path);

  GetIndexedSettings(cws_extension_id, profile_dir)
      .ForEach([&indexeddb_settings_backup_path](const base::FilePath& path) {
        const auto destination =
            indexeddb_settings_backup_path.Append(path.BaseName());
        base::DeletePathRecursively(destination);
        base::CopyDirectory(path, destination, true);
      });
}

void ClearExtensionSettingsOnFileThread(
    const extensions::ExtensionId& brave_hosted_extension_id,
    const base::FilePath& profile_dir) {
  CHECK(extensions_mv2::IsKnownMV2Extension(brave_hosted_extension_id));

  const auto local_settings =
      GetLocalSettings(brave_hosted_extension_id, profile_dir);
  base::DeletePathRecursively(local_settings);

  auto indexeddb_settings =
      GetIndexedSettings(brave_hosted_extension_id, profile_dir);
  indexeddb_settings.ForEach(
      [](const base::FilePath& path) { base::DeletePathRecursively(path); });
}

void ImportExtensionSettingsOnFileThread(
    const extensions::ExtensionId& brave_hosted_extension_id,
    const base::FilePath& profile_dir) {
  if (!IsBackupAvailableFor(brave_hosted_extension_id, profile_dir)) {
    return;
  }

  ClearExtensionSettingsOnFileThread(brave_hosted_extension_id, profile_dir);

  const auto local_settings_backup =
      GetLocalSettingsForImport(brave_hosted_extension_id, profile_dir);
  if (!local_settings_backup.empty()) {
    base::Move(
        local_settings_backup,
        profile_dir.Append(extensions::kLocalExtensionSettingsDirectoryName)
            .AppendASCII(brave_hosted_extension_id));
  }

  const auto cws_extension_id =
      *extensions_mv2::GetCwsExtensionId(brave_hosted_extension_id);

  GetIndexedSettingsForImport(brave_hosted_extension_id, profile_dir)
      .ForEach([&profile_dir, &brave_hosted_extension_id,
                &cws_extension_id](const base::FilePath& path) {
        auto name = path.BaseName().value();
        base::ReplaceFirstSubstringAfterOffset(
            &name, 0, ASCIIToPathStringType(cws_extension_id),
            ASCIIToPathStringType(brave_hosted_extension_id));
        base::Move(path, profile_dir.Append(kIndexedDBDir).Append(name));
      });
}

}  // namespace

namespace extensions_mv2 {

ExtensionsManifectV2Migrator::ExtensionsManifectV2Migrator(Profile* profile)
    : profile_(profile) {
  auto* registry = extensions::ExtensionRegistry::Get(profile_);
  auto* extension_prefs = extensions::ExtensionPrefs::Get(profile_);

  prefs_observation_.Observe(extension_prefs);
  registry_observation_.Observe(registry);

  for (const auto cws_extension : kCwsHosted) {
    const auto disable_reasons = extension_prefs->GetDisableReasons(
        extensions::ExtensionId(cws_extension.first));
    if (disable_reasons.contains(
            extensions::disable_reason::DISABLE_UNSUPPORTED_MANIFEST_VERSION)) {
      OnExtensionDisableReasonsChanged(
          extensions::ExtensionId(cws_extension.first), disable_reasons);
    }
  }
}

ExtensionsManifectV2Migrator::~ExtensionsManifectV2Migrator() {
  CHECK(!prefs_observation_.IsObserving());
  CHECK(!registry_observation_.IsObserving());
}

void ExtensionsManifectV2Migrator::Shutdown() {
  prefs_observation_.Reset();
  registry_observation_.Reset();
}

void ExtensionsManifectV2Migrator::OnExtensionPrefsWillBeDestroyed(
    extensions::ExtensionPrefs* prefs) {
  prefs_observation_.Reset();
}

void ExtensionsManifectV2Migrator::OnExtensionDisableReasonsChanged(
    const extensions::ExtensionId& extension_id,
    extensions::DisableReasonSet disabled_reasons) {
  if (!features::IsSettingsBackupEnabled() ||
      !IsKnownCwsMV2Extension(extension_id)) {
    return;
  }
  if (!disabled_reasons.contains(
          extensions::disable_reason::DISABLE_UNSUPPORTED_MANIFEST_VERSION)) {
    return;
  }

  BackupExtensionSettings(extension_id);
}

void ExtensionsManifectV2Migrator::OnShutdown(
    extensions::ExtensionRegistry* registry) {
  registry_observation_.Reset();
}

void ExtensionsManifectV2Migrator::OnExtensionInstalled(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    bool is_updates) {
  if (!features::IsSettingsImportEnabled()) {
    return;
  }
  if (is_updates || !extensions_mv2::IsKnownMV2Extension(extension->id())) {
    return;
  }
  extensions::GetExtensionFileTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&ImportExtensionSettingsOnFileThread,
                                extension->id(), profile_->GetPath()));
}

void ExtensionsManifectV2Migrator::BackupExtensionSettings(
    const extensions::ExtensionId& cws_extension_id) {
  extensions::GetExtensionFileTaskRunner()->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&BackupExtensionSettingsOnFileThread, cws_extension_id,
                     profile_->GetPath()),
      base::BindOnce(&ExtensionsManifectV2Migrator::OnBackupSettingsCompleted,
                     weak_factory_.GetWeakPtr(), cws_extension_id));
}

void ExtensionsManifectV2Migrator::OnBackupSettingsCompleted(
    const extensions::ExtensionId& cws_extension_id) {
  if (!features::IsExtensionReplacementEnabled()) {
    return;
  }
  const auto brave_hosted_extesnion_id =
      GetBraveHostedExtensionId(cws_extension_id);
  if (!brave_hosted_extesnion_id) {
    return;
  }
  auto* registry = extensions::ExtensionRegistry::Get(profile_);
  const auto* extension =
      registry->GetInstalledExtension(*brave_hosted_extesnion_id);
  if (extension) {
    // Already installed.
    return;
  }
  auto installer = ExtensionManifestV2Installer::CreateSilent(
      *brave_hosted_extesnion_id, profile_, profile_->GetURLLoaderFactory(),
      base::BindOnce(&ExtensionsManifectV2Migrator::OnSilentInstall,
                     weak_factory_.GetWeakPtr(), *brave_hosted_extesnion_id));
  installer->BeginInstall();
  silent_installers_.push_back(std::move(installer));
}

void ExtensionsManifectV2Migrator::OnSilentInstall(
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
    const auto cws_extension_id = GetCwsExtensionId(extension_id);
    if (cws_extension_id) {
      extensions::ExtensionRegistrar::Get(profile_)->UninstallExtension(
          *cws_extension_id,
          extensions::UninstallReason::UNINSTALL_REASON_INTERNAL_MANAGEMENT,
          nullptr);
    }
  }
}

//-----------------------------------------------------------------------------

ExtensionsManifectV2MigratorFactory::ExtensionsManifectV2MigratorFactory()
    : ProfileKeyedServiceFactory("ExtensionsManifestV2Migrator",
                                 ProfileSelections::BuildForRegularProfile()) {
  DependsOn(extensions::ExtensionPrefsFactory::GetInstance());
  DependsOn(extensions::ExtensionRegistryFactory::GetInstance());
  DependsOn(extensions::ExtensionRegistrarFactory::GetInstance());
}

ExtensionsManifectV2MigratorFactory::~ExtensionsManifectV2MigratorFactory() =
    default;

// static
ExtensionsManifectV2MigratorFactory*
ExtensionsManifectV2MigratorFactory::GetInstance() {
  return base::Singleton<ExtensionsManifectV2MigratorFactory>::get();
}

//  static
ExtensionsManifectV2Migrator*
ExtensionsManifectV2MigratorFactory::GetForBrowserContextForTesting(
    content::BrowserContext* context) {
  return static_cast<ExtensionsManifectV2Migrator*>(
      GetInstance()->GetServiceForBrowserContext(context, false));
}

std::unique_ptr<KeyedService>
ExtensionsManifectV2MigratorFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  if (!features::IsSettingsBackupEnabled() &&
      !features::IsSettingsImportEnabled() &&
      !features::IsExtensionReplacementEnabled()) {
    return nullptr;
  }
  return std::make_unique<ExtensionsManifectV2Migrator>(
      Profile::FromBrowserContext(context));
}

bool ExtensionsManifectV2MigratorFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

bool ExtensionsManifectV2MigratorFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

}  // namespace extensions_mv2
