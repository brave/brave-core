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
#include "base/numerics/byte_conversions.h"
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

// Valid short FBMD ASCII record: marker + type + field_count=0 + one field.
constexpr char kValidFbmdRecord[] = "FBMD01000012345678";

bool ContainsFbmd(base::span<const uint8_t> data) {
  return base::as_string_view(data).find("FBMD") != std::string_view::npos;
}

void AppendSpan(std::vector<uint8_t>& out, base::span<const uint8_t> bytes) {
  out.insert(out.end(), bytes.begin(), bytes.end());
}

void AppendString(std::vector<uint8_t>& out, std::string_view s) {
  AppendSpan(out, base::as_byte_span(s));
}

void AppendU16(std::vector<uint8_t>& out, uint16_t value) {
  AppendSpan(out, base::U16ToBigEndian(value));
}

void AppendU32(std::vector<uint8_t>& out, uint32_t value) {
  AppendSpan(out, base::U32ToBigEndian(value));
}

void AppendPhotoshopHeader(std::vector<uint8_t>& payload) {
  constexpr uint8_t kHeader[] = {'P', 'h', 'o', 't', 'o', 's', 'h',
                                 'o', 'p', ' ', '3', '.', '0', 0x00};
  AppendSpan(payload, base::span(kHeader));
}

// Appends one IRB. `pascal_name` is the name chars only (length byte + pad
// added here). Data is followed by a pad byte when `data.size()` is odd.
void AppendIrb(std::vector<uint8_t>& payload,
               uint16_t resource_id,
               base::span<const uint8_t> data,
               std::string_view pascal_name = {},
               bool include_data_pad = true) {
  AppendString(payload, "8BIM");
  AppendU16(payload, resource_id);

  const uint8_t name_len = static_cast<uint8_t>(pascal_name.size());
  payload.push_back(name_len);
  AppendString(payload, pascal_name);
  if ((1u + name_len) % 2u != 0u) {
    payload.push_back(0x00);
  }

  AppendU32(payload, static_cast<uint32_t>(data.size()));
  AppendSpan(payload, data);
  if (include_data_pad && (data.size() % 2u != 0u)) {
    payload.push_back(0x00);
  }
}

std::vector<uint8_t> WrapApp13(base::span<const uint8_t> app13_payload) {
  std::vector<uint8_t> jpeg = {0xFF, 0xD8, 0xFF, 0xED};
  AppendU16(jpeg, static_cast<uint16_t>(app13_payload.size() + 2u));
  AppendSpan(jpeg, app13_payload);
  jpeg.push_back(0xFF);
  jpeg.push_back(0xD9);
  return jpeg;
}

std::vector<uint8_t> JpegWithIptcPayload(std::string_view iptc,
                                         bool include_data_pad = true) {
  std::vector<uint8_t> payload;
  AppendPhotoshopHeader(payload);
  AppendIrb(payload, 0x0404, base::as_byte_span(iptc), /*pascal_name=*/{},
            include_data_pad);
  return WrapApp13(payload);
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

TEST_F(JpegIptcMetadataStripperTest, StripsFbmdAfterNonIptcIrb) {
  std::vector<uint8_t> payload;
  AppendPhotoshopHeader(payload);
  // Non-IPTC resource (e.g. 0x0400) with odd-sized data + pad, then IPTC/FBMD.
  const uint8_t other_data[] = {'x'};
  AppendIrb(payload, 0x0400, base::span(other_data));
  AppendIrb(payload, 0x0404,
            base::as_byte_span(std::string_view(kValidFbmdRecord)));

  std::vector<uint8_t> jpeg = WrapApp13(payload);
  ASSERT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kRemoved);
  EXPECT_FALSE(ContainsFbmd(jpeg));
}

TEST_F(JpegIptcMetadataStripperTest, StripsFbmdWithNonEmptyPascalName) {
  std::vector<uint8_t> payload;
  AppendPhotoshopHeader(payload);
  AppendIrb(payload, 0x0404,
            base::as_byte_span(std::string_view(kValidFbmdRecord)),
            "n");  // name_len=1 → name field is 2 bytes (len + 'n')

  std::vector<uint8_t> jpeg = WrapApp13(payload);
  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kRemoved);
  EXPECT_FALSE(ContainsFbmd(jpeg));
}

// --- Malformed / truncated inputs: must not crash; return kNotFound. ---

TEST_F(JpegIptcMetadataStripperTest,
       ReturnsNotFoundForIrbTruncatedAfterResourceId) {
  // APP13 payload ends exactly after "8BIM" + resource id (6 bytes), with no
  // Pascal name length byte.
  std::vector<uint8_t> payload;
  AppendPhotoshopHeader(payload);
  AppendString(payload, "8BIM");
  AppendU16(payload, 0x0404);

  std::vector<uint8_t> jpeg = WrapApp13(payload);
  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kNotFound);
}

TEST_F(JpegIptcMetadataStripperTest,
       ReturnsNotFoundForPascalNameExtendingPastPayload) {
  std::vector<uint8_t> payload;
  AppendPhotoshopHeader(payload);
  AppendString(payload, "8BIM");
  AppendU16(payload, 0x0404);
  payload.push_back(0x14);  // name_len=20, but only a few bytes remain
  AppendString(payload, "short");

  std::vector<uint8_t> jpeg = WrapApp13(payload);
  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kNotFound);
}

