// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TABS_PUBLIC_PARTITIONED_TAB_VISUAL_DATA_H_
#define BRAVE_COMPONENTS_TABS_PUBLIC_PARTITIONED_TAB_VISUAL_DATA_H_

#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/models/image_model.h"

namespace tabs {

// Represents the visual data for a partitioned tab in the tab strip.
// There're two types of partition tabs:
// 1. Container tabs
// 2. Leo tabs
struct PartitionedTabVisualData final {
  SkColor background_color = SK_ColorTRANSPARENT;
  ui::ImageModel icon;
};

}  // namespace tabs

#endif  // BRAVE_COMPONENTS_TABS_PUBLIC_PARTITIONED_TAB_VISUAL_DATA_H_
