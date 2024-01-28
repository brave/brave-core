/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sidebar/features.h"

namespace sidebar::features {

// Load sidebar item's url in the panel instead of loading it in the tab.
BASE_FEATURE(kSidebarMobileView,
             "SidebarMobileView",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace sidebar::features
