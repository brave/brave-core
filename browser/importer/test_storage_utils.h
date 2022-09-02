/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_TEST_STORAGE_UTILS_H_
#define BRAVE_BROWSER_IMPORTER_TEST_STORAGE_UTILS_H_

#include <string>

#include "base/files/file_util.h"
#include "base/values.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave {
absl::optional<base::Value::Dict> ReadStore(base::FilePath path,
                                            const std::string& id);
void CreateTestingStore(base::FilePath path, const std::string& id);
}  // namespace brave

#endif  // BRAVE_BROWSER_IMPORTER_TEST_STORAGE_UTILS_H_
