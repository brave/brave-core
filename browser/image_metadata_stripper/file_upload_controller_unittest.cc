/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/image_metadata_stripper/file_upload_controller.h"

#include <memory>
#include <optional>
#include <vector>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_path_override.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/image_metadata_stripper/common/features.h"
#include "brave/components/image_metadata_stripper/common/stripping_client.h"
#include "chrome/common/chrome_paths.h"
#include "components/tabs/public/mock_tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gmock/include/gmock/gmock.h"
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

// Drives PrimaryPageChanged on a controller that has already written a stripped
// copy, without a browser.
class FileUploadControllerPrimaryPageChangedTest
    : public content::RenderViewHostTestHarness {
 public:
  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();

    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(source_dir_.CreateUniqueTempDir());

    // The harness WebContents already has a primary page. Navigating here goes
    // through Brave's WebUI factory, which requires a Profile.
    ON_CALL(tab_, GetContents()).WillByDefault(testing::Return(web_contents()));
    ON_CALL(tab_, RegisterWillDiscardContents(testing::_))
        .WillByDefault([](tabs::TabInterface::WillDiscardContentsCallback) {
          return base::CallbackListSubscription();
        });
    controller_ = FileUploadController::MaybeCreate(tab_);
  }

  void TearDown() override {
    // Stop observing before the harness deletes the WebContents.
    controller_.reset();
    content::RenderViewHostTestHarness::TearDown();
  }

 protected:
  base::FilePath CreateSelectableFbmdImage() {
    const base::FilePath test_image =
        base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
            .AppendASCII("brave/test/data/image_metadata_stripper")
            .AppendASCII("fbmd_test_image.jpg");
    const base::FilePath selected =
        source_dir_.GetPath().AppendASCII("fbmd_test_image.jpg");
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_TRUE(base::CopyFile(test_image, selected));
    return selected;
  }

  bool PathExists(const base::FilePath& path) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    return base::PathExists(path);
  }

  // Stripped copies are written under the user data directory; keep them out
  // of the real one.
  base::ScopedPathOverride user_data_dir_override_{chrome::DIR_USER_DATA};
  base::ScopedTempDir source_dir_;
  tabs::MockTabInterface tab_;
  std::unique_ptr<FileUploadController> controller_;
};

TEST_F(FileUploadControllerPrimaryPageChangedTest,
       DeletesTempFileWhenPrimaryPageChanges) {
  base::test::TestFuture<std::vector<std::optional<base::FilePath>>> future;
  controller_->Strip({CreateSelectableFbmdImage()},
                     StrippingClient::kFileSelect, future.GetCallback());

  std::vector<std::optional<base::FilePath>> copies = future.Take();
  ASSERT_EQ(1u, copies.size());
  ASSERT_TRUE(copies[0].has_value());
  // The stripped copy lives at <tab dir>/<index>/<original basename>.
  const base::FilePath tab_dir = copies[0]->DirName().DirName();
  ASSERT_TRUE(PathExists(tab_dir));

  content::WebContentsObserver* observer = controller_.get();
  observer->PrimaryPageChanged(web_contents()->GetPrimaryPage());
  base::ThreadPoolInstance::Get()->FlushForTesting();

  EXPECT_FALSE(PathExists(tab_dir));
}

}  // namespace image_metadata_stripper
