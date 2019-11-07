/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_STARTUP_DATA_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_STARTUP_DATA_H_

#define BRAVE_STARTUP_DATA_H_    \
 private:                        \
  friend class BraveStartupData; \
 public:
// define BRAVE_STARTUP_DATA_H_

#define PreProfilePrefServiceInit virtual PreProfilePrefServiceInit
#include "../../../../chrome/browser/startup_data.h"
#undef PreProfilePrefServiceInit
#undef BRAVE_STARTUP_DATA_H_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_STARTUP_DATA_H_
