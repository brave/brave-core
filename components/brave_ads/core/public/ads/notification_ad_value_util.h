/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_NOTIFICATION_AD_VALUE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_NOTIFICATION_AD_VALUE_UTIL_H_

#include "base/containers/circular_deque.h"
#include "base/values.h"

namespace brave_ads {

struct NotificationAdInfo;

base::Value::Dict NotificationAdToValue(const NotificationAdInfo& ad);
base::Value::List NotificationAdsToValue(
    const base::circular_deque<NotificationAdInfo>& ads);

NotificationAdInfo NotificationAdFromValue(const base::Value::Dict& dict);
base::circular_deque<NotificationAdInfo> NotificationAdsFromValue(
    const base::Value::List& list);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_NOTIFICATION_AD_VALUE_UTIL_H_
