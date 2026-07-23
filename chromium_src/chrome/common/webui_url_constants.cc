/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/constants/webui_url_constants.h"

#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "chrome/common/webui_url_constants.h"
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_WALLET_WEBUI_HOSTS kWalletPageHost,
#else
#define BRAVE_WALLET_WEBUI_HOSTS
#endif

#define kChromeUIAutofillInternalsHost                                \
  kAdblockHost, kAdblockInternalsHost, kRewardsPageHost,              \
      kRewardsInternalsHost, kWelcomeHost,                            \
      BRAVE_WALLET_WEBUI_HOSTS kTorInternalsHost, kSkusInternalsHost, \
      kAdsInternalsHost, kNewTabTakeoverHost, kChromeUIAutofillInternalsHost

#include <chrome/common/webui_url_constants.cc>

#undef kChromeUIAutofillInternalsHost
#undef BRAVE_WALLET_WEBUI_HOSTS
