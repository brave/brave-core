/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/file_select/brave_file_select_image_metadata_stripper.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/base_paths.h"
#include "base/callback_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/path_service.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_path_override.h"
#include "base/test/test_future.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "brave/browser/image_metadata_stripper/file_upload_controller.h"
#include "brave/components/image_metadata_stripper/common/features.h"
#include "chrome/common/chrome_paths.h"
#include "components/tabs/public/mock_tab_interface.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/choosers/file_chooser.mojom.h"
#include "url/gurl.h"

namespace brave {

namespace {

constexpr char kFbmdImageName[] = "fbmd_test_image.jpg";

constexpr std::string_view kFbmdMarker = "FBMD";

using FileList = std::vector<blink::mojom::FileChooserFileInfoPtr>;

blink::mojom::FileChooserFileInfoPtr NativeFile(const base::FilePath& path) {
  return blink::mojom::FileChooserFileInfo::NewNativeFile(
      blink::mojom::NativeFileInfo::New(path, std::u16string(),
                                        std::vector<std::u16string>()));
}

}  // namespace

class FileSelectImageMetadataStripperTestBase
    : public content::RenderViewHostTestHarness {
 public:
  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();

    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(source_dir_.CreateUniqueTempDir());
  }

  void TearDown() override {
    upload_controller_.reset();
    content::RenderViewHostTestHarness::TearDown();
  }

 protected:
  explicit FileSelectImageMetadataStripperTestBase(bool strip_metadata) {
    feature_list_.InitWithFeatureState(
        image_metadata_stripper::features::kStripImageMetadataV1,
        strip_metadata);
  }

  // Production associates a tab with its WebContents and creates the
  // controller from BraveTabFeatures. The harness contents are not a tab, so
  // do that here.
  void AttachTabWithController() {
    tabs::TabLookupFromWebContents::CreateForWebContents(web_contents(), &tab_);
    ON_CALL(tab_, GetContents()).WillByDefault(testing::Return(web_contents()));
    ON_CALL(tab_, RegisterWillDiscardContents(testing::_))
        .WillByDefault([](tabs::TabInterface::WillDiscardContentsCallback) {
          return base::CallbackListSubscription();
        });
    upload_controller_ =
        image_metadata_stripper::FileUploadController::MaybeCreate(tab_);
  }

  // Copies the checked-in image carrying FBMD metadata into a directory the
  // test owns, so the stripper can never be seen to touch the original.
  base::FilePath CreateSelectableFbmdImage() {
    const base::FilePath test_image =
        base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
            .AppendASCII("brave/test/data/image_metadata_stripper")
            .AppendASCII(kFbmdImageName);

    const base::FilePath selected =
        source_dir_.GetPath().AppendASCII(kFbmdImageName);
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_TRUE(base::CopyFile(test_image, selected));
    return selected;
  }

  base::FilePath CreateSelectableFile(std::string_view name,
                                      std::string_view contents) {
    const base::FilePath path = source_dir_.GetPath().AppendASCII(name);
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_TRUE(base::WriteFile(path, contents));
    return path;
  }

  // Drives MaybeStripImageMetadataForUpload the way
  // FileSelectHelper::NotifyListenerAndEnd does. Returns the list handed to
  // |notify|, or std::nullopt when no strip was started, in which case |list|
  // is left with the caller.
  std::optional<FileList> Strip(content::WebContents* contents,
                                FileList& list) {
    bool already_processed = false;
    base::test::TestFuture<FileList> future;
    if (!MaybeStripImageMetadataForUpload(contents, already_processed, list,
                                          future.GetCallback())) {
      EXPECT_FALSE(already_processed);
      return std::nullopt;
    }
    EXPECT_TRUE(already_processed);
    return future.Take();
  }

  bool ContainsFbmd(const base::FilePath& path) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    std::string contents;
    EXPECT_TRUE(base::ReadFileToString(path, &contents));
    return contents.find(kFbmdMarker) != std::string::npos;
  }

  bool PathExists(const base::FilePath& path) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    return base::PathExists(path);
  }

  base::FilePath StripperRootDir() {
    return base::PathService::CheckedGet(chrome::DIR_USER_DATA)
        .Append(image_metadata_stripper::kStripperRootDirName);
  }

  base::ScopedTempDir source_dir_;
  // Stripped copies are written under the user data directory; keep them out
  // of the real one.
  base::ScopedPathOverride user_data_dir_override_{chrome::DIR_USER_DATA};
  base::test::ScopedFeatureList feature_list_;
  tabs::MockTabInterface tab_;
  std::unique_ptr<image_metadata_stripper::FileUploadController>
      upload_controller_;
};

class FileSelectImageMetadataStripperTest
    : public FileSelectImageMetadataStripperTestBase {
 public:
  FileSelectImageMetadataStripperTest()
      : FileSelectImageMetadataStripperTestBase(/*strip_metadata=*/true) {}
};

class FileSelectImageMetadataStripperDisabledTest
    : public FileSelectImageMetadataStripperTestBase {
 public:
  FileSelectImageMetadataStripperDisabledTest()
      : FileSelectImageMetadataStripperTestBase(/*strip_metadata=*/false) {}
};

