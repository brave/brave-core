/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/client/legacy_client_migration_util.h"

#include <cstdint>

#include "base/hash/hash.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "brave/components/brave_ads/common/pref_names.h"

namespace ads::client {

bool HasMigrated() {
  return AdsClientHelper::GetInstance()->GetBooleanPref(
      prefs::kHasMigratedClientState);
}

void SetHashForJson(const std::string& json) {
  const auto hash = static_cast<uint64_t>(base::PersistentHash(json));

  AdsClientHelper::GetInstance()->SetUint64Pref(prefs::kClientHash, hash);
}

}  // namespace ads::client
