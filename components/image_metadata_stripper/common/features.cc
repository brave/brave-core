// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/image_metadata_stripper/common/features.h"

#include "base/feature.h"

namespace image_metadata_stripper {
namespace features {

// TODO(https://github.com/brave/brave-browser/issues/5238): Enable by default
// once RemoveIptcMetadata() strips the FBMD payload.
BASE_FEATURE(kStripDownloadedImageMetadata, base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace features
}  // namespace image_metadata_stripper
