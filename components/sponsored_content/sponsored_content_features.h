/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_FEATURES_H_
#define BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_FEATURES_H_

#include "base/feature.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"

namespace sponsored_content::features {

BASE_DECLARE_FEATURE(kSponsoredContentComponent);

// The sponsored content component checks for updates more frequently than
// other components (the default browser-wide interval is every 5 hours).
inline constexpr base::FeatureParam<base::TimeDelta> kComponentUpdateCheckAfter{
    &kSponsoredContentComponent, "component_update_check_after",
    base::Minutes(15)};

}  // namespace sponsored_content::features

#endif  // BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_FEATURES_H_
