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

  // TODO(https://github.com/brave/brave-browser/issues/5238): Add the core
  // logic to remove the IPTC metadata here.
  DVLOG(1) << "IPTC strip skipped; not implemented yet: " << file_path;
  return false;
}

}  // namespace image_metadata_stripper
