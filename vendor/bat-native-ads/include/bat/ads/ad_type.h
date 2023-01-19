/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_TYPE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_TYPE_H_

#include <ostream>
#include <string>

#include "bat/ads/public/interfaces/ads.mojom-forward.h"

namespace ads {

class AdType final {
 public:
  enum Value {
    kUndefined,
    kNotificationAd,
    kNewTabPageAd,
    kPromotedContentAd,
    kInlineContentAd,
    kSearchResultAd
  };

  AdType();

  // Allow implicit conversion of the enum value to this wrapper
  constexpr AdType(const Value& value)  // NOLINT (runtime/explicit)
      : value_(value) {}

  explicit AdType(const std::string& value);
  explicit AdType(mojom::AdType value);

  Value value() const;
  std::string ToString() const;

  bool operator==(const AdType& other) const;
  bool operator!=(const AdType& other) const;

 private:
  Value value_ = kUndefined;
};

std::ostream& operator<<(std::ostream& os, const AdType& type);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_TYPE_H_
