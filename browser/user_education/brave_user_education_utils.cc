/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/user_education/brave_user_education_utils.h"

#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/user_education/user_education_service.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/user_education/common/user_education_data.h"
#include "components/user_education/common/user_education_storage_service.h"

namespace brave {

void SuppressUserEducation(UserEducationService* service) {
  if (!service) {
    return;
  }

  auto& storage_service = service->user_education_storage_service();

  // Suppress "New" badges for below features by setting counts to exceed
  // policy limits, ensuring the badge never shows.
  const base::Feature* badges_to_suppress[] = {
      &features::kSideBySide,
      &features::kSideBySideLinkMenuNewBadge,
  };

  constexpr int kNeverShowCount = 999;

  for (const auto* feature : badges_to_suppress) {
    user_education::NewBadgeData data =
        storage_service.ReadNewBadgeData(*feature);

    if (data.feature_enabled_time.is_null()) {
      data.feature_enabled_time = storage_service.GetCurrentTime();
    }

    data.show_count = kNeverShowCount;
    data.used_count = kNeverShowCount;

    storage_service.SaveNewBadgeData(*feature, data);
  }

  // Suppress IPH (In Product Help) promos for below features by marking
  // them as dismissed.
  const base::Feature* promos_to_suppress[] = {
      &feature_engagement::kIPHSideBySidePinnableFeature,
      &feature_engagement::kIPHSideBySideTabSwitchFeature,
  };

  for (const auto* feature : promos_to_suppress) {
    user_education::FeaturePromoData data;
    data.is_dismissed = true;
    storage_service.SavePromoData(*feature, data);
  }
}

}  // namespace brave
