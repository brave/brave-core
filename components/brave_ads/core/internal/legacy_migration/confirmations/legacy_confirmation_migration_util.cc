/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration_util.h"

#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_helper.h"

namespace brave_ads {

bool HasMigratedConfirmation() {
  return AdsClientHelper::GetInstance()->GetBooleanPref(
      prefs::kHasMigratedConfirmationState);
}

}  // namespace brave_ads
