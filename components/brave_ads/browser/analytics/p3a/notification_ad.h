/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ANALYTICS_P3A_NOTIFICATION_AD_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ANALYTICS_P3A_NOTIFICATION_AD_H_

class PrefService;

namespace brave_ads {

inline constexpr char kNotificationAdPositionHistogramName[] =
    "Brave.Rewards.CustomNotificationAdPosition";

void RecordNotificationAdPositionMetric(bool should_show_custom_notification,
                                        PrefService* profile_prefs);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ANALYTICS_P3A_NOTIFICATION_AD_H_
