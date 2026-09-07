/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/file_select/brave_file_select_image_metadata_stripper.h"

#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave {

class BraveFileSelectImageMetadataStripperUnitTest : public testing::Test {
 protected:
  void TearDown() override {
    for (const auto& path : cleanup_) {
      base::DeletePathRecursively(path);
    }
  }

  // Same CreateNewTempDirectory prefix as production `temp_root_dir`.
  base::FilePath CreateTempRootDir() {
    base::FilePath temp_root_dir;
    EXPECT_TRUE(base::CreateNewTempDirectory(kUploadStripTempDirPrefix,
                                             &temp_root_dir));
    cleanup_.push_back(temp_root_dir);
    return temp_root_dir;
  }

  base::FilePath CreateUnrelatedTempFile() {
    base::FilePath file;
    EXPECT_TRUE(base::CreateTemporaryFile(&file));
    cleanup_.push_back(file);
    return file;
  }

  base::FilePath CreateUnrelatedTempDir() {
    base::FilePath dir;
    EXPECT_TRUE(
        base::CreateNewTempDirectory(FILE_PATH_LITERAL("other_prefix"), &dir));
    cleanup_.push_back(dir);
    return dir;
  }

  std::vector<base::FilePath> cleanup_;
};

TEST_F(BraveFileSelectImageMetadataStripperUnitTest,
       DeletesTempRootAndRemovesItFromTheList) {
  const base::FilePath temp_root_dir = CreateTempRootDir();
  ASSERT_TRUE(
      base::WriteFile(temp_root_dir.AppendASCII("photo.jpg"), "stripped"));

  std::vector<base::FilePath> paths = {temp_root_dir};
  DeleteImageMetadataStripperTemporaryDir(paths);

  EXPECT_TRUE(paths.empty());
  EXPECT_FALSE(base::PathExists(temp_root_dir));
}

TEST_F(BraveFileSelectImageMetadataStripperUnitTest,
       LeavesNonTempRootFilesUntouched) {
  // Create another unrelated directory.
  const base::FilePath leftover = CreateUnrelatedTempFile();

  std::vector<base::FilePath> paths = {leftover};
  DeleteImageMetadataStripperTemporaryDir(paths);

  ASSERT_EQ(1u, paths.size());
  EXPECT_EQ(leftover, paths[0]);
  EXPECT_TRUE(base::PathExists(leftover));
}

TEST_F(BraveFileSelectImageMetadataStripperUnitTest,
       IgnoresAFileWhoseNameContainsTheBraveUploadStripPrefix) {
  // The token in the basename is not enough; the path must be a directory.
  const base::FilePath leftover = CreateUnrelatedTempFile();
  const base::FilePath prefixed_file = leftover.DirName().Append(
      base::FilePath::StringType(kUploadStripTempDirPrefix) +
      FILE_PATH_LITERAL("_not_a_dir"));
  ASSERT_TRUE(base::CopyFile(leftover, prefixed_file));
  cleanup_.push_back(prefixed_file);

  std::vector<base::FilePath> paths = {prefixed_file};
  DeleteImageMetadataStripperTemporaryDir(paths);

  ASSERT_EQ(1u, paths.size());
  EXPECT_EQ(prefixed_file, paths[0]);
  EXPECT_TRUE(base::PathExists(prefixed_file));
}

}  // namespace brave
