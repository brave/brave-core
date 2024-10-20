// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_WEB_VIEW_BROWSER_STATE_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_WEB_VIEW_BROWSER_STATE_H_

#include "ios/web/public/browser_state.h"

#define GetPrefs()   \
  GetPrefs_Unused(); \
  PrefService* GetPrefs() override
#include "src/ios/web_view/internal/web_view_browser_state.h"  // IWYU pragma: export
#undef GetPrefs

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_WEB_VIEW_BROWSER_STATE_H_
