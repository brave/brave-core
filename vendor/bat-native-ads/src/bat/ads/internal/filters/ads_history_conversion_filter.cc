/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/filters/ads_history_conversion_filter.h"

namespace ads {

AdsHistoryConversionFilter::AdsHistoryConversionFilter() = default;

AdsHistoryConversionFilter::~AdsHistoryConversionFilter() = default;

std::deque<AdHistory> AdsHistoryConversionFilter::Apply(
    const std::deque<AdHistory>& history) const {
  std::deque<AdHistory> ads = history;

  const auto iter = std::remove_if(ads.begin(), ads.end(),
      [this](const AdHistory& ad) {
    return ShouldFilterConfirmationType(ad.ad_content.ad_action);
  });

  ads.erase(iter, ads.end());

  return ads;
}

bool AdsHistoryConversionFilter::ShouldFilterConfirmationType(
    const ConfirmationType& type) const {
  switch (type.value()) {
    case ConfirmationType::kClicked:
    case ConfirmationType::kViewed: {
      return false;
    }

    case ConfirmationType::kNone:
    case ConfirmationType::kDismissed:
    case ConfirmationType::kLanded:
    case ConfirmationType::kFlagged:
    case ConfirmationType::kUpvoted:
    case ConfirmationType::kDownvoted:
    case ConfirmationType::kConversion: {
      return true;
    }
  }
}

}  // namespace ads
