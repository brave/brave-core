/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/region_code_feature.h"

namespace brave_l10n {

BASE_FEATURE(kFetchResourcesByRegionCodeFeature,
             "FetchResourcesByRegionCode",
             base::FEATURE_ENABLED_BY_DEFAULT);

}  // namespace brave_l10n
