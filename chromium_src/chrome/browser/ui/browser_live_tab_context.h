// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_LIVE_TAB_CONTEXT_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_LIVE_TAB_CONTEXT_H_

#include "components/sessions/core/live_tab_context.h"

// Add decorator method to add custom tab title data to extra data for tabs.
// BrowserLiveTabContext is used to tracking tab's state so that it can be
// used tab restoration, and we need keep custom title for restored tabs.
#define GetExtraDataForTab(...)                       \
  GetExtraDataForTab_ChromiumImpl(__VA_ARGS__) const; \
  std::map<std::string, std::string> GetExtraDataForTab(__VA_ARGS__)

#include <chrome/browser/ui/browser_live_tab_context.h>  // IWYU pragma: export

#undef GetExtraDataForTab

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_LIVE_TAB_CONTEXT_H_
