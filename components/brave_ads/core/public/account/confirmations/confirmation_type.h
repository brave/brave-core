/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ACCOUNT_CONFIRMATIONS_CONFIRMATION_TYPE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ACCOUNT_CONFIRMATIONS_CONFIRMATION_TYPE_H_

#include <ostream>
#include <string_view>

namespace brave_ads {

enum class ConfirmationType {
  kUndefined,

  // Clicked is when the user clicks on an ad.
  kClicked,

  // Dismissed is when the user dismisses an ad.
  kDismissed,

  // Viewed impression is when the ad is shown to the user.
  kViewedImpression,

  // Served impression is when the ad is served.
  kServedImpression,

  // Landed is when the user lands on the ad's landing page.
  kLanded,

  // When the user marks an ad as inappropriate.
  kMarkAdAsInappropriate,

  // When the user saves an ad.
  kSavedAd,

  // When the user likes an ad.
  kLikedAd,

  // When the user dislikes an ad.
  kDislikedAd,

  // When the user converts on an ad.
  kConversion,

  // When a new tab page video ad starts playing.
  kMediaPlay,

  // When played 25% of a new tab page video ad.
  kMedia25,

  // When played 100% of a new tab page video ad.
  kMedia100,

  kMinValue = 0,
  kMaxValue = kMedia100
};

// Returns a `ConfirmationType` value based on the string input.
ConfirmationType ToConfirmationType(std::string_view value);

// Returns a string constant for a given `ConfirmationType` value.
const char* ToString(ConfirmationType type);

std::ostream& operator<<(std::ostream& os, ConfirmationType type);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ACCOUNT_CONFIRMATIONS_CONFIRMATION_TYPE_H_
