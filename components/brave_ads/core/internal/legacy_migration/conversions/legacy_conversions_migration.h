/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_CONVERSIONS_LEGACY_CONVERSIONS_MIGRATION_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_CONVERSIONS_LEGACY_CONVERSIONS_MIGRATION_H_

#include "brave/components/brave_ads/core/ads_callback.h"

namespace brave_ads::conversions {

void Migrate(InitializeCallback callback);

}  // namespace brave_ads::conversions

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_CONVERSIONS_LEGACY_CONVERSIONS_MIGRATION_H_
