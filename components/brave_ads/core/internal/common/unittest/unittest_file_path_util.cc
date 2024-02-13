/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_path_util.h"

#include "base/files/file_path.h"
#include "base/path_service.h"

namespace brave_ads {

namespace {

base::FilePath TestDataRootPath() {
  return base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
      .AppendASCII("brave")
      .AppendASCII("components")
      .AppendASCII("brave_ads");
}

}  // namespace

base::FilePath TestDataPath() {
  return TestDataRootPath().AppendASCII("core").AppendASCII("test").AppendASCII(
      "data");
}

base::FilePath ComponentResourcesTestDataPath() {
  return TestDataPath().AppendASCII("resources");
}

base::FilePath DataResourcesPath() {
  return TestDataRootPath().AppendASCII("resources");
}

}  // namespace brave_ads
