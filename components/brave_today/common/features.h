/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_TODAY_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_TODAY_COMMON_FEATURES_H_

#include "base/feature_list.h"

namespace brave_today {
namespace features {

BASE_DECLARE_FEATURE(kBraveNewsFeature);
BASE_DECLARE_FEATURE(kBraveNewsV2Feature);
BASE_DECLARE_FEATURE(kBraveNewsCardPeekFeature);
BASE_DECLARE_FEATURE(kBraveNewsSubscribeButtonFeature);

}  // namespace features
}  // namespace brave_today

#endif  // BRAVE_COMPONENTS_BRAVE_TODAY_COMMON_FEATURES_H_
