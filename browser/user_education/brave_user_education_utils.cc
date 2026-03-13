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

  // Suppress IPH (In Product Help) promos for below features by marking
  // them as dismissed.
  const base::Feature* promos_to_suppress[] = {
      &feature_engagement::kIPHSideBySidePinnableFeature,
      &feature_engagement::kIPHSideBySideTabSwitchFeature,
  };

  for (const auto* feature : promos_to_suppress) {
    user_education::FeaturePromoData data;
    auto existing = storage_service.ReadPromoData(*feature);
    if (existing) {
      data = *existing;
    }
    data.is_dismissed = true;
    data.last_dismissed_by = user_education::FeaturePromoClosedReason::kDismiss;
    if (data.last_show_time.is_null()) {
      data.last_show_time = storage_service.GetCurrentTime();
    }
    storage_service.SavePromoData(*feature, data);
  }
}

}  // namespace brave
