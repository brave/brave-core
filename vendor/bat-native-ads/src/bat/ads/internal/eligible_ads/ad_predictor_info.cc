/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/ad_predictor_info.h"

#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/bundle/creative_inline_content_ad_info.h"

namespace ads {

template <typename T>
AdPredictorInfo<T>::AdPredictorInfo() = default;

template <typename T>
AdPredictorInfo<T>::AdPredictorInfo(const AdPredictorInfo<T>& info) = default;

template <typename T>
AdPredictorInfo<T>::~AdPredictorInfo() = default;

template struct AdPredictorInfo<CreativeAdNotificationInfo>;
template struct AdPredictorInfo<CreativeInlineContentAdInfo>;

}  // namespace ads
