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

namespace {

// This enum lets the client know the result of their stripping request.
enum class StrippingResultCode {
  // The input file does not exist.
  kFileNotFound,
  // Initial file read failed to check for metadata.
  kFileReadFailed,
  // File rewrite failed with the stripped out metadata.
  kFileWriteFailed,

  // Metadata was not found.
  kMetadataNotFound,
  // Metadata was found but not able to be stripped.
  kStrippingFailed,
  // Metadata was found and stripped.
  kStripped,
};

constexpr std::string_view GetStrippingClient(StrippingClient client) {
  switch (client) {
    case StrippingClient::kDownloadManager:
      return "Download manager";
    case StrippingClient::kFileSelect:
      return "File selector";
  }
  NOTREACHED();
}

void LogStrippingResult(const StrippingClient client,
                        const StrippingResultCode result) {
  const auto tag = GetStrippingClient(client);

  switch (result) {
    case image_metadata_stripper::StrippingResultCode::kFileNotFound: {
      DVLOG(1) << tag << ";Stripping skipped as the file does not exist.";
      return;
    }

    case image_metadata_stripper::StrippingResultCode::kFileReadFailed: {
      DVLOG(1) << tag << ";Failed to read the file to check for metadata.";
      return;
    }

    case image_metadata_stripper::StrippingResultCode::kFileWriteFailed: {
      DVLOG(1) << tag << ";Failed to rewrite the file without the metadata.";
      return;
    }

    case image_metadata_stripper::StrippingResultCode::kMetadataNotFound: {
      DVLOG(1) << tag
               << ";Stripping ignored as FBMD metadata may not be present.";
      return;
    }

    case image_metadata_stripper::StrippingResultCode::kStrippingFailed: {
      DVLOG(1) << tag << ";Failed to strip image metadata from file.";
      return;
    }

    case image_metadata_stripper::StrippingResultCode::kStripped: {
      DVLOG(1) << tag << ";FBMD found and stripped from image metadata.";
      return;
    }
  }
  NOTREACHED();
}

StrippingResultCode RemoveIptcMetadataInternal(
    const base::FilePath& file_path) {
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

}  // namespace

bool RemoveIptcMetadata(const StrippingClient client,
                        const base::FilePath& file_path) {
  const StrippingResultCode result = RemoveIptcMetadataInternal(file_path);

  LogStrippingResult(client, result);

  return result == StrippingResultCode::kStripped;
}

}  // namespace image_metadata_stripper
