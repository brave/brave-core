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

namespace {

bool HasSupportedImageExtension(const base::FilePath& path) {
  static constexpr const base::FilePath::CharType* kExtensions[] = {
      FILE_PATH_LITERAL(".jpg"),  FILE_PATH_LITERAL(".jpeg"),
      FILE_PATH_LITERAL(".jpe"),  FILE_PATH_LITERAL(".png"),
      FILE_PATH_LITERAL(".webp"), FILE_PATH_LITERAL(".tif"),
      FILE_PATH_LITERAL(".tiff"),
  };
  const base::FilePath::StringType ext = path.FinalExtension();
  for (const auto* supported : kExtensions) {
    if (base::FilePath::CompareEqualIgnoreCase(ext, supported)) {
      return true;
    }
  }
  return false;
}

bool IsSupportedImageFile(const base::FilePath& path) {
  return !path.empty() && HasSupportedImageExtension(path);
}

}  // namespace

void RemoveIptcMetadata(const base::FilePath& file_path) {
  if (!IsSupportedImageFile(file_path)) {
    return;
  }

  if (!base::PathExists(file_path)) {
    DVLOG(1) << "IPTC strip skipped; file missing: " << file_path;
    return;
  }

  std::optional<std::vector<uint8_t>> file_bytes =
      base::ReadFileToBytes(file_path);
  if (!file_bytes.has_value()) {
    DVLOG(1) << "IPTC strip failed; could not read: " << file_path;
    return;
  }

  std::vector<uint8_t> stripped = base::ToVector(
      remove_iptc_metadata(rust::Slice<const uint8_t>(*file_bytes)));
  if (stripped.empty()) {
    // Unrecognized/invalid image bytes — leave the file unchanged.
    DVLOG(1) << "IPTC strip skipped; unrecognized image: " << file_path;
    return;
  }

  if (stripped == *file_bytes) {
    return;
  }

  if (!base::WriteFile(file_path, base::as_byte_span(stripped))) {
    DVLOG(1) << "IPTC strip failed; could not write: " << file_path;
  }
}

}  // namespace image_metadata_stripper
