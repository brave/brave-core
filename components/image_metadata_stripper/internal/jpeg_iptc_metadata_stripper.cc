/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/image_metadata_stripper/internal/jpeg_iptc_metadata_stripper.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <vector>

#include "base/containers/span.h"
#include "base/numerics/byte_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_view_util.h"

namespace image_metadata_stripper::jpeg {

namespace {

// ---------------------------------------------------------------------------
// JPEG APP13 / Photoshop IRB / IPTC / Facebook FBMD layout
// ---------------------------------------------------------------------------
//
// Author's references:
// - JPEG markers:
//   (1) https://en.wikipedia.org/wiki/JPEG#Syntax_and_structure: Captures the
//   structure of the JPEG File.
//   (2)
//   https://koushtav.me/jpeg/tutorial/c++/decoder/2019/03/02/lets-write-a-simple-jpeg-library-part-2:
//   This is a really nice blog which writes a custom JPEG parser and talks
//   about the JPEG markers and APP0 layout.
//
// - Photoshop Image Resource Blocks (IRB) in APP13:
//   (3) https://help.accusoft.com/ImageGear/v17.2/Windows/DLL/topic756.html:
//   "Photoshop Image Resource APP13 Marker Segment" section outlines the APP13
//   layout.
//   (4)
//   https://www.adobe.com/devnet-apps/photoshop/fileformatashtml/#50577409_46269:
//   Outlines the layout of the IRB (Image resource block) inside the APP13
//   segment payload and the IPTC image resource id "0x0404".
//
// - Facebook FBMD tracking string in IPTC Special Instructions:
//   (5)
//   https://www.hackerfactor.com/blog/index.php?/archives/726-Facebook-Tracking.html
//   - Outlines the FBMD payload layout to be able to parse the jpeg correctly.
//   (6) https://iptc.org/news/what-does-facebook-do-with-your-photo-metadata/ -
//   Outlines FBMD stored inside the special instructions field in the IPTC.
//
// Nested on-disk layout (big-endian multi-byte integers unless noted):
//
//   JPEG bytes
//   +--------------------------------------------------------------------+
//   |  ... other segments ...                                            |
//   |                                                                    |
//   |  APP13 segment                                                     |
//   |  +--------+----------------+-------------------------------------+ |
//   |  | FF ED  | len_hi len_lo  | payload                             | |
//   |  | marker | BE uint16      |                                     | |
//   |  +--------+----------------+-------------------------------------+ |
//   |            ^-- length includes these 2 length bytes, not FF ED     |
//   |                                                                    |
//   |  APP13 payload                                                     |
//   |  +------------------+--------------------------------------------+ |
//   |  | "Photoshop 3.0\0"| Image Resource Block(s)                    | |
//   |  | 14 bytes         |                                            | |
//   |  +------------------+--------------------------------------------+ |
//   |                                                                    |
//   |  One Image Resource Block (IRB)                                    |
//   |  +------+-----------+-------------+----------+-------------------+ |
//   |  | 8BIM | res_id    | Pascal name | data_len | data (+ pad)      | |
//   |  | 4B   | BE uint16 | even-sized  | BE uint32| even total size   | |
//   |  +------+-----------+-------------+----------+-------------------+ |
//   |            ^-- IPTC-NAA is resource id 0x0404                      |
//   |                                                                    |
//   |  IPTC data (may have a short binary prefix) then FBMD ASCII record |
//   |  +------+------------+----------------+--------------------------+ |
//   |  | FBMD | type       | field_count    | fields                   | |
//   |  | 4B   | 2 hex ASCII| 4 hex ASCII    | (field_count+1) x 8 ASCII| |
//   |  +------+------------+----------------+--------------------------+ |
//   +--------------------------------------------------------------------+
//
// Pascal name: This is padding strategy to keep the bytes even. 1-byte length
// N, then N chars, then 0/1 pad so (1+N) is even. Empty name is common: bytes
// 00 00.
//
// Example APP13 marker + Photoshop IRB header (hex):
//   FF ED | 00 84 | 50 68 6F 74 6F 73 68 6F 70 20 33 2E 30 00 | 38 42 49 4D |
//   04 04 | ... marker  length  P  h  o  t  o  s  h  o  p     3  .  0  \0   8
//   B  I  M   0x0404
//
// Example FBMD record (ASCII hex characters stored "verbatim" in IPTC):
//   FBMD 01 000a c6030000 4a1d0000 2d4b0000 67580000 c9650000 d5fc0000
//              54350100 953a0100 d3420100 e84b0100 5f8f0100
//   |    |  |     `---- 11 fields of 8 hex chars when field_count=0x000a ----'
//   |    |  `---- field_count (4 hex ASCII chars) = 10
//   |    `---- type (2 hex ASCII chars)
//   `---- marker
//
// Algorithm implemented below:
// 1. Byte-scan the JPEG for candidate APP13 markers (0xFF 0xED). This is not a
//    full JPEG segment walk, so coincidental FF ED bytes may appear in other
//    payloads; those are rejected by the validation chain below.
// 2. Read the big-endian segment length at offset +2. Reject truncated or
//    lengths < 2. Payload is the following (length - 2) bytes.
// 3. Require the Photoshop IRB identifier "Photoshop 3.0\0" at the start of
//    the APP13 payload.
// 4. Walk IRBs ("8BIM" + id + Pascal name + data). Keep only resource id
//    0x0404 (IPTC). Skip other resources using their padded sizes.
// 5. Search the IPTC bytes for a well-formed "FBMD" ASCII record and compute
//    its size as: 4 + 2 + 4 + (field_count + 1) * 8.
// 6. Zero that FBMD record in place. APP13 / IRB / IPTC length fields are left
//    unchanged, so the surrounding structure stays parseable.
// ---------------------------------------------------------------------------

// APP13 segment header:
//   [kMarkerPrefix][kApp13Marker][len_hi][len_lo][payload...]
//    0xFF           0xED          <--- BE uint16 --->
//   APP0=E0, APP1=E1, ... APP13=ED.
constexpr uint8_t kMarkerPrefix = 0xFF;  // 1 byte
constexpr uint8_t kApp13Marker = 0xED;   // 1 byte

// APP13 payload / IRB:
//   [kPhotoshopHeader        ][k8BimSignature][kIptcResourceId][Pascal
//   name][data_len][data...]
//    "P h o t o s h o p   3 . 0 \0"
//     0 1 2 3 4 5 6 7 8 9 10 11 12 13   "8 B I M"   0x04 0x04
//     <--------- 14 bytes --------->    <- 4B ->   <-- 2B -->
constexpr std::string_view kPhotoshopHeader("Photoshop 3.0\0",
                                            14);     // 14 bytes with NULL.
constexpr std::string_view k8BimSignature = "8BIM";  // 4 bytes
constexpr uint16_t kIptcResourceId = 0x0404;         // 2 bytes

// FBMD ASCII record inside IPTC data:
//   [kFbmdMarker][type][field_count][field0  ][field1  ]...
//    F B M D      0 1   0 0 0 a      c 6 0 3 0 0 0 0 ...
//    <-- 4B -->  <-2->  <--- 4 --->  <------ 8 ------>
//
//   kFbmdMarkerChars   = 4  (ASCII marker "FBMD")
//   kFbmdTypeChars     = 2  (ASCII hex chars for the type byte)
//   kFbmdLengthChars   = 4  (ASCII hex chars for field_count)
//   kFbmdFieldChars    = 8  (ASCII hex chars per 32-bit field)
//   kFbmdHeaderSize    = 4 + 2 + 4 = 10
//   kFbmdLengthOffset  = 4 + 2 = 6  (start of field_count)
//   record size        = kFbmdHeaderSize + (field_count + 1) * kFbmdFieldChars
constexpr std::string_view kFbmdMarker = "FBMD";  // 4 bytes
constexpr size_t kFbmdMarkerChars = 4u;
constexpr size_t kFbmdTypeChars = 2u;
constexpr size_t kFbmdLengthChars = 4u;
constexpr size_t kFbmdFieldChars = 8u;
constexpr size_t kFbmdHeaderSize =
    kFbmdMarkerChars + kFbmdTypeChars + kFbmdLengthChars;
constexpr size_t kFbmdLengthOffset = kFbmdMarkerChars + kFbmdTypeChars;
static_assert(kFbmdMarker.size() == kFbmdMarkerChars);

// Reads a big-endian uint16 at `offset` in `data`. Returns false if truncated.
bool ReadU16(base::span<const uint8_t> data, size_t offset, uint16_t& out) {
  if (offset + 2u > data.size()) {
    return false;
  }
  out = base::U16FromBigEndian(data.subspan(offset).first<2u>());
  return true;
}

// Reads a big-endian uint32 at `offset` in `data`. Returns false if truncated.
bool ReadU32(base::span<const uint8_t> data, size_t offset, uint32_t& out) {
  if (offset + 4u > data.size()) {
    return false;
  }
  out = base::U32FromBigEndian(data.subspan(offset).first<4u>());
  return true;
}

// Returns the IPTC (0x0404) resource data within an APP13 `payload`, or an
// empty span if not present.
base::span<const uint8_t> FindIptcData(base::span<const uint8_t> payload) {
  // 1. Look for the Photoshop header string.
  if (payload.size() < kPhotoshopHeader.size() ||
      base::as_string_view(payload.first(kPhotoshopHeader.size())) !=
          kPhotoshopHeader) {
    return {};
  }

  size_t pos = kPhotoshopHeader.size();

  // 2. Iterate over each Image resource blocks.
  // Each resource block: "8BIM"(4) + id (2) + Pascal name (variable) + length
  // (4) + data. See reference (4).
  while (pos + 6u <= payload.size()) {
    // 1. Search for "8BIM" signature.
    if (base::as_string_view(payload.subspan(pos, k8BimSignature.size())) !=
        k8BimSignature) {
      break;
    }
    pos += k8BimSignature.size();

    // 2. Look for "0x0404" resource id corresponding to IPTC metadata.
    uint16_t resource_id = 0;
    if (!ReadU16(payload, pos, resource_id)) {
      break;
    }
    pos += 2u;

    if (pos >= payload.size()) {
      break;
    }

    // 3. Pascal name: 1-byte length + bytes, padded to an even total size.
    const uint8_t name_length = payload[pos];
    // 1u is accounting for the actual byte which contained the name_length.
    size_t name_field = 1u + name_length;
    name_field += name_field % 2u;  // Pad to even.
    if (pos + name_field > payload.size()) {
      break;
    }
    pos += name_field;  // pos now points to the 4-byte holding the length of
                        // the IRB data.

    // 4. Read the length.
    uint32_t data_length = 0;
    if (!ReadU32(payload, pos, data_length)) {
      break;
    }
    // Now pos is pointing to the beginning of the actual resource
    // block.
    pos += 4u;

    if (pos + data_length > payload.size()) {
      break;
    }

    // 5. Return the data block associated with the IRB if it was for
    // IPTC.
    if (resource_id == kIptcResourceId) {
      return payload.subspan(pos, data_length);
    }
    // Otherwise, move to the next IRB block (data padded to even).
    const size_t skip = data_length + (data_length % 2u);
    if (pos + skip > payload.size()) {
      break;
    }
    pos += skip;
  }
  return {};
}

// Returns the span of the first valid "FBMD" record within `iptc`, or empty.
base::span<const uint8_t> FindFbmdRecord(base::span<const uint8_t> iptc) {
  // 1. Find the offset of the block corresponding to "FBMD".
  const auto match = std::ranges::search(iptc, base::as_byte_span(kFbmdMarker));
  if (match.empty()) {
    return {};
  }

  const size_t offset =
      static_cast<size_t>(std::distance(iptc.begin(), match.begin()));
  base::span<const uint8_t> record = iptc.subspan(offset);
  if (record.size() < kFbmdHeaderSize) {
    return {};
  }

  // 2. Get the length of the FBMD data block.
  const std::string_view length_hex =
      base::as_string_view(record.subspan(kFbmdLengthOffset, kFbmdLengthChars));
  // The |fbmd_data_chunks_count| is encoded as 4 hex characters after the type.
  uint32_t fbmd_data_chunks_count = 0;
  if (!base::HexStringToUInt(length_hex, &fbmd_data_chunks_count)) {
    return {};
  }

  // Payload is (fbmd_data_chunks_count + 1) × 8 ASCII hex chars.
  const size_t value_chars =
      (static_cast<size_t>(fbmd_data_chunks_count) + 1u) * kFbmdFieldChars;
  const size_t record_size = kFbmdHeaderSize + value_chars;
  if (record_size > record.size()) {
    return {};
  }
  return record.first(record_size);
}

// Scans for APP13 (0xFFED) candidates and returns the first FBMD record found
// via APP13 → Photoshop IRB → IPTC (0x0404).
base::span<const uint8_t> FindFbmdInJpeg(base::span<const uint8_t> jpeg) {
  for (size_t i = 0; i + 4u <= jpeg.size(); ++i) {
    // 1. Search for the App13 marker
    if (jpeg[i] != kMarkerPrefix || jpeg[i + 1u] != kApp13Marker) {
      continue;
    }
    // 2. Extract the App13's segment length.
    // Segment length is a big-endian uint16 at i+2 (high) / i+3 (low), and
    // includes those two length bytes.
    uint16_t segment_length = 0;
    if (!ReadU16(jpeg, i + 2u, segment_length) || segment_length < 2u) {
      continue;
    }
    const size_t payload_offset = i + 4u;
    const size_t payload_length = segment_length - 2u;
    if (payload_offset + payload_length > jpeg.size()) {
      continue;
    }

    // 3. Check for the presence of IPTC data.
    const base::span<const uint8_t> app13 =
        jpeg.subspan(payload_offset, payload_length);
    const base::span<const uint8_t> iptc = FindIptcData(app13);
    if (iptc.empty()) {
      continue;
    }

    // 4. Finally, check for the presence of the "FBMD" record.
    const base::span<const uint8_t> record = FindFbmdRecord(iptc);
    if (!record.empty()) {
      return record;
    }
  }
  return {};
}

bool IsFbmdPresent(base::span<const uint8_t> jpeg) {
  return !FindFbmdInJpeg(jpeg).empty();
}

bool RemoveFbmd(std::vector<uint8_t>& jpeg) {
  base::span<const uint8_t> data(jpeg);
  base::span<const uint8_t> record = FindFbmdInJpeg(data);
  if (record.empty()) {
    return false;
  }

  // Zero the entire FBMD record in place. Because the bytes are overwritten
  // rather than removed, the enclosing APP13 and IPTC length fields remain
  // valid and require no updating.

  // Use raw pointers: `record` is a nested subspan, so checked span iterators
  // from different spans cannot be distance(). This triggers a DCHECK in tests.
  const size_t record_offset = static_cast<size_t>(record.data() - data.data());
  std::ranges::fill(base::span(jpeg).subspan(record_offset, record.size()),
                    uint8_t{0});
  return true;
}

}  // namespace

FbmdStripResult RemoveFbmdIptcMetadata(std::vector<uint8_t>& jpeg) {
  if (!IsFbmdPresent(jpeg)) {
    return FbmdStripResult::kNotFound;
  }

  return RemoveFbmd(jpeg) ? FbmdStripResult::kRemoved
                          : FbmdStripResult::kFailed;
}

}  // namespace image_metadata_stripper::jpeg
