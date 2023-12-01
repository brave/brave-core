/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_TEST_STORAGE_UTILS_H_
#define BRAVE_BROWSER_IMPORTER_TEST_STORAGE_UTILS_H_

#include <optional>
#include <string>

#include "base/containers/flat_map.h"
#include "base/files/file_util.h"
#include "base/values.h"

namespace brave {
std::optional<base::Value::Dict> ReadStore(base::FilePath path,
                                           const std::string& id);
void CreateTestingStore(base::FilePath path,
                        const std::string& id,
                        const base::flat_map<std::string, std::string>& values);
}  // namespace brave

#endif  // BRAVE_BROWSER_IMPORTER_TEST_STORAGE_UTILS_H_
