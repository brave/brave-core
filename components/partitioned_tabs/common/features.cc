// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/partitioned_tabs/common/features.h"

namespace partitioned_tabs::features {

// Enable Isolated Tabs. Allows users to open websites in isolated tabs, keeping
// different identities separate within the same browser profile.
BASE_FEATURE(kPartitionedTabs,
             "PartitionedTabs",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace partitioned_tabs::features
