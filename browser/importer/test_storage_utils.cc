/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/test_storage_utils.h"

#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "components/value_store/test_value_store_factory.h"
#include "components/value_store/value_store.h"
#include "extensions/common/constants.h"

namespace brave {

void CreateTestingStore(
    base::FilePath path,
    const std::string& id,
    const base::flat_map<std::string, std::string>& values) {
  auto store_factory =
      base::MakeRefCounted<value_store::TestValueStoreFactory>(path);
  auto source_store0 = store_factory->CreateValueStore(
      base::FilePath(extensions::kLocalExtensionSettingsDirectoryName), id);
  for (const auto& kv : values) {
    source_store0->Set(value_store::ValueStore::DEFAULTS, kv.first,
                       base::Value(kv.second));
  }
}

std::optional<base::Value::Dict> ReadStore(base::FilePath path,
                                           const std::string& id) {
  if (!base::DirectoryExists(path))
    return std::nullopt;
  auto store_factory =
      base::MakeRefCounted<value_store::TestValueStoreFactory>(path);
  auto source_store0 = store_factory->CreateValueStore(
      base::FilePath(extensions::kLocalExtensionSettingsDirectoryName), id);
  auto store = source_store0->Get();
  if (!store.status().ok())
    return std::nullopt;
  return store.PassSettings();
}

}  // namespace brave
