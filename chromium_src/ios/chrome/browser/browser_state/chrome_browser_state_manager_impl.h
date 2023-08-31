// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_BROWSER_STATE_CHROME_BROWSER_STATE_MANAGER_IMPL_H_
#define BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_BROWSER_STATE_CHROME_BROWSER_STATE_MANAGER_IMPL_H_

#define DoFinalInitForServices virtual DoFinalInitForServices
#define browser_state_info_cache_ \
  browser_state_info_cache_;      \
  friend class BraveBrowserStateManagerImpl

#include "src/ios/chrome/browser/browser_state/chrome_browser_state_manager_impl.h" // IWYU pragma: export
#undef browser_state_info_cache_
#undef DoFinalInitForServices

#endif  // BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_BROWSER_STATE_CHROME_BROWSER_STATE_MANAGER_IMPL_H_
