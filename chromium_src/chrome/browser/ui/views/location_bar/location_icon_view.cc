/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Early return from `UpdateBackground` in order to avoid resetting the ink drop
// and clearing the current ink drop state. Since we always use the same
// background color, there is no need to reset the ink drop.
//
// Additionally, when the user toggles the Shields status while the page info
// bubble is open, we intentionally keep the bubble open. Since the Shields
// status update will trigger a call to this method, an early return prevents
// the ink drop from disappearing while the bubble is open.
#define BRAVE_LOCATION_ICON_VIEW_UPDATE_BACKGROUND return;

#include <chrome/browser/ui/views/location_bar/location_icon_view.cc>

#undef BRAVE_LOCATION_ICON_VIEW_UPDATE_BACKGROUND
