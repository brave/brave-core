/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/peek/peek_features.h"

namespace peek::features {

// Opens links in a floating "Peek" card over the current tab. Triggered by
// Shift+click and a context menu item.
BASE_FEATURE(kBravePeek, base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace peek::features
