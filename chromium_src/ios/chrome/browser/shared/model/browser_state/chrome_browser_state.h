// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_SHARED_MODEL_BROWSER_STATE_CHROME_BROWSER_STATE_H_
#define BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_SHARED_MODEL_BROWSER_STATE_CHROME_BROWSER_STATE_H_

#include "ios/web/public/browser_state.h"

#define GetPrefs()   \
  GetPrefs_Unused(); \
  PrefService* GetPrefs() override
#include "src/ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#undef GetPrefs

#endif  // BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_SHARED_MODEL_BROWSER_STATE_CHROME_BROWSER_STATE_H_
