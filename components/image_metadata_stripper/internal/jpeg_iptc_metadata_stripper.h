/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_INTERNAL_JPEG_IPTC_METADATA_STRIPPER_H_
#define BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_INTERNAL_JPEG_IPTC_METADATA_STRIPPER_H_

#include <cstdint>
#include <vector>

namespace image_metadata_stripper::jpeg {

enum class FbmdStripResult {
  kNotFound,
  kRemoved,
  kFailed,
};

// Removes Facebook FBMD metadata from JPEG bytes in-place when present in the
// IPTC record (APP13 / Photoshop IRB with resource id 0x0404). See the
// implementation file for documentation.
FbmdStripResult RemoveFbmdIptcMetadata(std::vector<uint8_t>& jpeg);

}  // namespace image_metadata_stripper::jpeg

#endif  // BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_INTERNAL_JPEG_IPTC_METADATA_STRIPPER_H_
