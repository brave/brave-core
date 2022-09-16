/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/choose/ad_predictor_info.h"

#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_info.h"

namespace ads {

template <typename T>
AdPredictorInfo<T>::AdPredictorInfo() = default;

template <typename T>
AdPredictorInfo<T>::AdPredictorInfo(const AdPredictorInfo<T>& info) = default;

template <typename T>
AdPredictorInfo<T>& AdPredictorInfo<T>::operator=(
    const AdPredictorInfo<T>& info) = default;

template <typename T>
AdPredictorInfo<T>::AdPredictorInfo(AdPredictorInfo<T>&& other) noexcept =
    default;

template <typename T>
AdPredictorInfo<T>& AdPredictorInfo<T>::operator=(
    AdPredictorInfo<T>&& other) noexcept = default;

template <typename T>
AdPredictorInfo<T>::~AdPredictorInfo() = default;

template struct AdPredictorInfo<CreativeInlineContentAdInfo>;
template struct AdPredictorInfo<CreativeNewTabPageAdInfo>;
template struct AdPredictorInfo<CreativeNotificationAdInfo>;

}  // namespace ads
