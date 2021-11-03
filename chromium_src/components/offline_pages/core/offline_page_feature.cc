/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../components/offline_pages/core/offline_page_feature.cc"

#include "base/feature_override.h"

namespace offline_pages {

#if defined(OS_ANDROID)
DISABLE_FEATURE_BY_DEFAULT(kPrefetchingOfflinePagesFeature);
#endif

}  // namespace offline_pages
