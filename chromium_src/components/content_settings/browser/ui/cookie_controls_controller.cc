/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/content_settings/browser/ui/cookie_controls_controller.h"
#include "components/content_settings/browser/ui/cookie_controls_view.h"

#define OnCookieControlsIconStatusChanged(ICON_VISIBLE, PROTECTIONS_ON,      \
                                          BLOCKING_STATUS, SHOULD_HIGHLIGHT) \
  OnCookieControlsIconStatusChanged(ICON_VISIBLE, PROTECTIONS_ON,            \
                                    BLOCKING_STATUS, false)
#include "src/components/content_settings/browser/ui/cookie_controls_controller.cc"
#undef OnCookieControlsIconStatusChanged
