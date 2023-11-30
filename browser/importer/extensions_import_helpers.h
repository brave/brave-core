/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_EXTENSIONS_IMPORT_HELPERS_H_
#define BRAVE_BROWSER_IMPORTER_EXTENSIONS_IMPORT_HELPERS_H_

#include <string>
#include <vector>

#include "base/files/file_util.h"
#include "base/values.h"

namespace brave {
void ImportStorages(base::FilePath source_profile,
                    base::FilePath target_profile,
                    std::vector<std::string> extensions_ids);

void RemoveExtensionsSettings(base::FilePath target_profile,
                              const std::string& extension_id);
}  // namespace brave

#endif  // BRAVE_BROWSER_IMPORTER_EXTENSIONS_IMPORT_HELPERS_H_
