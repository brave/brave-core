/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/extensions_import_helpers.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/common/importer/chrome_importer_utils.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/webstore_install_with_prompt.h"
#include "chrome/browser/profiles/profile.h"
#include "components/value_store/value_store.h"
#include "components/value_store/value_store_factory.h"
#include "components/value_store/value_store_factory_impl.h"
#include "extensions/browser/api/storage/value_store_util.h"
#include "extensions/browser/extension_file_task_runner.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"

namespace extensions_import {

// Silent installer via websotre w/o any prompt or bubble.
class WebstoreInstallerForImporting
    : public extensions::WebstoreInstallWithPrompt {
 public:
  using WebstoreInstallWithPrompt::AbortInstall;
  using WebstoreInstallWithPrompt::WebstoreInstallWithPrompt;

 private:
  ~WebstoreInstallerForImporting() override = default;

  std::unique_ptr<ExtensionInstallPrompt::Prompt> CreateInstallPrompt()
      const override {
    return nullptr;
  }
  bool ShouldShowPostInstallUI() const override { return false; }
};

base::expected<std::vector<ImportingExtension>, bool> GetExtensionsList(
    const base::FilePath& source_profile,
    const base::FilePath& target_profile) {
  std::vector<ImportingExtension> result;
  const auto extensions = GetImportableChromeExtensionsList(source_profile);
  if (!extensions || extensions->empty()) {
    return base::ok(std::move(result));
  }

  auto source_store_factory =
      base::MakeRefCounted<value_store::ValueStoreFactoryImpl>(source_profile);

  for (const auto& extension_id : *extensions) {
    const auto value_store =
        base::FilePath(extensions::kLocalExtensionSettingsDirectoryName)
            .AppendASCII(extension_id);

    bool has_local_settings = false;

    if (source_store_factory->HasValueStore(
            base::FilePath(extensions::kLocalExtensionSettingsDirectoryName)
                .AppendASCII(extension_id))) {
      auto store = extensions::value_store_util::CreateSettingsStore(
          extensions::settings_namespace::LOCAL,
          extensions::value_store_util::ModelType::EXTENSION, extension_id,
          source_store_factory);
      const auto settings = store->Get();
      if (!settings.status().ok()) {
        if (!settings.status().IsCorrupted()) {
          return base::unexpected(false);
        }
      } else {
        has_local_settings = true;
      }
    }
    ImportingExtension e;
    e.id = extension_id;
    e.has_local_settings = has_local_settings;
    result.push_back(std::move(e));
  }

  return base::ok(std::move(result));
}

bool ImportLocalExtensionSettings(const std::string& extension_id,
                                  const base::FilePath& source_profile,
                                  const base::FilePath& target_profile) {
  auto source_store_factory =
      base::MakeRefCounted<value_store::ValueStoreFactoryImpl>(source_profile);
  auto source_store = extensions::value_store_util::CreateSettingsStore(
      extensions::settings_namespace::LOCAL,
      extensions::value_store_util::ModelType::EXTENSION, extension_id,
      source_store_factory);

  auto settings = source_store->Get();
  if (!settings.status().ok()) {
    return false;
  }

  auto target_store_factory =
      base::MakeRefCounted<value_store::ValueStoreFactoryImpl>(target_profile);

  extensions::value_store_util::DeleteValueStore(
      extensions::settings_namespace::LOCAL,
      extensions::value_store_util::ModelType::EXTENSION, extension_id,
      target_store_factory);

  auto target_store = extensions::value_store_util::CreateSettingsStore(
      extensions::settings_namespace::LOCAL,
      extensions::value_store_util::ModelType::EXTENSION, extension_id,
      target_store_factory);

  const auto result = target_store->Set(value_store::ValueStore::DEFAULTS,
                                        settings.PassSettings());
  return result.status().ok();
}

bool ImportIndexedDBExtensionSettings(const std::string& extension_id,
                                      const base::FilePath& source_profile,
                                      const base::FilePath& target_profile) {
  constexpr char kIndexedDBDir[] = "IndexedDB";

  const base::FilePath pattern = base::FilePath::FromASCII(
      base::StrCat({"chrome-extension_", extension_id, "_*indexeddb*"}));

  base::FileEnumerator target_enumerator(
      target_profile.AppendASCII(kIndexedDBDir), false,
      base::FileEnumerator::DIRECTORIES, pattern.value());
  // Clear the target profile.
  target_enumerator.ForEach(
      [](const base::FilePath& path) { base::DeletePathRecursively(path); });

  base::FileEnumerator source_enumerator(
      source_profile.AppendASCII(kIndexedDBDir), false,
      base::FileEnumerator::DIRECTORIES, pattern.value());
  // Copy settings.
  source_enumerator.ForEach([&target_profile,
                             &kIndexedDBDir](const base::FilePath& path) {
    base::CopyDirectory(
        path, target_profile.AppendASCII(kIndexedDBDir).Append(path.BaseName()),
        true);
  });

  return true;
}

bool ImportExtensionSettings(const std::string& extension_id,
                             const base::FilePath& source_profile,
                             const base::FilePath& target_profile) {
  if (!ImportLocalExtensionSettings(extension_id, source_profile,
                                    target_profile)) {
    return false;
  }
  if (!ImportIndexedDBExtensionSettings(extension_id, source_profile,
                                        target_profile)) {
    return false;
  }
  return true;
}

ImportingExtension::ImportingExtension() = default;
ImportingExtension::ImportingExtension(ImportingExtension&&) = default;
ImportingExtension& ImportingExtension::operator=(ImportingExtension&&) =
    default;

ImportingExtension::~ImportingExtension() {
  if (installer) {
    installer->AbortInstall();
  }
}

ExtensionsImporter::ExtensionsImporter(const base::FilePath& source_profile,
                                       Profile* target_profile)
    : source_profile_(source_profile), target_profile_(target_profile) {
  CHECK(target_profile_);
  CHECK_NE(source_profile_, target_profile_->GetPath());
}

ExtensionsImporter::~ExtensionsImporter() = default;

// static
base::RepeatingCallback<ExtensionImportStatus(const std::string& extension_id)>&
ExtensionsImporter::GetExtensionInstallerForTesting() {
  static base::NoDestructor<base::RepeatingCallback<ExtensionImportStatus(
      const std::string& extension_id)>>
      g_intsaller;
  return *g_intsaller;
}

void ExtensionsImporter::Prepare(OnReady on_ready) {
  CHECK(extensions_.empty());
  extensions::GetExtensionFileTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&GetExtensionsList, source_profile_,
                     target_profile_->GetPath()),
      base::BindOnce(&ExtensionsImporter::OnGetExtensionsForImport,
                     weak_factory_.GetWeakPtr(), std::move(on_ready)));
}

