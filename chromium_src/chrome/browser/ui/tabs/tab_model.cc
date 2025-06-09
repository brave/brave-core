/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/tab_model.h"

#include "src/chrome/browser/ui/tabs/tab_model.cc"

namespace tabs {

bool TabModel::IsPartitionedTab() const {
  return partitioned_tab_visual_data_.has_value();
}

void TabModel::SetPartitionedTabVisualData(
    const std::optional<PartitionedTabVisualData>& data) {
  partitioned_tab_visual_data_ = data;
}

std::optional<PartitionedTabVisualData> TabModel::GetPartitionedTabVisualData()
    const {
  return partitioned_tab_visual_data_;
}

}  // namespace tabs
