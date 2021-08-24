/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/webui_url_constants.h"
#include "chrome/common/webui_url_constants.h"

#define BRAVE_HOST_URLS                                                  \
  kAdblockHost, kIPFSWebUIHost, kRewardsPageHost, kRewardsInternalsHost, \
      kWelcomeHost, kWalletPageHost, kTorInternalsHost
#define kChromeUIAppCacheInternalsHost \
  kChromeUIAppCacheInternalsHost, BRAVE_HOST_URLS
#include "../../../../chrome/common/webui_url_constants.cc"
#undef kChromeUIAppCacheInternalsHost
#undef BRAVE_HOST_URLS
