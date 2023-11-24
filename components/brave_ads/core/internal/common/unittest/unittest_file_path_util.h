/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_FILE_PATH_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_FILE_PATH_UTIL_H_

#include "base/files/file_path.h"

namespace brave_ads {

base::FilePath TestDataFileResourcesPath();

base::FilePath DataResourcesPath();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_FILE_PATH_UTIL_H_
