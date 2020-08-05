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
const char kConfirmationTypeNone[] = "";
const char kConfirmationTypeClicked[] = "click";
const char kConfirmationTypeDismissed[] = "dismiss";
const char kConfirmationTypeViewed[] = "view";
const char kConfirmationTypeLanded[] = "landed";
const char kConfirmationTypeFlagged[] = "flag";
const char kConfirmationTypeUpvoted[] = "upvote";
const char kConfirmationTypeDownvoted[] = "downvote";
const char kConfirmationTypeConversion[] = "conversion";

}  // namespace

ConfirmationType::ConfirmationType(
    const std::string& value) {
  if (value == kConfirmationTypeNone) {
    value_ = kNone;
  } else if (value == kConfirmationTypeClicked) {
    value_ = kClicked;
  } else if (value == kConfirmationTypeDismissed) {
    value_ = kDismissed;
  } else if (value == kConfirmationTypeViewed) {
    value_ = kViewed;
  } else if (value == kConfirmationTypeLanded) {
    value_ = kLanded;
  } else if (value == kConfirmationTypeFlagged) {
    value_ = kFlagged;
  } else if (value == kConfirmationTypeUpvoted) {
    value_ = kUpvoted;
  } else if (value == kConfirmationTypeDownvoted) {
    value_ = kDownvoted;
  } else if (value == kConfirmationTypeConversion) {
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
    case kNone: {
      return kConfirmationTypeNone;
    }

    case kClicked: {
      return kConfirmationTypeClicked;
    }

    case kDismissed: {
      return kConfirmationTypeDismissed;
    }

    case kViewed: {
      return kConfirmationTypeViewed;
    }

    case kLanded: {
      return kConfirmationTypeLanded;
    }

    case kFlagged: {
      return kConfirmationTypeFlagged;
    }

    case kUpvoted: {
      return kConfirmationTypeUpvoted;
    }

    case kDownvoted: {
      return kConfirmationTypeDownvoted;
    }

    case kConversion: {
      return kConfirmationTypeConversion;
    }
  }
}

bool ConfirmationType::operator==(
    const ConfirmationType& rhs) const {
  return value_ == rhs.value_;
}

bool ConfirmationType::operator!=(
    const ConfirmationType& rhs) const {
  return value_ != rhs.value_;
}

}  // namespace ads
