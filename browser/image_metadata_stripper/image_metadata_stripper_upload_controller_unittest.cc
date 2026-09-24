/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/image_metadata_stripper/image_metadata_stripper_upload_controller.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/test/run_until.h"
#include "base/test/test_future.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave {

namespace {

constexpr char kFbmdImageName[] = "fbmd_test_image.jpg";
constexpr std::string_view kFbmdMarker = "FBMD";

}  // namespace

class ImageMetadataStripperDirControllerTest
    : public ChromeRenderViewHostTestHarness {
 public:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(source_dir_.CreateUniqueTempDir());
    controller_ =
        std::make_unique<ImageMetadataStripperUploadController>(web_contents());
  }

  void TearDown() override {
    controller_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

 protected:
  base::FilePath CreateFbmdImage() {
    const base::FilePath test_image =
        base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
            .AppendASCII("brave/test/data/image_metadata_stripper")
            .AppendASCII(kFbmdImageName);
    const base::FilePath copy =
        source_dir_.GetPath().AppendASCII(kFbmdImageName);
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_TRUE(base::CopyFile(test_image, copy));
    return copy;
  }

  base::FilePath CreateFile(std::string_view name, std::string_view contents) {
    const base::FilePath path = source_dir_.GetPath().AppendASCII(name);
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_TRUE(base::WriteFile(path, contents));
    return path;
  }

  std::vector<std::optional<base::FilePath>> Strip(
      std::vector<base::FilePath> files) {
    base::test::TestFuture<std::vector<std::optional<base::FilePath>>> future;
    controller_->MaybeStrip(
        image_metadata_stripper::StrippingClient::kFileSelect, std::move(files),
        future.GetCallback());
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

  std::unique_ptr<ImageMetadataStripperUploadController> controller_;
  base::ScopedTempDir source_dir_;
};

TEST_F(ImageMetadataStripperDirControllerTest,
       CreatesRootBeforeStripAndReusesIt) {
  const base::FilePath fbmd = CreateFbmdImage();
  ASSERT_TRUE(ContainsFbmd(fbmd));
  EXPECT_TRUE(controller_->GetTempRootDirForTesting().empty());

  const auto first = Strip({fbmd});
  ASSERT_EQ(1u, first.size());
  ASSERT_TRUE(first[0]);
  EXPECT_NE(fbmd, *first[0]);
  EXPECT_FALSE(ContainsFbmd(*first[0]));
  EXPECT_TRUE(ContainsFbmd(fbmd));
  EXPECT_EQ(fbmd.BaseName(), first[0]->BaseName());

  const base::FilePath root = controller_->GetTempRootDirForTesting();
  ASSERT_FALSE(root.empty());
  EXPECT_TRUE(PathExists(root));

  const auto second = Strip({fbmd});
  ASSERT_EQ(1u, second.size());
  ASSERT_TRUE(second[0]);
  EXPECT_NE(*first[0], *second[0]);
  EXPECT_EQ(root, controller_->GetTempRootDirForTesting());
  EXPECT_TRUE(PathExists(*first[0]));
}

TEST_F(ImageMetadataStripperDirControllerTest,
       DoesNotCreateRootWhenNothingToStrip) {
  const auto result = Strip({CreateFile("notes.txt", "FBMD")});
  ASSERT_EQ(1u, result.size());
  EXPECT_FALSE(result[0]);
  EXPECT_TRUE(controller_->GetTempRootDirForTesting().empty());
}

TEST_F(ImageMetadataStripperDirControllerTest, DoesNotCreateRootForCleanJpeg) {
  // Minimal valid JPEG: SOI + COM + EOI, no FBMD record.
  const std::vector<uint8_t> clean_jpeg = {
      0xFF, 0xD8, 0xFF, 0xFE, 0x00, 0x10, 'N', 'O', '_', 'M',  'E',
      'T',  'A',  '_',  'M',  'A',  'R',  'K', 'E', 'R', 0xFF, 0xD9,
  };
  const base::FilePath clean = CreateFile(
      "clean.jpg",
      std::string_view(reinterpret_cast<const char*>(clean_jpeg.data()),
                       clean_jpeg.size()));

  const auto result = Strip({clean});
  ASSERT_EQ(1u, result.size());
  EXPECT_FALSE(result[0]);
  EXPECT_TRUE(controller_->GetTempRootDirForTesting().empty());
}

TEST_F(ImageMetadataStripperDirControllerTest,
       AlignsResultsWithInputIncludingSkippedEntries) {
  const base::FilePath fbmd = CreateFbmdImage();
  const auto result = Strip({
      base::FilePath(),
      fbmd,
      CreateFile("notes.txt", "plain"),
  });

  ASSERT_EQ(3u, result.size());
  EXPECT_FALSE(result[0]);
  ASSERT_TRUE(result[1]);
  EXPECT_FALSE(ContainsFbmd(*result[1]));
  EXPECT_FALSE(result[2]);
}

TEST_F(ImageMetadataStripperDirControllerTest, DeletesRootOnDestruction) {
  const auto result = Strip({CreateFbmdImage()});
  ASSERT_EQ(1u, result.size());
  ASSERT_TRUE(result[0]);
  const base::FilePath root = controller_->GetTempRootDirForTesting();
  ASSERT_TRUE(PathExists(root));

  controller_.reset();
  EXPECT_TRUE(base::test::RunUntil([&]() { return !PathExists(root); }));
}

TEST_F(ImageMetadataStripperDirControllerTest,
       DeletesRootWhenPrimaryPageIsDestroyed) {
  const auto first = Strip({CreateFbmdImage()});
  ASSERT_EQ(1u, first.size());
  ASSERT_TRUE(first[0]);
  const base::FilePath root = controller_->GetTempRootDirForTesting();
  ASSERT_TRUE(PathExists(root));

  NavigateAndCommit(GURL("https://example.com/"));
  EXPECT_TRUE(base::test::RunUntil([&]() { return !PathExists(root); }));
  EXPECT_TRUE(controller_->GetTempRootDirForTesting().empty());

  const auto second = Strip({CreateFbmdImage()});
  ASSERT_EQ(1u, second.size());
  ASSERT_TRUE(second[0]);
  const base::FilePath new_root = controller_->GetTempRootDirForTesting();
  ASSERT_FALSE(new_root.empty());
  EXPECT_NE(root, new_root);
}

}  // namespace brave
