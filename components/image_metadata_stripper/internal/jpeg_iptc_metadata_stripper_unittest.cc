/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/image_metadata_stripper/internal/jpeg_iptc_metadata_stripper.h"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "base/base_paths.h"
#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string_view_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace image_metadata_stripper::jpeg {
namespace {

// Minimal JPEG with APP13 → Photoshop IRB → IPTC Special Instructions
// containing a short FBMD record. Layout:
//   SOI | APP13(Photoshop 3.0 + 8BIM/0x0404 + IPTC 1C 02 28 "FBMD...") | EOI
constexpr uint8_t kJpegWithFbmd[] = {
    0xFF,
    0xD8,  // SOI
    0xFF,
    0xED,
    0x00,
    0x34,  // APP13, segment length = 52
    // "Photoshop 3.0\0"
    0x50,
    0x68,
    0x6F,
    0x74,
    0x6F,
    0x73,
    0x68,
    0x6F,
    0x70,
    0x20,
    0x33,
    0x2E,
    0x30,
    0x00,
    // 8BIM resource: id 0x0404, empty Pascal name, data length 0x17
    0x38,
    0x42,
    0x49,
    0x4D,
    0x04,
    0x04,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x17,
    // IPTC Special Instructions (1C 02 28), length 18, payload
    // FBMD01000012345678
    0x1C,
    0x02,
    0x28,
    0x00,
    0x12,
    0x46,
    0x42,
    0x4D,
    0x44,
    0x30,
    0x31,
    0x30,
    0x30,
    0x30,
    0x30,
    0x31,
    0x32,
    0x33,
    0x34,
    0x35,
    0x36,
    0x37,
    0x38,
    0x00,  // IRB data pad (odd length)
    0xFF,
    0xD9,  // EOI
};

bool ContainsFbmd(base::span<const uint8_t> data) {
  return base::as_string_view(data).find("FBMD") != std::string_view::npos;
}

class JpegIptcMetadataStripperTest : public testing::Test {
 protected:
  static std::vector<uint8_t> JpegWithFbmd() {
    return std::vector<uint8_t>(std::begin(kJpegWithFbmd),
                                std::end(kJpegWithFbmd));
  }

  // brave_components_unittests does not register brave::DIR_TEST_DATA (that
  // comes from chrome's path provider), so resolve via the src root.
  static base::FilePath GetTestDataDir() {
    return base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
        .AppendASCII("brave/test/data/image_metadata_stripper");
  }
};

TEST_F(JpegIptcMetadataStripperTest, StripsFbmdFromJpegBytes) {
  std::vector<uint8_t> jpeg = JpegWithFbmd();
  ASSERT_TRUE(ContainsFbmd(jpeg));
  const size_t original_size = jpeg.size();

  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kRemoved);
  EXPECT_FALSE(ContainsFbmd(jpeg));
  EXPECT_EQ(jpeg.size(), original_size);

  // Second pass should find nothing.
  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kNotFound);
}

TEST_F(JpegIptcMetadataStripperTest, StripsFbmdFromTestImage) {
  const base::FilePath input_path =
      GetTestDataDir().AppendASCII("fbmd_test_image.jpg");
  std::optional<std::vector<uint8_t>> jpeg = base::ReadFileToBytes(input_path);
  ASSERT_TRUE(jpeg.has_value());
  ASSERT_FALSE(jpeg->empty());
  ASSERT_TRUE(ContainsFbmd(*jpeg));

  const size_t original_size = jpeg->size();
  EXPECT_EQ(RemoveFbmdIptcMetadata(*jpeg), FbmdStripResult::kRemoved);
  EXPECT_FALSE(ContainsFbmd(*jpeg));
  EXPECT_EQ(jpeg->size(), original_size);
  EXPECT_EQ(RemoveFbmdIptcMetadata(*jpeg), FbmdStripResult::kNotFound);
}

TEST_F(JpegIptcMetadataStripperTest, ReturnsNotFoundWhenFbmdAbsent) {
  // Minimal JPEG: SOI + EOI, no APP13 / IPTC / FBMD.
  std::vector<uint8_t> jpeg = {0xFF, 0xD8, 0xFF, 0xD9};
  EXPECT_FALSE(ContainsFbmd(jpeg));
  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kNotFound);
}

TEST_F(JpegIptcMetadataStripperTest, ReturnsNotFoundForEmptyInput) {
  std::vector<uint8_t> jpeg;
  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kNotFound);
}

}  // namespace
}  // namespace image_metadata_stripper::jpeg
