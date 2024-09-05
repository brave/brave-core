// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ipfs/ipfs_component_cleaner.h"

#include <fstream>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/ipfs/ipfs_common.h"
#include "build/build_config.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

class IpfsComponentCleanerUnitTest : public testing::Test {
 public:
  IpfsComponentCleanerUnitTest() = default;
  ~IpfsComponentCleanerUnitTest() override = default;

 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    base::FilePath user_data_dir_tmp =
        temp_dir_.GetPath().Append(FILE_PATH_LITERAL("user_data"));
    ASSERT_TRUE(
        base::PathService::Override(chrome::DIR_USER_DATA, user_data_dir_tmp));
    user_data_path_ = base::PathService::CheckedGet(chrome::DIR_USER_DATA);
  }

  content::BrowserTaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::ScopedTempDir temp_dir_;
  base::FilePath user_data_path_;
};

TEST_F(IpfsComponentCleanerUnitTest, CleanIpfsComponent) {
  EXPECT_FALSE(user_data_path_.empty());
  base::FilePath cache_folder =
      user_data_path_.Append(FILE_PATH_LITERAL("brave_ipfs"));
  base::CreateDirectory(cache_folder);
  EXPECT_TRUE(base::PathExists(cache_folder));
  base::FilePath cache_folder_subdir =
      cache_folder.Append(FILE_PATH_LITERAL("subdir1"));
  base::CreateDirectory(cache_folder_subdir);
  EXPECT_TRUE(base::PathExists(cache_folder_subdir));
  base::FilePath cache_folder_subdir_file_01 =
      cache_folder_subdir.Append(FILE_PATH_LITERAL("The file 01.txt"));
  base::WriteFile(cache_folder_subdir_file_01, "12345678901234567890");

  base::FilePath component_id_folder =
      user_data_path_.Append(base::FilePath(kIpfsClientComponentId));
  base::CreateDirectory(component_id_folder);
  EXPECT_TRUE(base::PathExists(component_id_folder));
  base::FilePath component_id_folde_subdir =
      component_id_folder.Append(FILE_PATH_LITERAL("subdir1"));
  base::CreateDirectory(component_id_folde_subdir);
  EXPECT_TRUE(base::PathExists(component_id_folde_subdir));
  base::FilePath component_id_folde_subdir_file_01 =
      component_id_folde_subdir.Append(FILE_PATH_LITERAL("The file 01.txt"));
  base::WriteFile(component_id_folde_subdir_file_01, "12345678901234567890");

  ipfs::CleanupIpfsComponent(
      base::PathService::CheckedGet(chrome::DIR_USER_DATA));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(base::PathExists(cache_folder));
  EXPECT_FALSE(base::PathExists(component_id_folder));
}
