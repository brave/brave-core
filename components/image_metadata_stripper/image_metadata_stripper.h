/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_IMAGE_METADATA_STRIPPER_H_
#define BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_IMAGE_METADATA_STRIPPER_H_

#include "base/files/file_path.h"

namespace image_metadata_stripper {

// Removes the FBMD metadata from the IPTC Instructions field
// (https://www.iptc.org/std/photometadata/documentation/userguide/#_instructions)
// for an image file in |file_path|.
// TODO(https://github.com/brave/brave-browser/issues/5238): Add the core
// logic to remove the IPTC metadata.
bool RemoveIptcMetadata(const base::FilePath& file_path);

}  // namespace image_metadata_stripper

#endif  // BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_IMAGE_METADATA_STRIPPER_H_
