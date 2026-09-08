/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// This file contains code that used to be upstream and had to be restored in
// Brave to support delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937

#include "brave/installer/setup/archive_patch_helper.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class ArchivePatchHelperTest : public testing::Test {
 protected:
  void SetUp() override {
    // This requires chrome::RegisterPathProvider() to have been called.
    ASSERT_TRUE(base::PathService::Get(chrome::DIR_TEST_DATA, &data_dir_));
    data_dir_ = data_dir_.AppendASCII("installer");
    ASSERT_TRUE(base::PathExists(data_dir_));

    // Create a temp directory for testing.
    ASSERT_TRUE(test_dir_.CreateUniqueTempDir());
  }

  void TearDown() override {
    data_dir_.clear();
    // Clean up test directory manually so we can fail if it leaks.
    ASSERT_TRUE(test_dir_.Delete());
  }

  // The path to input data used in tests.
  base::FilePath data_dir_;

  // The temporary directory used to contain the test operations.
  base::ScopedTempDir test_dir_;
};

}  // namespace

TEST_F(ArchivePatchHelperTest, ZucchiniPatching) {
  base::FilePath src = data_dir_.AppendASCII("archive1.7z");
  base::FilePath patch = data_dir_.AppendASCII("zucchini_archive.diff");
  base::FilePath dest = test_dir_.GetPath().AppendASCII("archive2.7z");
  installer::ArchivePatchHelper archive_helper(
      test_dir_.GetPath(), base::FilePath(), src, dest,
      installer::UnPackConsumer::UNCOMPRESSED_CHROME_ARCHIVE);
  archive_helper.set_last_uncompressed_file(patch);
  EXPECT_TRUE(archive_helper.ZucchiniEnsemblePatch());
  base::FilePath base = data_dir_.AppendASCII("archive2.7z");
  EXPECT_TRUE(base::ContentsEqual(dest, base));
}

// UncompressAndPatch is the composite operation used by the setup.exe
// self-patch flow (--update-setup-exe): uncompress a compressed archive
// holding a patch file, then apply that patch. test_patch.packed.7z holds a
// copy of zucchini_archive.diff, which patches archive1.7z into archive2.7z.
TEST_F(ArchivePatchHelperTest, UncompressAndPatch) {
  base::FilePath src_root;
  ASSERT_TRUE(base::PathService::Get(base::DIR_SRC_TEST_DATA_ROOT, &src_root));
  base::FilePath compressed = src_root.AppendASCII("brave")
                                  .AppendASCII("test")
                                  .AppendASCII("data")
                                  .AppendASCII("installer")
                                  .AppendASCII("test_patch.packed.7z");
  base::FilePath src = data_dir_.AppendASCII("archive1.7z");
  base::FilePath dest = test_dir_.GetPath().AppendASCII("archive2.7z");
  EXPECT_TRUE(installer::ArchivePatchHelper::UncompressAndPatch(
      test_dir_.GetPath(), compressed, src, dest,
      installer::UnPackConsumer::UNCOMPRESSED_CHROME_ARCHIVE));
  EXPECT_TRUE(base::ContentsEqual(dest, data_dir_.AppendASCII("archive2.7z")));
  // The extracted patch file is deleted after it has been applied.
  EXPECT_FALSE(
      base::PathExists(test_dir_.GetPath().AppendASCII("chrome_patch.diff")));
}

TEST_F(ArchivePatchHelperTest, InvalidDiff_MisalignedCblen) {
  base::FilePath src = data_dir_.AppendASCII("bin.old");
  base::FilePath patch = data_dir_.AppendASCII("misaligned_cblen.diff");
  base::FilePath dest = test_dir_.GetPath().AppendASCII("bin.new");
  installer::ArchivePatchHelper archive_helper(
      test_dir_.GetPath(), base::FilePath(), src, dest,
      installer::UnPackConsumer::UNCOMPRESSED_CHROME_ARCHIVE);
  archive_helper.set_last_uncompressed_file(patch);
  // Should fail, but not crash.
  EXPECT_FALSE(archive_helper.BinaryPatch());
}

TEST_F(ArchivePatchHelperTest, InvalidDiff_NegativeSeek) {
  base::FilePath src = data_dir_.AppendASCII("bin.old");
  base::FilePath patch = data_dir_.AppendASCII("negative_seek.diff");
  base::FilePath dest = test_dir_.GetPath().AppendASCII("bin.new");
  installer::ArchivePatchHelper archive_helper(
      test_dir_.GetPath(), base::FilePath(), src, dest,
      installer::UnPackConsumer::UNCOMPRESSED_CHROME_ARCHIVE);
  archive_helper.set_last_uncompressed_file(patch);
  // Should fail, but not crash.
  EXPECT_FALSE(archive_helper.BinaryPatch());
}
