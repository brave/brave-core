/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"

#include <cstdint>
#include <optional>
#include <vector>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "brave/components/image_metadata_stripper/internal/jpeg_iptc_metadata_stripper.h"

namespace image_metadata_stripper {

StrippingResultCode RemoveIptcMetadata(const base::FilePath& file_path) {
  if (!base::PathExists(file_path)) {
    DVLOG(1) << "IPTC strip skipped; file missing: " << file_path;
    return StrippingResultCode::kFileNotFound;
  }

  std::optional<std::vector<uint8_t>> file_bytes =
      base::ReadFileToBytes(file_path);
  if (!file_bytes.has_value()) {
    DVLOG(1) << "IPTC strip failed; could not read: " << file_path;
    return StrippingResultCode::kFileReadFailed;
  }

  jpeg::FbmdStripResult result =
      jpeg::RemoveFbmdIptcMetadata(file_bytes.value());
  switch (result) {
    case jpeg::FbmdStripResult::kNotFound:
      return StrippingResultCode::kMetadataNotFound;
    case jpeg::FbmdStripResult::kFailed:
      return StrippingResultCode::kStrippingFailed;
    case jpeg::FbmdStripResult::kRemoved:
      break;
  }

  if (!base::WriteFile(file_path, file_bytes.value())) {
    DVLOG(1) << "IPTC strip failed; could not write: " << file_path;
    return StrippingResultCode::kFileWriteFailed;
  }

  return StrippingResultCode::kStripped;
}

}  // namespace image_metadata_stripper
