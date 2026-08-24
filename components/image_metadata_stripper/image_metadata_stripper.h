/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_IMAGE_METADATA_STRIPPER_H_
#define BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_IMAGE_METADATA_STRIPPER_H_

#include "base/files/file_path.h"

namespace image_metadata_stripper {

// This class lets the client know on a very high level what happened to their
// stripping request.
enum class StrippingResultCode { kStripped, kStrippingFailed, kIgnored };

// Removes the FBMD metadata from the IPTC Instructions field
// (https://www.iptc.org/std/photometadata/documentation/userguide/#_instructions)
// for an image file in |file_path|.
StrippingResultCode RemoveIptcMetadata(const base::FilePath& file_path);

}  // namespace image_metadata_stripper

#endif  // BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_IMAGE_METADATA_STRIPPER_H_
