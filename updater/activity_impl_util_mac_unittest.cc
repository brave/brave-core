/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "chrome/updater/activity_impl_util_posix.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace updater {

TEST(ActivityImplUtilMacTest, GetActiveFile) {
  const base::FilePath home_dir("/Users/test");
  const std::string app_id = "test.app.id";

  const base::FilePath active_file = GetActiveFile(home_dir, app_id);

  EXPECT_EQ(active_file.value(),
            "/Users/test/Library/BraveSoftware/BraveSoftwareUpdate/"
            "Actives/test.app.id");
}

}  // namespace updater
