// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_METRICS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_METRICS_H_

#define LogProfileAvatarSelection \
    LogProfileAvatarSelection_ChromiumImpl(size_t index); \
    static void LogProfileAvatarSelection
#include "src/chrome/browser/profiles/profile_metrics.h"
#undef LogProfileAvatarSelection

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_METRICS_H_
