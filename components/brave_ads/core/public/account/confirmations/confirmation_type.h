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
  // When adding new confirmation types they must be added with highest
  // priority at the top so that ads history can be filtered.
  kUndefined,
  kClicked,
  kDismissed,
  kViewed,
  kServed,
  kLanded,
  kMarkAdAsInappropriate,
  kSavedAd,
  kLikedAd,
  kDislikedAd,
  kConversion
};

// Returns a `ConfirmationType` value based on the string input.
ConfirmationType ToConfirmationType(std::string_view value);

// Returns a string constant for a given `ConfirmationType` value.
const char* ToString(ConfirmationType type);

std::ostream& operator<<(std::ostream& os, ConfirmationType type);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ACCOUNT_CONFIRMATIONS_CONFIRMATION_TYPE_H_
