/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/confirmation_type.h"

#include "bat/ads/internal/logging.h"

namespace ads {

namespace {

// Do not change the following string values as they are used for persisting and
// restoring state
const char kUndefinedConfirmationType[] = "";
const char kClickedConfirmationType[] = "click";
const char kDismissedConfirmationType[] = "dismiss";
const char kViewedConfirmationType[] = "view";
const char kServedConfirmationType[] = "served";
const char kTransferredConfirmationType[] = "landed";
const char kFlaggedConfirmationType[] = "flag";
const char kUpvotedConfirmationType[] = "upvote";
const char kDownvotedConfirmationType[] = "downvote";
const char kConversionConfirmationType[] = "conversion";

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

ConfirmationType::operator std::string() const {
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

}  // namespace ads
