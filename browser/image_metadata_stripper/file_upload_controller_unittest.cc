/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/image_metadata_stripper/file_upload_controller.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_path_override.h"
#include "base/test/task_environment.h"
#include "brave/components/image_metadata_stripper/common/features.h"
#include "chrome/common/chrome_paths.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace image_metadata_stripper {

class FileUploadControllerCleanUpTestBase : public testing::Test {
 protected:
  explicit FileUploadControllerCleanUpTestBase(bool strip_metadata) {
    feature_list_.InitWithFeatureState(features::kStripImageMetadataV1,
                                       strip_metadata);
  }

  base::FilePath UserDataDir() {
    return base::PathService::CheckedGet(chrome::DIR_USER_DATA);
  }

  base::FilePath StripperRootDir() {
    return UserDataDir().Append(kStripperRootDirName);
  }

  // Lays out what a previous session could leave behind: two tab directories,
  // each holding stripped copies in numbered subdirectories.
  void CreateLeftoversFromLastSession() {
    const base::FilePath tab_a =
        StripperRootDir().Append(kStripTempDirPrefix).AddExtensionASCII("a");
    const base::FilePath tab_b =
        StripperRootDir().Append(kStripTempDirPrefix).AddExtensionASCII("b");
    for (const base::FilePath& file :
         {tab_a.AppendASCII("0").AppendASCII("photo.jpg"),
          tab_a.AppendASCII("1").AppendASCII("photo.jpg"),
          tab_b.AppendASCII("0").AppendASCII("image.jpg")}) {
      ASSERT_TRUE(base::CreateDirectory(file.DirName()));
      ASSERT_TRUE(base::WriteFile(file, "stripped"));
    }
  }

  // Must outlive the thread pool started by |task_environment_|.
  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_;
  // Keep the cleanup away from the real user data directory.
  base::ScopedPathOverride user_data_dir_override_{chrome::DIR_USER_DATA};
};

class FileUploadControllerCleanUpTest
    : public FileUploadControllerCleanUpTestBase {
 public:
  FileUploadControllerCleanUpTest()
      : FileUploadControllerCleanUpTestBase(/*strip_metadata=*/true) {}
};

class FileUploadControllerCleanUpDisabledTest
    : public FileUploadControllerCleanUpTestBase {
 public:
  FileUploadControllerCleanUpDisabledTest()
      : FileUploadControllerCleanUpTestBase(/*strip_metadata=*/false) {}
};

TEST_F(FileUploadControllerCleanUpTest, DeletesFilesLeftFromLastSession) {
  CreateLeftoversFromLastSession();
  ASSERT_TRUE(base::PathExists(StripperRootDir()));

  FileUploadController::CleanUpFromLastSession();
  base::ThreadPoolInstance::Get()->FlushForTesting();

  EXPECT_FALSE(base::PathExists(StripperRootDir()));
}

TEST_F(FileUploadControllerCleanUpTest, LeavesTheRestOfUserDataAlone) {
  CreateLeftoversFromLastSession();
  const base::FilePath sibling_dir = UserDataDir().AppendASCII("Default");
  const base::FilePath sibling_file = sibling_dir.AppendASCII("Preferences");
  ASSERT_TRUE(base::CreateDirectory(sibling_dir));
  ASSERT_TRUE(base::WriteFile(sibling_file, "{}"));

  FileUploadController::CleanUpFromLastSession();
  base::ThreadPoolInstance::Get()->FlushForTesting();

  EXPECT_FALSE(base::PathExists(StripperRootDir()));
  EXPECT_TRUE(base::PathExists(sibling_file));
}

TEST_F(FileUploadControllerCleanUpTest, DoesNothingWhenThereAreNoLeftovers) {
  ASSERT_FALSE(base::PathExists(StripperRootDir()));

  FileUploadController::CleanUpFromLastSession();

  EXPECT_FALSE(base::PathExists(StripperRootDir()));
  EXPECT_TRUE(base::PathExists(UserDataDir()));
}

TEST_F(FileUploadControllerCleanUpDisabledTest, KeepsFilesLeftFromLastSession) {
  CreateLeftoversFromLastSession();

  FileUploadController::CleanUpFromLastSession();

  EXPECT_TRUE(base::PathExists(StripperRootDir()));
}

}  // namespace image_metadata_stripper
