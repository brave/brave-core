// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/tabs/public/tab_interface.h"

namespace tabs {

bool TabInterface::IsPartitionedTab() const {
  return false;
}

std::optional<PartitionedTabVisualData>
TabInterface::GetPartitionedTabVisualData() const {
  return std::nullopt;
}

}  // namespace tabs
