// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_BROWSER_STATE_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_BROWSER_STATE_H_

class PrefService;

#define GetRequestContext() \
  GetRequestContext() = 0;  \
  virtual PrefService* GetPrefs()
#include "src/ios/web/public/browser_state.h"  // IWYU pragma: export
#undef GetRequestContext

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_BROWSER_STATE_H_
