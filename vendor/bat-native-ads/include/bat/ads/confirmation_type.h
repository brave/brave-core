/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_CONFIRMATION_TYPE_H_
#define BAT_ADS_CONFIRMATION_TYPE_H_

#include <string>

namespace ads {

class ConfirmationType {
 public:
  enum Value {
    // When adding new confirmation types they must be added with highest
    // priority at the top so that ads history can be filtered
    kUndefined,
    kClicked,
    kDismissed,
    kViewed,
    kTransferred,
    kFlagged,
    kUpvoted,
    kDownvoted,
    kConversion
  };

  ConfirmationType() = default;

  // Allow implicit conversion of the enum value to this wrapper
  constexpr ConfirmationType(
      const Value& value)
      : value_(value) {}

  explicit ConfirmationType(
      const std::string& value);

  Value value() const;
  operator std::string() const;

  bool operator==(
      const ConfirmationType& rhs) const;
  bool operator!=(
      const ConfirmationType& rhs) const;

 private:
  Value value_;
};

}  // namespace ads

#endif  // BAT_ADS_CONFIRMATION_TYPE_H_