bool ExtensionsImporter::Import(OnExtensionImported on_extension) {
  CHECK(!IsImportInProgress());

  for (auto& extension : extensions_) {
    if (extension.is_installed) {
      on_extension.Run(extension.id, ExtensionImportStatus::kOk);
      continue;
    }

    auto wrapper = base::BindOnce(
        [](base::WeakPtr<ExtensionsImporter> self, OnExtensionImported callback,
           const std::string& id, ExtensionImportStatus status) {
          if (self) {
            --self->in_progress_count_;
            callback.Run(id, status);
          }
        },
        weak_factory_.GetWeakPtr(), on_extension);

    ++in_progress_count_;
    if (auto installer = GetExtensionInstallerForTesting()) {
      const auto status = installer.Run(extension.id);
      base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
          FROM_HERE,
          base::BindOnce(&ExtensionsImporter::OnExtensionInstalled,
                         weak_factory_.GetWeakPtr(), &extension,
                         std::move(wrapper),
                         status == ExtensionImportStatus::kOk, "",
                         extensions::webstore_install::Result::SUCCESS));
    } else {
      extension.installer = base::MakeRefCounted<WebstoreInstallerForImporting>(
          extension.id, target_profile_, nullptr,
          base::BindOnce(&ExtensionsImporter::OnExtensionInstalled,
                         weak_factory_.GetWeakPtr(), &extension,
                         std::move(wrapper)));
      extension.installer->BeginInstall();
    }
  }

  return IsImportInProgress();
}

const ImportingExtension* ExtensionsImporter::GetExtension(
    const std::string& id) const {
  auto fnd = std::ranges::find(extensions_, id, &ImportingExtension::id);
  if (fnd == extensions_.end()) {
    return nullptr;
  }
  return &(*fnd);
}

bool ExtensionsImporter::IsImportInProgress() const {
  return in_progress_count_ > 0;
}

void ExtensionsImporter::OnGetExtensionsForImport(OnReady on_ready,
                                                  ExtensionsListResult result) {
  if (!result.has_value()) {
    return std::move(on_ready).Run(false);
  }
  extensions_ = std::move(result).value();

  auto* registry = extensions::ExtensionRegistry::Get(target_profile_);
  for (auto& extension : extensions_) {
    extension.is_installed =
        registry->GetInstalledExtension(extension.id) != nullptr;
  }

  std::move(on_ready).Run(true);
}

void ExtensionsImporter::OnExtensionInstalled(
    ImportingExtension* extension,
    OnOneExtensionImported on_extension,
    bool success,
    const std::string& error,
    extensions::webstore_install::Result result) {
  extension->is_installed = success;
  extension->installer.reset();

  if (!success) {
    return std::move(on_extension)
        .Run(extension->id, ExtensionImportStatus::kFailedToInstall);
  } else if (!extension->has_local_settings) {
    return std::move(on_extension)
        .Run(extension->id, ExtensionImportStatus::kOk);
  }

  CHECK(extension->has_local_settings);

  auto* service =
      extensions::ExtensionSystem::Get(target_profile_)->extension_service();
  service->DisableExtension(extension->id,
                            extensions::disable_reason::DISABLE_RELOAD);
  ImportExtensionSettings(extension, std::move(on_extension));
}

void ExtensionsImporter::ImportExtensionSettings(
    ImportingExtension* extension,
    OnOneExtensionImported on_extension) {
  CHECK(extension->has_local_settings);
  extensions::GetExtensionFileTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&extensions_import::ImportExtensionSettings, extension->id,
                     source_profile_, target_profile_->GetPath()),
      base::BindOnce(&ExtensionsImporter::OnExtensionSettingsImported,
                     weak_factory_.GetWeakPtr(), extension,
                     std::move(on_extension)));
}

void ExtensionsImporter::OnExtensionSettingsImported(
    ImportingExtension* extension,
    OnOneExtensionImported on_extension,
    bool success) {
  auto* service =
      extensions::ExtensionSystem::Get(target_profile_)->extension_service();
  service->EnableExtension(extension->id);

  if (!success) {
    return std::move(on_extension)
        .Run(extension->id, ExtensionImportStatus::kFailedToImportSettings);
  }

  std::move(on_extension).Run(extension->id, ExtensionImportStatus::kOk);
}

}  // namespace extensions_import
