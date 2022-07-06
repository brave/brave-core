/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/confirmation_type.h"

#include "base/notreached.h"

namespace ads {

namespace {

// Do not change the following string values as they are used for persisting and
// restoring state
constexpr char kUndefinedType[] = "";
constexpr char kClickedType[] = "click";
constexpr char kDismissedType[] = "dismiss";
constexpr char kViewedType[] = "view";
constexpr char kServedType[] = "served";
constexpr char kTransferredType[] = "landed";
constexpr char kSavedType[] = "bookmark";
constexpr char kFlaggedType[] = "flag";
constexpr char kUpvotedType[] = "upvote";
constexpr char kDownvotedType[] = "downvote";
constexpr char kConversionType[] = "conversion";

}  // namespace

ConfirmationType::ConfirmationType() = default;

ConfirmationType::ConfirmationType(const ConfirmationType&) = default;

ConfirmationType& ConfirmationType::operator=(const ConfirmationType&) =
    default;

ConfirmationType::ConfirmationType(const std::string& value) {
  if (value == kUndefinedType) {
    value_ = kUndefined;
  } else if (value == kClickedType) {
    value_ = kClicked;
  } else if (value == kDismissedType) {
    value_ = kDismissed;
  } else if (value == kViewedType) {
    value_ = kViewed;
  } else if (value == kServedType) {
    value_ = kServed;
  } else if (value == kTransferredType) {
    value_ = kTransferred;
  } else if (value == kSavedType) {
    value_ = kSaved;
  } else if (value == kFlaggedType) {
    value_ = kFlagged;
  } else if (value == kUpvotedType) {
    value_ = kUpvoted;
  } else if (value == kDownvotedType) {
    value_ = kDownvoted;
  } else if (value == kConversionType) {
    value_ = kConversion;
  } else {
    NOTREACHED();
  }
}

ConfirmationType::Value ConfirmationType::value() const {
  return value_;
}

std::string ConfirmationType::ToString() const {
  switch (value_) {
    case kUndefined: {
      return kUndefinedType;
    }

    case kClicked: {
      return kClickedType;
    }

    case kDismissed: {
      return kDismissedType;
    }

    case kViewed: {
      return kViewedType;
    }

    case kServed: {
      return kServedType;
    }

    case kTransferred: {
      return kTransferredType;
    }

    case kSaved: {
      return kSavedType;
    }

    case kFlagged: {
      return kFlaggedType;
    }

    case kUpvoted: {
      return kUpvotedType;
    }

    case kDownvoted: {
      return kDownvotedType;
    }

    case kConversion: {
      return kConversionType;
    }
  }

  NOTREACHED() << "Unexpected value for Value: " << value_;
  return kUndefinedType;
}

bool ConfirmationType::operator==(const ConfirmationType& rhs) const {
  return value_ == rhs.value_;
}

bool ConfirmationType::operator!=(const ConfirmationType& rhs) const {
  return value_ != rhs.value_;
}

std::ostream& operator<<(std::ostream& os, const ConfirmationType& type) {
  os << type.ToString();
  return os;
}

}  // namespace ads
