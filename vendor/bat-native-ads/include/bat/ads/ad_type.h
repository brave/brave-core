/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_TYPE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_TYPE_H_

#include <string>

namespace ads {

class AdType {
 public:
  enum Value {
    kUndefined,
    kAdNotification,
    kNewTabPageAd,
    kPromotedContentAd,
    kInlineContentAd
  };

  AdType() = default;

  // Allow implicit conversion of the enum value to this wrapper
  constexpr AdType(const Value& value)  // NOLINT(runtime/explicit)
      : value_(value) {}

  explicit AdType(const std::string& value);

  Value value() const;
  operator std::string() const;

  bool operator==(const AdType& rhs) const;
  bool operator!=(const AdType& rhs) const;

 private:
  Value value_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_TYPE_H_
