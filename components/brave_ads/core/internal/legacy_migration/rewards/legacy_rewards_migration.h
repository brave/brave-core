/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_REWARDS_LEGACY_REWARDS_MIGRATION_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_REWARDS_LEGACY_REWARDS_MIGRATION_H_

#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads::rewards {

void Migrate(InitializeCallback callback);

}  // namespace brave_ads::rewards

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_REWARDS_LEGACY_REWARDS_MIGRATION_H_
