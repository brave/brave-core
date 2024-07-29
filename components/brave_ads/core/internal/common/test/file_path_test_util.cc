/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/file_path_test_util.h"

#include "base/files/file_path.h"
#include "base/path_service.h"

namespace brave_ads::test {

namespace {

base::FilePath RootPath() {
  return base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
      .AppendASCII("brave")
      .AppendASCII("components")
      .AppendASCII("brave_ads");
}

}  // namespace

base::FilePath DataResourcesPath() {
  return RootPath().AppendASCII("resources");
}

base::FilePath DataPath() {
  return RootPath().AppendASCII("core").AppendASCII("test").AppendASCII("data");
}

base::FilePath ResourceComponentsDataPath() {
  return DataPath().AppendASCII("resources").AppendASCII("components");
}

base::FilePath UrlResponsesDataPath() {
  return DataPath().AppendASCII("url_responses");
}

}  // namespace brave_ads::test
