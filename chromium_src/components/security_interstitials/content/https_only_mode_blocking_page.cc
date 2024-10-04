/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/security_interstitials/content/https_only_mode_blocking_page.h"

// Avoid modifying `OpenUrlInNewForegroundTab` definition.
#include "components/security_interstitials/content/security_interstitial_controller_client.h"

namespace security_interstitials {

namespace {
constexpr char kBraveLearnMoreLink[] =
    "https://support.brave.com/hc/en-us/articles/15513090104717";
}  // namespace

}  // namespace security_interstitials

#define OpenUrlInNewForegroundTab(...) \
  OpenUrlInNewForegroundTab(GURL(kBraveLearnMoreLink))

#include "src/components/security_interstitials/content/https_only_mode_blocking_page.cc"

#undef OpenUrlInNewForegroundTab
