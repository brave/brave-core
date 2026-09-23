/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/content_settings/browser/ui/cookie_controls_controller.h"
#include "components/content_settings/browser/ui/cookie_controls_view.h"

// Force `icon_visible` to false so the cookie controls page action never shows.
// Brave blocks third-party cookies via Shields, so this upstream page action is
// redundant. Historically we hid it by preventing creation of the legacy
// `CookieControlsIconView` (see the `#define kCookieControls` trick in
// page_action_icon_controller.cc), but once the page action migrated to the new
// framework (`CookieControlsPageActionController`), visibility is driven by the
// `icon_visible` value passed here instead. Neutralizing it at the controller
// keeps the icon hidden on both the legacy and new framework paths.
//
// The `X && !X` trick refers to the local variable while still passing false,
// avoiding `error: unused variable` failures.
#define OnCookieControlsIconStatusChanged(ICON_VISIBLE, CONTROLS_STATE) \
  OnCookieControlsIconStatusChanged(icon_visible && !icon_visible,      \
                                    CONTROLS_STATE)
#include <components/content_settings/browser/ui/cookie_controls_controller.cc>
#undef OnCookieControlsIconStatusChanged
