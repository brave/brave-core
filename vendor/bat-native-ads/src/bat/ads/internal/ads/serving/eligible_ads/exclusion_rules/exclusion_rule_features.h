/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULE_FEATURES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULE_FEATURES_H_

#include "base/feature_list.h"  // IWYU pragma: keep

namespace base {
class TimeDelta;
}  // namespace base

namespace ads::exclusion_rules::features {

BASE_DECLARE_FEATURE(kFeature);

bool IsEnabled();

bool ShouldExcludeAdIfConverted();

base::TimeDelta ExcludeAdIfDismissedWithinTimeWindow();

base::TimeDelta ExcludeAdIfTransferredWithinTimeWindow();

}  // namespace ads::exclusion_rules::features

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULE_FEATURES_H_
