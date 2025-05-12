/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_source_test_util.h"

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "brave/components/constants/brave_paths.h"

namespace ntp_background_images::test {

base::FilePath GetSponsoredImagesComponentPath() {
  const base::FilePath file_path =
      base::PathService::CheckedGet(brave::DIR_TEST_DATA);
  return file_path.AppendASCII("components")
      .AppendASCII("ntp_sponsored_images");
}

}  // namespace ntp_background_images::test
