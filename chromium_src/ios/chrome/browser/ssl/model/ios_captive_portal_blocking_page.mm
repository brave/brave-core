/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/ssl/model/captive_portal_tab_helper.h"

// Replace the Chrome CaptivePortalTabHelper with the Brave variant that doesn't
// use WebStateList and its insertion agent
#define CaptivePortalTabHelper BraveCaptivePortalTabHelper
#include <ios/chrome/browser/ssl/model/ios_captive_portal_blocking_page.mm>
#undef CaptivePortalTabHelper
