// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_QUERY_FILTER_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_QUERY_FILTER_COMMON_FEATURES_H_

#include "base/feature.h"
#include "base/feature_list.h"

namespace query_filter {
namespace features {

// Controls whether to rely on a dedicated throttle to intercept non-renderer
// navigations on the browser side.
BASE_DECLARE_FEATURE(kQueryFilterBrowserThrottleEnabled);

}  // namespace features
}  // namespace query_filter

#endif  // BRAVE_COMPONENTS_QUERY_FILTER_COMMON_FEATURES_H_
