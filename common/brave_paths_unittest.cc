/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/brave_paths.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BravePathsTest, PathTest) {
  base::FilePath test_dir;
  EXPECT_TRUE(base::PathService::Get(brave::DIR_TEST_DATA, &test_dir));
  EXPECT_FALSE(base::PathService::Get(chrome::PATH_END, &test_dir));
}
