// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/profiles/custom_profile_image/image_file.h"

#include <cstdint>
#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"

namespace custom_profile_image {
namespace {

SkBitmap Bitmap(int side, SkColor color) {
  SkBitmap bitmap;
  bitmap.allocN32Pixels(side, side);
  bitmap.eraseColor(color);
  return bitmap;
}

std::vector<uint8_t> PngHeader(uint32_t width, uint32_t height) {
  std::vector<uint8_t> bytes = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n',
                                0,    0,   0,   13,  'I',  'H',  'D',  'R'};
  for (uint32_t value : {width, height}) {
    for (int shift = 24; shift >= 0; shift -= 8) {
      bytes.push_back(static_cast<uint8_t>(value >> shift));
    }
  }
  return bytes;
}

class CustomProfileImageFileTest : public testing::Test {
 protected:
  void SetUp() override { ASSERT_TRUE(directory_.CreateUniqueTempDir()); }
  base::FilePath path() { return directory_.GetPath(); }
  base::ScopedTempDir directory_;
};

TEST_F(CustomProfileImageFileTest, ReplaceReloadAndRemove) {
  ImageFile file(path());
  ASSERT_TRUE(file.Replace(Bitmap(512, SK_ColorRED)));
  ImageFile reopened(path());
  auto bitmap = reopened.Read();
  ASSERT_FALSE(bitmap.drawsNothing());
  EXPECT_EQ(bitmap.width(), 512);
  EXPECT_EQ(bitmap.getColor(256, 256), SK_ColorRED);
  ASSERT_TRUE(reopened.Replace(Bitmap(16, SK_ColorBLUE)));
  bitmap = file.Read();
  EXPECT_EQ(bitmap.width(), 16);
  EXPECT_EQ(bitmap.getColor(8, 8), SK_ColorBLUE);
  ASSERT_TRUE(file.Remove());
  EXPECT_TRUE(reopened.Read().drawsNothing());
}

TEST_F(CustomProfileImageFileTest, FailedWritePreservesCommittedImage) {
  ImageFile file(path());
  ASSERT_TRUE(file.Replace(Bitmap(16, SK_ColorRED)));
  // Fail after valid bitmap processing by making the staging path unwritable.
  ASSERT_TRUE(base::CreateDirectory(path().AppendASCII(kPendingImageFileName)));
  EXPECT_FALSE(file.Replace(Bitmap(16, SK_ColorBLUE)));
  auto bitmap = file.Read();
  ASSERT_FALSE(bitmap.drawsNothing());
  EXPECT_EQ(bitmap.getColor(8, 8), SK_ColorRED);
}

TEST_F(CustomProfileImageFileTest, FailedReplacementPreservesDestination) {
  const auto destination = path().AppendASCII(kImageFileName);
  ASSERT_TRUE(base::CreateDirectory(destination));
  ASSERT_TRUE(base::WriteFile(destination.AppendASCII("keep"), "existing"));
  ImageFile file(path());
  EXPECT_FALSE(file.Replace(Bitmap(16, SK_ColorRED)));
  EXPECT_FALSE(base::PathExists(path().AppendASCII(kPendingImageFileName)));
}

TEST_F(CustomProfileImageFileTest, MissingProfileIsNotRecreated) {
  const auto missing = path().AppendASCII("deleted");
  ImageFile file(missing);
  EXPECT_FALSE(file.Replace(Bitmap(16, SK_ColorRED)));
  EXPECT_FALSE(base::PathExists(missing));
}

TEST_F(CustomProfileImageFileTest, CorruptImageAndUncommittedFileAreNotLoaded) {
  ASSERT_TRUE(base::WriteFile(path().AppendASCII(kImageFileName), "bad"));
  ASSERT_TRUE(
      base::WriteFile(path().AppendASCII(kPendingImageFileName), "pending"));
  ImageFile file(path());
  EXPECT_TRUE(file.Read().drawsNothing());
  EXPECT_FALSE(base::PathExists(path().AppendASCII(kPendingImageFileName)));
}

TEST(CustomProfileImageHeaderTest, AcceptsOnlySquareHeadersUpToMaxSide) {
  EXPECT_TRUE(internal::HasNormalizedHeader(PngHeader(512, 512)));
  EXPECT_TRUE(internal::HasNormalizedHeader(PngHeader(1, 1)));
  EXPECT_FALSE(internal::HasNormalizedHeader(PngHeader(513, 513)));
  EXPECT_FALSE(
      internal::HasNormalizedHeader(PngHeader(0xffffffff, 0xffffffff)));
  EXPECT_FALSE(internal::HasNormalizedHeader(PngHeader(16, 8)));
  EXPECT_FALSE(internal::HasNormalizedHeader(PngHeader(0, 0)));

  auto truncated = PngHeader(16, 16);
  truncated.pop_back();
  EXPECT_FALSE(internal::HasNormalizedHeader(truncated));
  auto bad_signature = PngHeader(16, 16);
  bad_signature[1] = 'X';
  EXPECT_FALSE(internal::HasNormalizedHeader(bad_signature));
  auto wrong_chunk = PngHeader(16, 16);
  wrong_chunk[12] = 'X';
  EXPECT_FALSE(internal::HasNormalizedHeader(wrong_chunk));
}

}  // namespace
}  // namespace custom_profile_image
