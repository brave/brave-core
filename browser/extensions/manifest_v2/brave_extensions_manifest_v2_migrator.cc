// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/extensions/manifest_v2/brave_extensions_manifest_v2_migrator.h"

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "brave/browser/extensions/manifest_v2/brave_extensions_manifest_v2_installer.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/extension_file_task_runner.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_prefs_factory.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"

namespace {

constexpr base::FilePath::CharType kExtensionMV2BackupDir[] =
    FILE_PATH_LITERAL("ExtensionsMV2Backup");

constexpr base::FilePath::CharType kIndexedDBDir[] =
    FILE_PATH_LITERAL("IndexedDB");

void BackupExtensionSettingsOnFileThread(
    const extensions::ExtensionId& extension_id,
    const base::FilePath& profile_dir) {
  const auto local_settings_path =
      profile_dir.Append(extensions::kLocalExtensionSettingsDirectoryName)
          .AppendASCII(extension_id);
  const auto backup_path = profile_dir.Append(kExtensionMV2BackupDir);
  if (base::PathExists(local_settings_path)) {
    const auto local_settings_backup_path =
        backup_path.Append(extensions::kLocalExtensionSettingsDirectoryName);
    base::CreateDirectory(local_settings_backup_path);
    base::CopyDirectory(local_settings_path, local_settings_backup_path, true);
  }

  const auto pattern = base::FilePath::FromASCII(
      base::StrCat({"chrome-extension_", extension_id, "_*indexeddb*"}));

  const auto indexeddb_settings_backup_path = backup_path.Append(kIndexedDBDir);
  base::CreateDirectory(indexeddb_settings_backup_path);

  base::FileEnumerator source_enumerator(
      profile_dir.Append(kIndexedDBDir), false,
      base::FileEnumerator::DIRECTORIES, pattern.value());
  source_enumerator.ForEach(
      [&indexeddb_settings_backup_path](const base::FilePath& path) {
        base::CopyDirectory(
            path, indexeddb_settings_backup_path.Append(path.BaseName()), true);
      });
}

}  // namespace

namespace extensions_mv2 {

ExtensionsManifectV2Migrator::ExtensionsManifectV2Migrator(Profile* profile)
    : profile_(profile) {
  observation_.Observe(extensions::ExtensionPrefs::Get(profile_));
}

ExtensionsManifectV2Migrator::~ExtensionsManifectV2Migrator() {
  CHECK(!observation_.IsObserving());
}

void ExtensionsManifectV2Migrator::Shutdown() {
  observation_.Reset();
}

void ExtensionsManifectV2Migrator::OnExtensionDisableReasonsChanged(
    const extensions::ExtensionId& extension_id,
    extensions::DisableReasonSet disabled_reasons) {
  if (!IsKnownCwsMV2Extension(extension_id)) {
    return;
  }
  if (!disabled_reasons.contains(
          extensions::disable_reason::DISABLE_UNSUPPORTED_MANIFEST_VERSION)) {
    return;
  }
  BackupExtensionSettings(extension_id);
}

void ExtensionsManifectV2Migrator::BackupExtensionSettings(
    const extensions::ExtensionId& extension_id) {
  extensions::GetExtensionFileTaskRunner()->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&BackupExtensionSettingsOnFileThread, extension_id,
                     profile_->GetPath()),
      base::DoNothing());
}

//-----------------------------------------------------------------------------

ExtensionsManifectV2MigratorFactory::ExtensionsManifectV2MigratorFactory()
    : ProfileKeyedServiceFactory("ExtensionsManifestV2Migrator",
                                 ProfileSelections::BuildForRegularProfile()) {
  DependsOn(extensions::ExtensionPrefsFactory::GetInstance());
}

ExtensionsManifectV2MigratorFactory::~ExtensionsManifectV2MigratorFactory() =
    default;

// static
ExtensionsManifectV2MigratorFactory*
ExtensionsManifectV2MigratorFactory::GetInstance() {
  return base::Singleton<ExtensionsManifectV2MigratorFactory>::get();
}

std::unique_ptr<KeyedService>
ExtensionsManifectV2MigratorFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
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
