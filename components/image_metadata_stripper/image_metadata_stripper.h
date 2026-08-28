/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_IMAGE_METADATA_STRIPPER_H_
#define BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_IMAGE_METADATA_STRIPPER_H_

#include "base/files/file_path.h"

namespace image_metadata_stripper {

// The client requesting to strip the image metadata.
enum class StrippingClient {
  // Download flow.
  kDownloadManager,
  // Upload flow.
  kFileSelect,
};

// Removes the FBMD metadata from the IPTC Instructions field for an image file
// in |file_path|. The |client| is needed to log the stripping result code
// tagged on client.
// Returns true only when the fbmd iptc metadata was actually removed and false
// otherwise.
bool RemoveIptcMetadata(const StrippingClient client,
                        const base::FilePath& file_path);

}  // namespace image_metadata_stripper

#endif  // BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_IMAGE_METADATA_STRIPPER_H_