TEST_F(FileSelectImageMetadataStripperTest, UploadsAStrippedCopyOfTheImage) {
  AttachTabWithController();
  const base::FilePath selected = CreateSelectableFbmdImage();
  ASSERT_TRUE(ContainsFbmd(selected));

  FileList list;
  list.push_back(NativeFile(selected));
  std::optional<FileList> result = Strip(web_contents(), list);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(1u, result->size());
  const auto& native = (*result)[0]->get_native_file();
  const base::FilePath& stripped_copy = native->file_path;

  EXPECT_NE(selected, stripped_copy);
  EXPECT_TRUE(StripperRootDir().IsParent(stripped_copy));
  EXPECT_FALSE(ContainsFbmd(stripped_copy));
  // The page must still see the name of the file that was picked.
  EXPECT_EQ(selected.BaseName(), stripped_copy.BaseName());
  EXPECT_EQ(selected.BaseName().AsUTF16Unsafe(), native->display_name);
  // The file the user picked is never modified.
  EXPECT_TRUE(ContainsFbmd(selected));
}

TEST_F(FileSelectImageMetadataStripperTest, KeepsFilesThatCannotBeStripped) {
  AttachTabWithController();
  const base::FilePath text = CreateSelectableFile("notes.txt", "FBMD");
  const base::FilePath clean_jpeg =
      CreateSelectableFile("no_fbmd.jpg", "not fbmd");
  const GURL file_system_url("filesystem:https://example.com/temporary/a.jpg");

  FileList list;
  list.push_back(NativeFile(text));
  list.push_back(NativeFile(clean_jpeg));
  list.push_back(blink::mojom::FileChooserFileInfo::NewFileSystem(
      blink::mojom::FileSystemFileInfo::New(file_system_url, base::Time(), 0)));
  std::optional<FileList> result = Strip(web_contents(), list);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(3u, result->size());
  EXPECT_EQ(text, (*result)[0]->get_native_file()->file_path);
  EXPECT_TRUE((*result)[0]->get_native_file()->display_name.empty());
  EXPECT_EQ(clean_jpeg, (*result)[1]->get_native_file()->file_path);
  EXPECT_TRUE((*result)[1]->get_native_file()->display_name.empty());
  ASSERT_TRUE((*result)[2]->is_file_system());
  EXPECT_EQ(file_system_url, (*result)[2]->get_file_system()->url);
}

TEST_F(FileSelectImageMetadataStripperTest,
       DeletesStrippedCopiesWhenTheControllerIsDestroyed) {
  AttachTabWithController();

  FileList list;
  list.push_back(NativeFile(CreateSelectableFbmdImage()));
  std::optional<FileList> result = Strip(web_contents(), list);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(1u, result->size());
  // The stripped copy lives at <tab dir>/<index>/<original basename>.
  const base::FilePath tab_dir =
      (*result)[0]->get_native_file()->file_path.DirName().DirName();
  ASSERT_TRUE(PathExists(tab_dir));

  upload_controller_.reset();
  ASSERT_TRUE(base::test::RunUntil([&]() { return !PathExists(tab_dir); }));
}

TEST_F(FileSelectImageMetadataStripperTest, DoesNothingWithoutWebContents) {
  FileList list;
  list.push_back(NativeFile(CreateSelectableFbmdImage()));

  EXPECT_FALSE(Strip(/*contents=*/nullptr, list).has_value());
  EXPECT_EQ(1u, list.size());
}

TEST_F(FileSelectImageMetadataStripperTest, DoesNothingForAnEmptySelection) {
  AttachTabWithController();

  FileList list;
  EXPECT_FALSE(Strip(web_contents(), list).has_value());
}

TEST_F(FileSelectImageMetadataStripperTest,
       DoesNothingWhenTheContentsHaveNoController) {
  const base::FilePath selected = CreateSelectableFbmdImage();
  FileList list;
  list.push_back(NativeFile(selected));

  EXPECT_FALSE(Strip(web_contents(), list).has_value());
  ASSERT_EQ(1u, list.size());
  EXPECT_EQ(selected, list[0]->get_native_file()->file_path);
}

TEST_F(FileSelectImageMetadataStripperTest, DoesNothingWhenAlreadyProcessed) {
  AttachTabWithController();
  const base::FilePath selected = CreateSelectableFbmdImage();
  FileList list;
  list.push_back(NativeFile(selected));

  bool already_processed = true;
  EXPECT_FALSE(MaybeStripImageMetadataForUpload(
      web_contents(), already_processed, list,
      base::BindOnce([](FileList) { ADD_FAILURE(); })));
  EXPECT_TRUE(already_processed);
  ASSERT_EQ(1u, list.size());
  EXPECT_EQ(selected, list[0]->get_native_file()->file_path);
}

TEST_F(FileSelectImageMetadataStripperDisabledTest, KeepsTheSelectedImage) {
  AttachTabWithController();
  const base::FilePath selected = CreateSelectableFbmdImage();
  FileList list;
  list.push_back(NativeFile(selected));

  EXPECT_FALSE(Strip(web_contents(), list).has_value());
  ASSERT_EQ(1u, list.size());
  EXPECT_EQ(selected, list[0]->get_native_file()->file_path);
  EXPECT_TRUE(ContainsFbmd(selected));
  EXPECT_FALSE(PathExists(StripperRootDir()));
}

}  // namespace brave
