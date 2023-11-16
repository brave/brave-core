/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_ads {

BASE_DECLARE_FEATURE(kIssuersFeature);

inline constexpr base::FeatureParam<int> kMaximumIssuerPublicKeys{
    &kIssuersFeature, "maximum_public_keys", 6};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_FEATURE_H_
