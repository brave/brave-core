// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_COMMON_STRIPPING_CLIENT_H_
#define BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_COMMON_STRIPPING_CLIENT_H_

namespace image_metadata_stripper {

// The client requesting to strip the image metadata.
enum class StrippingClient {
  // Download flow.
  kDownloadManager,
  // Upload flow.
  kFileSelect,
  // Files dropped onto a page.
  kDragDrop,
};

}  // namespace image_metadata_stripper

#endif  // BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_COMMON_STRIPPING_CLIENT_H_
