/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_CONFIRMATION_TYPE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_CONFIRMATION_TYPE_H_

#include <ostream>
#include <string>

namespace brave_ads {

class ConfirmationType final {
 public:
  enum Value {
    // When adding new confirmation types they must be added with highest
    // priority at the top so that ads history can be filtered
    kUndefined,
    kClicked,
    kDismissed,
    kViewed,
    kServed,
    kTransferred,
    kFlagged,
    kSaved,
    kUpvoted,
    kDownvoted,
    kConversion
  };

  ConfirmationType();

  // Allow implicit conversion of the enum value to this wrapper
  constexpr ConfirmationType(const Value& value)  // NOLINT (runtime/explicit)
      : value_(value) {}

  explicit ConfirmationType(const std::string& value);

  Value value() const;
  std::string ToString() const;

 private:
  Value value_ = kUndefined;
};

bool operator==(const ConfirmationType&, const ConfirmationType&);
bool operator!=(const ConfirmationType&, const ConfirmationType&);

std::ostream& operator<<(std::ostream& os, const ConfirmationType& type);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_CONFIRMATION_TYPE_H_
