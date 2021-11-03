/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "../../../../../content/public/common/content_features.cc"

#include "base/feature_override.h"

namespace features {

DISABLE_FEATURE_BY_DEFAULT(kDirectSockets);
DISABLE_FEATURE_BY_DEFAULT(kIdleDetection);
DISABLE_FEATURE_BY_DEFAULT(kNotificationTriggers);
DISABLE_FEATURE_BY_DEFAULT(kSignedExchangeSubresourcePrefetch);
DISABLE_FEATURE_BY_DEFAULT(kSubresourceWebBundles);
#if defined(OS_ANDROID)
DISABLE_FEATURE_BY_DEFAULT(kWebNfc);
#endif
DISABLE_FEATURE_BY_DEFAULT(kWebOTP);

}  // namespace features
