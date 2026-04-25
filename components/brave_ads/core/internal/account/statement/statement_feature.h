/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_STATEMENT_STATEMENT_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_STATEMENT_STATEMENT_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_ads {

BASE_DECLARE_FEATURE(kAccountStatementFeature);

inline constexpr base::FeatureParam<int> kNextPaymentDay{
    &kAccountStatementFeature, "next_payment_day", 7};

inline constexpr base::FeatureParam<double> kMinEstimatedEarningsMultiplier{
    &kAccountStatementFeature, "minimum_estimated_earnings_multiplier", 0.8};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_STATEMENT_STATEMENT_FEATURE_H_
