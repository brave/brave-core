/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ad_units/notification_ad/custom_notification_ad_feature.h"

#include "base/feature_list.h"

namespace brave_ads {

BASE_FEATURE(kCustomNotificationAdFeature,
             "CustomNotificationAds",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace brave_ads
