/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_DATABASE_DATABASE_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_DATABASE_DATABASE_CONSTANTS_H_

namespace brave_ads::database {

inline constexpr int kVersionNumber = 44;
inline constexpr int kCompatibleVersionNumber = 44;

// If the database version number is less than or equal to this value, the
// database will be razed and recreated during migration. This should be updated
// to match CR versions of the browser that no longer refill confirmation
// tokens.
inline constexpr int kRazeDatabaseThresholdVersionNumber = 32;

}  // namespace brave_ads::database

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_DATABASE_DATABASE_CONSTANTS_H_
