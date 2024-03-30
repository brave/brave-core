/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_FEATURES_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_FEATURES_H_

#include "base/feature_list.h"

namespace features {

BASE_DECLARE_FEATURE(kBraveCleanupSessionCookiesOnSessionRestore);
BASE_DECLARE_FEATURE(kBraveCopyCleanLinkByDefault);
BASE_DECLARE_FEATURE(kBraveOverrideDownloadDangerLevel);
BASE_DECLARE_FEATURE(kBraveWebViewRoundedCorners);
BASE_DECLARE_FEATURE(kBraveDayZeroExperiment);

}  // namespace features

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_FEATURES_H_
