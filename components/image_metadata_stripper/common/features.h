// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_COMMON_FEATURES_H_

#include "base/feature_list.h"

namespace image_metadata_stripper {
namespace features {

// Controls whether tracking metadata is stripped from images on download /
// upload flow.
BASE_DECLARE_FEATURE(kStripImageMetadataV1);

}  // namespace features
}  // namespace image_metadata_stripper

#endif  // BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_COMMON_FEATURES_H_
