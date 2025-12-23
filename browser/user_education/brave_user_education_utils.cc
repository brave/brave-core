/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/user_education/brave_user_education_utils.h"

#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/user_education/user_education_service.h"
#include "components/user_education/common/user_education_data.h"
#include "components/user_education/common/user_education_storage_service.h"

namespace brave {

void SuppressNewBadgesForFeatures(UserEducationService* service) {
  if (!service) {
    return;
  }

  // Get the storage service to manipulate badge data
  auto& storage_service = service->user_education_storage_service();

  // List of features we want to suppress badges for
  const base::Feature* features_to_suppress[] = {
      &features::kSideBySide, &features::kSideBySideLinkMenuNewBadge,
      // Add more features here as needed
  };

  // Set counts to exceed the policy limits for each feature, ensuring
  // the badge never shows:
  // - show_count: Set to 999 (far exceeds default limit)
  // - used_count: Set to 999 (far exceeds default limit)
  // This makes NewBadgePolicy::ShouldShowNewBadge() return false
  constexpr int kNeverShowCount = 999;

  for (const auto* feature : features_to_suppress) {
    // Read current badge data
    user_education::NewBadgeData data =
        storage_service.ReadNewBadgeData(*feature);

    // If this is the first time we're seeing this feature (null enabled time),
    // initialize it properly
    if (data.feature_enabled_time.is_null()) {
      data.feature_enabled_time = storage_service.GetCurrentTime();
    }

    // Set counts to maximum to suppress the badge
    data.show_count = kNeverShowCount;
    data.used_count = kNeverShowCount;

    // Save the modified data back to storage
    storage_service.SaveNewBadgeData(*feature, data);
  }
}

}  // namespace brave
