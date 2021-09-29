/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_UNITTEST_FILE_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_UNITTEST_FILE_UTIL_H_

#include <string>

#include "base/files/file_path.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

base::FilePath GetTestPath();

absl::optional<std::string> ReadFileFromTestPathToString(
    const std::string& name);

base::FilePath GetResourcesPath();

absl::optional<std::string> ReadFileFromResourcePathToString(
    const std::string& name);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_UNITTEST_FILE_UTIL_H_
