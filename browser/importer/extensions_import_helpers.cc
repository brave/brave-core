/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/extensions_import_helpers.h"

#include <memory>

#include "base/containers/flat_map.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "brave/common/importer/importer_constants.h"
#include "components/value_store/value_store.h"
#include "components/value_store/value_store_factory.h"
#include "components/value_store/value_store_factory_impl.h"
#include "extensions/browser/api/storage/value_store_util.h"
#include "extensions/browser/extension_file_task_runner.h"
#include "extensions/common/constants.h"

namespace brave {
namespace {
using ExtensionStorageMap =
    base::flat_map<std::string, std::unique_ptr<value_store::ValueStore>>;

std::vector<std::string> GetChromeExtensionsSettingsList(
    const base::FilePath& source_profile,
    std::vector<std::string> extensions_ids) {
  auto store_factory_source =
      base::MakeRefCounted<value_store::ValueStoreFactoryImpl>(source_profile);
  std::vector<std::string> result;
  for (const auto& id : extensions_ids) {
    if (!store_factory_source->HasValueStore(
            base::FilePath(extensions::kLocalExtensionSettingsDirectoryName)
                .AppendASCII(id)))
      continue;
    result.push_back(id);
  }
  return result;
}

ExtensionStorageMap CreateStorages(
    std::vector<std::string> ids,
    bool skip_if_exists,
    scoped_refptr<value_store::ValueStoreFactory> factory) {
  ExtensionStorageMap storages;
  for (const auto& id : ids) {
    if (skip_if_exists &&
        extensions::value_store_util::HasValueStore(
            extensions::settings_namespace::LOCAL,
            extensions::value_store_util::ModelType::EXTENSION, id, factory)) {
      continue;
    }
    storages[id] = extensions::value_store_util::CreateSettingsStore(
        extensions::settings_namespace::LOCAL,
        extensions::value_store_util::ModelType::EXTENSION, id, factory);
  }
  return storages;
}

}  // namespace

void ImportStorages(base::FilePath source_profile,
                    base::FilePath target_profile,
                    std::vector<std::string> extensions_ids) {
  DCHECK(
      extensions::GetExtensionFileTaskRunner()->RunsTasksInCurrentSequence());
  const auto ids_with_settings =
      GetChromeExtensionsSettingsList(source_profile, extensions_ids);
  const auto source_storages = CreateStorages(
      ids_with_settings, false,
      base::MakeRefCounted<value_store::ValueStoreFactoryImpl>(source_profile));
  auto target_storages = CreateStorages(
      ids_with_settings, true,
      base::MakeRefCounted<value_store::ValueStoreFactoryImpl>(target_profile));
  for (const auto& source_store : source_storages) {
    auto content = source_store.second->Get();
    if (!content.status().ok() || !target_storages.count(source_store.first))
      continue;
    target_storages[source_store.first]->Set(value_store::ValueStore::DEFAULTS,
                                             content.PassSettings());
  }
}

void RemoveExtensionsSettings(base::FilePath target_profile,
                              const std::string& extension_id) {
  DCHECK(
      extensions::GetExtensionFileTaskRunner()->RunsTasksInCurrentSequence());
  extensions::value_store_util::DeleteValueStore(
      extensions::settings_namespace::LOCAL,
      extensions::value_store_util::ModelType::EXTENSION, extension_id,
      base::MakeRefCounted<value_store::ValueStoreFactoryImpl>(target_profile));
}

}  // namespace brave
