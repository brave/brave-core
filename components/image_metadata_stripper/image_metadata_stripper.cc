/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"

#include <cstdint>
#include <optional>
#include <vector>

#include "base/containers/span.h"
#include "base/containers/to_vector.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "brave/components/image_metadata_stripper/rs/src/lib.rs.h"

namespace image_metadata_stripper {

bool RemoveIptcMetadata(const base::FilePath& file_path) {
  if (!base::PathExists(file_path)) {
    DVLOG(1) << "IPTC strip skipped; file missing: " << file_path;
    return false;
  }

  std::optional<std::vector<uint8_t>> file_bytes =
      base::ReadFileToBytes(file_path);
  if (!file_bytes.has_value()) {
    DVLOG(1) << "IPTC strip failed; could not read: " << file_path;
    return false;
  }

  std::vector<uint8_t> stripped = base::ToVector(
      remove_iptc_metadata(rust::Slice<const uint8_t>(*file_bytes)));
  if (stripped.empty()) {
    // Unrecognized/invalid image bytes — leave the file unchanged.
    DVLOG(1) << "IPTC strip skipped; unrecognized image: " << file_path;
    return true;
  }

  if (stripped == *file_bytes) {
    return true;
  }

  if (!base::WriteFile(file_path, base::as_byte_span(stripped))) {
    DVLOG(1) << "IPTC strip failed; could not write: " << file_path;
    return false;
  }

  return true;
}

}  // namespace image_metadata_stripper
