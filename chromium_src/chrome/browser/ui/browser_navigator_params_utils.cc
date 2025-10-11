/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser_navigator_params.h"

// Needed to prevent overriding url_typed_with_http_scheme
#include "chrome/browser/ui/location_bar/location_bar.h"

// We want URLs that were manually typed with HTTP scheme to be HTTPS
// upgradable, but preserve upstream's behavior regarding captive portals (like
// hotel login pages, which typically aren't cofnigured to work with HTTPS)
#define url_typed_with_http_scheme \
  url_typed_with_http_scheme;      \
  force_no_https_upgrade = false

#include <chrome/browser/ui/browser_navigator_params_utils.cc>

#undef url_typed_with_http_scheme
