/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "ios/components/security_interstitials/https_only_mode/https_only_mode_blocking_page.h"

// Avoid modifying `OpenUrlInNewForegroundTab` definition.
#import "ios/components/security_interstitials/ios_blocking_page_controller_client.h"

namespace {
constexpr char kBraveLearnMoreLink[] =
    "https://support.brave.app/hc/en-us/articles/15513090104717";
}  // namespace

#define OpenUrlInNewForegroundTab(...) \
  OpenUrlInNewForegroundTab(GURL(kBraveLearnMoreLink))

#include <ios/components/security_interstitials/https_only_mode/https_only_mode_blocking_page.mm>

#undef OpenUrlInNewForegroundTab
