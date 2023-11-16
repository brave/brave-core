/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_TRANSFER_TRANSFER_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_TRANSFER_TRANSFER_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

BASE_DECLARE_FEATURE(kTransferFeature);

inline constexpr base::FeatureParam<base::TimeDelta> kTransferAfter{
    &kTransferFeature, "transfer_after", base::Seconds(10)};

inline constexpr base::FeatureParam<int> kTransferCap{&kTransferFeature,
                                                      "transfer_cap", 1};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_TRANSFER_TRANSFER_FEATURE_H_
