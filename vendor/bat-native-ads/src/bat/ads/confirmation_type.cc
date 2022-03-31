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
constexpr char kUndefinedConfirmationType[] = "";
constexpr char kClickedConfirmationType[] = "click";
constexpr char kDismissedConfirmationType[] = "dismiss";
constexpr char kViewedConfirmationType[] = "view";
constexpr char kServedConfirmationType[] = "served";
constexpr char kTransferredConfirmationType[] = "landed";
constexpr char kSavedConfirmationType[] = "bookmark";
constexpr char kFlaggedConfirmationType[] = "flag";
constexpr char kUpvotedConfirmationType[] = "upvote";
constexpr char kDownvotedConfirmationType[] = "downvote";
constexpr char kConversionConfirmationType[] = "conversion";

}  // namespace

ConfirmationType::ConfirmationType(const std::string& value) {
  if (value == kUndefinedConfirmationType) {
    value_ = kUndefined;
  } else if (value == kClickedConfirmationType) {
    value_ = kClicked;
  } else if (value == kDismissedConfirmationType) {
    value_ = kDismissed;
  } else if (value == kViewedConfirmationType) {
    value_ = kViewed;
  } else if (value == kServedConfirmationType) {
    value_ = kServed;
  } else if (value == kTransferredConfirmationType) {
    value_ = kTransferred;
  } else if (value == kSavedConfirmationType) {
    value_ = kSaved;
  } else if (value == kFlaggedConfirmationType) {
    value_ = kFlagged;
  } else if (value == kUpvotedConfirmationType) {
    value_ = kUpvoted;
  } else if (value == kDownvotedConfirmationType) {
    value_ = kDownvoted;
  } else if (value == kConversionConfirmationType) {
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
      return kUndefinedConfirmationType;
    }

    case kClicked: {
      return kClickedConfirmationType;
    }

    case kDismissed: {
      return kDismissedConfirmationType;
    }

    case kViewed: {
      return kViewedConfirmationType;
    }

    case kServed: {
      return kServedConfirmationType;
    }

    case kTransferred: {
      return kTransferredConfirmationType;
    }

    case kSaved: {
      return kSavedConfirmationType;
    }

    case kFlagged: {
      return kFlaggedConfirmationType;
    }

    case kUpvoted: {
      return kUpvotedConfirmationType;
    }

    case kDownvoted: {
      return kDownvotedConfirmationType;
    }

    case kConversion: {
      return kConversionConfirmationType;
    }
  }
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
