/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_CONFIRMATION_TYPE_H_
#define BAT_CONFIRMATIONS_CONFIRMATION_TYPE_H_

#include <string>

namespace confirmations {

class ConfirmationType {
 public:
  enum Value : int {
    UNKNOWN,
    CLICK,
    DISMISS,
    VIEW,
    LANDED,
    FLAG,
    UPVOTE,
    DOWNVOTE
  };

  ConfirmationType() = default;

  // Allow implicit conversion of the enum value to this wrapper
  constexpr ConfirmationType(const Value& value) : value_(value) {}  // NOLINT

  explicit ConfirmationType(const std::string& value);

  bool IsSupported() const;

  int value() const;
  operator std::string() const;

  bool operator==(ConfirmationType type) const;
  bool operator!=(ConfirmationType type) const;

 private:
  Value value_;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_CONFIRMATION_TYPE_H_
