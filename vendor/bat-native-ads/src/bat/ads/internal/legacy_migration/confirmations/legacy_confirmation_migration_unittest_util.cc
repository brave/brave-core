/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/confirmations/legacy_confirmation_migration_unittest_util.h"

#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/legacy_migration/confirmations/legacy_confirmation_migration.h"
#include "bat/ads/pref_names.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ads {
namespace confirmations {

void Migrate(const bool should_migrate) {
  Migrate([=](const bool success) {
    if (should_migrate) {
      ASSERT_TRUE(success);
    } else {
      ASSERT_FALSE(success);
    }
  });
}

uint64_t GetHash() {
  return AdsClientHelper::GetInstance()->GetUint64Pref(
      prefs::kConfirmationsHash);
}

void SetHash(const uint64_t hash) {
  AdsClientHelper::GetInstance()->SetUint64Pref(prefs::kConfirmationsHash,
                                                hash);
}

}  // namespace confirmations
}  // namespace ads
