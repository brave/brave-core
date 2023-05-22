/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "chrome/common/webui_url_constants.h"

#define kChromeUIAttributionInternalsHost                                     \
  kChromeUIAttributionInternalsHost, kAdblockHost, kAdblockInternalsHost,     \
      kRewardsPageHost, kRewardsInternalsHost, kWelcomeHost, kWalletPageHost, \
      kTorInternalsHost, kSkusInternalsHost
#define kChromeUIPerformanceSettingsURL kChromeUIPerformanceSettingsURL_UnUsed
#define kPerformanceSubPage kPerformanceSubPage_UnUsed

#if BUILDFLAG(ENABLE_IPFS_INTERNALS_WEBUI)
#define kChromeUIBlobInternalsHost kChromeUIBlobInternalsHost, kIPFSWebUIHost
#endif

#include "src/chrome/common/webui_url_constants.cc"

#undef kChromeUIBlobInternalsHost
#undef kPerformanceSubPage
#undef kChromeUIPerformanceSettingsURL
#undef kChromeUIAttributionInternalsHost

namespace chrome {

const char kChromeUIPerformanceSettingsURL[] = "chrome://settings/system";
const char kPerformanceSubPage[] = "system";

}  // namespace chrome