TEST_F(JpegIptcMetadataStripperTest,
       ReturnsNotFoundForIrbDataLengthExtendingPastPayload) {
  std::vector<uint8_t> payload;
  AppendPhotoshopHeader(payload);
  AppendString(payload, "8BIM");
  AppendU16(payload, 0x0404);
  payload.push_back(0x00);  // empty Pascal name (len) + pad
  payload.push_back(0x00);
  AppendU32(payload, 0x100);  // claims 256 bytes of data
  AppendString(payload, "only-a-few-bytes");

  std::vector<uint8_t> jpeg = WrapApp13(payload);
  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kNotFound);
}

TEST_F(JpegIptcMetadataStripperTest,
       ReturnsNotFoundWhenNonIptcIrbMissingDataPad) {
  // Odd-sized non-IPTC data without the required pad byte, then another IRB
  // prefix that must not be walked off the end.
  std::vector<uint8_t> payload;
  AppendPhotoshopHeader(payload);
  const uint8_t other_data[] = {'x'};
  AppendIrb(payload, 0x0400, base::span(other_data), /*pascal_name=*/{},
            /*include_data_pad=*/false);
  AppendString(payload, "8BIM");
  AppendU16(payload, 0x0404);

  std::vector<uint8_t> jpeg = WrapApp13(payload);
  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kNotFound);
}

TEST_F(JpegIptcMetadataStripperTest, ReturnsNotFoundWithoutPhotoshopHeader) {
  std::vector<uint8_t> payload;
  AppendString(payload, "NotPhotoshop!!");
  AppendIrb(payload, 0x0404,
            base::as_byte_span(std::string_view(kValidFbmdRecord)));

  std::vector<uint8_t> jpeg = WrapApp13(payload);
  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kNotFound);
}

TEST_F(JpegIptcMetadataStripperTest, ReturnsNotFoundForWrongBimSignature) {
  std::vector<uint8_t> payload;
  AppendPhotoshopHeader(payload);
  AppendString(payload, "9BIM");  // not "8BIM"
  AppendU16(payload, 0x0404);

  std::vector<uint8_t> jpeg = WrapApp13(payload);
  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kNotFound);
}

TEST_F(JpegIptcMetadataStripperTest, ReturnsNotFoundForApp13TruncatedByLength) {
  // Declared APP13 length extends past the end of the file.
  std::vector<uint8_t> jpeg = {0xFF, 0xD8, 0xFF, 0xED, 0x00, 0x40};
  AppendPhotoshopHeader(jpeg);
  jpeg.push_back(0xFF);
  jpeg.push_back(0xD9);
  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kNotFound);
}

TEST_F(JpegIptcMetadataStripperTest, ReturnsNotFoundForApp13LengthLessThanTwo) {
  std::vector<uint8_t> jpeg = {0xFF, 0xD8, 0xFF, 0xED, 0x00, 0x01, 0xFF, 0xD9};
  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kNotFound);
}

TEST_F(JpegIptcMetadataStripperTest, ReturnsNotFoundForCoincidentalFfEdBytes) {
  // FF ED appears in what looks like image bytes, but is not a usable APP13.
  std::vector<uint8_t> jpeg = {0xFF, 0xD8, 0x00, 0xFF, 0xED,
                               0x00, 0x02, 0xFF, 0xD9};
  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kNotFound);
}

TEST_F(JpegIptcMetadataStripperTest, ReturnsNotFoundForTruncatedFbmdHeader) {
  // IPTC contains "FBMD" but not enough bytes for type + field_count.
  std::vector<uint8_t> jpeg = JpegWithIptcPayload("FBMD01");
  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kNotFound);
}

TEST_F(JpegIptcMetadataStripperTest,
       ReturnsNotFoundForInvalidFbmdFieldCountHex) {
  // field_count nibble is not valid hex ('G').
  std::vector<uint8_t> jpeg = JpegWithIptcPayload("FBMD01G00012345678");
  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kNotFound);
}

TEST_F(JpegIptcMetadataStripperTest,
       ReturnsNotFoundWhenFbmdFieldCountExceedsIptc) {
  // field_count=0x000A claims 11 fields (88 ASCII chars) but only one field
  // is present.
  std::vector<uint8_t> jpeg = JpegWithIptcPayload("FBMD01000A12345678");
  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kNotFound);
}

TEST_F(JpegIptcMetadataStripperTest, ReturnsNotFoundWhenIptcHasNoFbmd) {
  std::vector<uint8_t> jpeg = JpegWithIptcPayload("not-a-tracking-record");
  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kNotFound);
}

TEST_F(JpegIptcMetadataStripperTest, ReturnsNotFoundForApp13WithHeaderOnly) {
  std::vector<uint8_t> payload;
  AppendPhotoshopHeader(payload);
  std::vector<uint8_t> jpeg = WrapApp13(payload);
  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kNotFound);
}

TEST_F(JpegIptcMetadataStripperTest,
       ReturnsNotFoundForTruncatedDataLengthField) {
  std::vector<uint8_t> payload;
  AppendPhotoshopHeader(payload);
  AppendString(payload, "8BIM");
  AppendU16(payload, 0x0404);
  payload.push_back(0x00);  // empty name + pad
  payload.push_back(0x00);
  // Only 2 of 4 data_length bytes.
  payload.push_back(0x00);
  payload.push_back(0x10);

  std::vector<uint8_t> jpeg = WrapApp13(payload);
  EXPECT_EQ(RemoveFbmdIptcMetadata(jpeg), FbmdStripResult::kNotFound);
}

}  // namespace
}  // namespace image_metadata_stripper::jpeg
