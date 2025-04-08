/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/content_settings/browser/ui/cookie_controls_controller.h"
#include "components/content_settings/browser/ui/cookie_controls_view.h"

// Trying a little trick with `should_highlight && !should_highlight` to get to
// refer to `should_highlight` but at the same time pass false into this call,
// and avoiding `error: unused variable 'should_highlight'` failures.
#define OnCookieControlsIconStatusChanged(ICON_VISIBLE, PROTECTIONS_ON,      \
                                          BLOCKING_STATUS, SHOULD_HIGHLIGHT) \
  OnCookieControlsIconStatusChanged(ICON_VISIBLE, PROTECTIONS_ON,            \
                                    BLOCKING_STATUS,                         \
                                    should_highlight && !should_highlight)
#include "src/components/content_settings/browser/ui/cookie_controls_controller.cc"
#undef OnCookieControlsIconStatusChanged
