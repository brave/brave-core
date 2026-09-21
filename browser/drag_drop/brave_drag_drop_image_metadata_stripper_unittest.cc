/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/drag_drop/brave_drag_drop_image_metadata_stripper.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/image_metadata_stripper/common/features.h"
#include "content/public/common/drop_data.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/clipboard/file_info.h"

namespace brave {

namespace {

constexpr char kFbmdImageName[] = "fbmd_test_image.jpg";

constexpr std::string_view kFbmdMarker = "FBMD";

}  // namespace

class DragDropImageMetadataStripperTestBase
    : public content::RenderViewHostTestHarness {
 public:
  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();

    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(source_dir_.CreateUniqueTempDir());
  }

 protected:
  explicit DragDropImageMetadataStripperTestBase(bool strip_metadata) {
    feature_list_.InitWithFeatureState(
        image_metadata_stripper::features::kStripImageMetadataV1,
        strip_metadata);
  }

  // Copies the checked-in image carrying FBMD metadata into a directory the
  // test owns, so the stripper can never be seen to touch the original.
  base::FilePath CreateDroppableFbmdImage() {
    const base::FilePath test_image =
        base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
            .AppendASCII("brave/test/data/image_metadata_stripper")
            .AppendASCII(kFbmdImageName);

    const base::FilePath dropped =
        source_dir_.GetPath().AppendASCII(kFbmdImageName);
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_TRUE(base::CopyFile(test_image, dropped));
    return dropped;
  }

  base::FilePath CreateDroppableFile(std::string_view name,
                                     std::string_view contents) {
    const base::FilePath path = source_dir_.GetPath().AppendASCII(name);
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_TRUE(base::WriteFile(path, contents));
    return path;
  }

  // Drives the wrapped completion callback the way HandleOnPerformingDrop does
  // and waits for the drop to complete.
  std::optional<content::DropData> PerformDrop(
      std::optional<content::DropData> drop_data) {
    base::test::TestFuture<std::optional<content::DropData>> future;
    MaybeStripImageMetadataForDrop(web_contents(), future.GetCallback())
        .Run(std::move(drop_data));
    return future.Take();
  }

  std::optional<content::DropData> PerformDropOf(const base::FilePath& path) {
    content::DropData drop_data;
    drop_data.filenames.emplace_back(path, base::FilePath());
    return PerformDrop(std::move(drop_data));
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

  // The stripped copy lives at <temp root>/<index>/<original basename>.
  base::FilePath TempRootOf(const base::FilePath& stripped_copy) {
    return stripped_copy.DirName().DirName();
  }

  base::ScopedTempDir source_dir_;
  base::test::ScopedFeatureList feature_list_;
};

class DragDropImageMetadataStripperTest
    : public DragDropImageMetadataStripperTestBase {
 public:
  DragDropImageMetadataStripperTest()
      : DragDropImageMetadataStripperTestBase(/*strip_metadata=*/true) {}
};

class DragDropImageMetadataStripperDisabledTest
    : public DragDropImageMetadataStripperTestBase {
 public:
  DragDropImageMetadataStripperDisabledTest()
      : DragDropImageMetadataStripperTestBase(/*strip_metadata=*/false) {}
};

TEST_F(DragDropImageMetadataStripperTest, DropsAStrippedCopyOfTheImage) {
  const base::FilePath dropped = CreateDroppableFbmdImage();
  ASSERT_TRUE(ContainsFbmd(dropped));

  const std::optional<content::DropData> result = PerformDropOf(dropped);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(1u, result->filenames.size());
  const base::FilePath& stripped_copy = result->filenames[0].path;

  EXPECT_NE(dropped, stripped_copy);
  EXPECT_FALSE(ContainsFbmd(stripped_copy));
  // The page must still see the name of the file that was dropped.
  EXPECT_EQ(dropped.BaseName(), stripped_copy.BaseName());
  // The file the user dropped is never modified.
  EXPECT_TRUE(ContainsFbmd(dropped));
}

TEST_F(DragDropImageMetadataStripperTest,
       DeletesStrippedCopiesWhenWebContentsIsDestroyed) {
  const std::optional<content::DropData> result =
      PerformDropOf(CreateDroppableFbmdImage());

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(1u, result->filenames.size());
  const base::FilePath temp_root = TempRootOf(result->filenames[0].path);
  ASSERT_TRUE(PathExists(temp_root));

  DeleteContents();
  ASSERT_TRUE(base::test::RunUntil([&]() { return !PathExists(temp_root); }));
}

TEST_F(DragDropImageMetadataStripperTest, KeepsAJpegWithoutMetadataToStrip) {
  const base::FilePath dropped = CreateDroppableFile("no_fbmd.jpg", "not fbmd");

  const std::optional<content::DropData> result = PerformDropOf(dropped);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(1u, result->filenames.size());
  EXPECT_EQ(dropped, result->filenames[0].path);
}

TEST_F(DragDropImageMetadataStripperTest, KeepsFilesThatAreNotJpegs) {
  const base::FilePath dropped = CreateDroppableFile("notes.txt", "FBMD");

  const std::optional<content::DropData> result = PerformDropOf(dropped);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(1u, result->filenames.size());
  EXPECT_EQ(dropped, result->filenames[0].path);
}

TEST_F(DragDropImageMetadataStripperTest, PassesThroughABlockedDrop) {
  EXPECT_FALSE(PerformDrop(std::nullopt).has_value());
}

TEST_F(DragDropImageMetadataStripperDisabledTest, KeepsTheDroppedImage) {
  const base::FilePath dropped = CreateDroppableFbmdImage();

  const std::optional<content::DropData> result = PerformDropOf(dropped);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(1u, result->filenames.size());
  EXPECT_EQ(dropped, result->filenames[0].path);
  EXPECT_TRUE(ContainsFbmd(dropped));
}

}  // namespace brave
