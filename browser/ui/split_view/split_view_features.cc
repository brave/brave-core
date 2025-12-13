/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/split_view/split_view_features.h"

namespace split_view::features {

// Feature flag to control split view link navigation functionality.
// When enabled, allows navigation from the left pane to redirect to the right
// pane when the split view is linked.
BASE_FEATURE(kSplitViewLink, base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace split_view::features
