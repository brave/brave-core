/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_DATABASE_DATABASE_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_DATABASE_DATABASE_CONSTANTS_H_

namespace brave_ads::database {

inline constexpr int kVersionNumber = 58;
inline constexpr int kCompatibleVersionNumber = 58;

// If the database version number is less than or equal to this value, the
// database will be razed and recreated during migration rather than migrated
// incrementally. Recreation uses `Create`, which builds the current schema
// directly and skips `Migrate` entirely, so `threshold + 1`'s migration is
// unreachable too. Versions at or below this threshold are over a year old,
// so their per-table migration code has been removed; raise this value again
// once the next batch of migration paths turns a year old.
inline constexpr int kRazeDatabaseThresholdVersionNumber = 50;

}  // namespace brave_ads::database

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_DATABASE_DATABASE_CONSTANTS_H_
