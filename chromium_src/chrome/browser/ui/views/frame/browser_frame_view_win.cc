/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/features.h"
#include "ui/base/ui_base_features.h"

namespace features {

bool BraveHorizontalTabsUpdateEnabled() {
  return tabs::features::HorizontalTabsUpdateEnabled();
}

}  // namespace features

// When updated horizontal tabs are enabled, we want to use the same layout
// logic as upstream's "Refresh2023" for tab strip positioning and for the
// window caption button height. When upstream's feature flag is removed, this
// define can also be removed.
#define IsChromeRefresh2023 BraveHorizontalTabsUpdateEnabled

#include "src/chrome/browser/ui/views/frame/browser_frame_view_win.cc"  // IWYU pragma: export

#undef IsChromeRefresh2023
