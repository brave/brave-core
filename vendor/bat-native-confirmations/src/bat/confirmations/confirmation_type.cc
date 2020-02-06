/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/confirmation_type.h"

namespace confirmations {

const char kConfirmationTypeClicked[] = "click";
const char kConfirmationTypeDismissed[] = "dismiss";
const char kConfirmationTypeViewed[] = "view";
const char kConfirmationTypeLanded[] = "landed";
const char kConfirmationTypeFlagged[] = "flag";
const char kConfirmationTypeUpvoted[] = "upvote";
const char kConfirmationTypeDownvoted[] = "downvote";
const char kConfirmationTypeConversion[] = "conversion";
const char kConfirmationTypeUnknown[] = "";

ConfirmationType::ConfirmationType(
    const std::string& value) {
  if (value == kConfirmationTypeClicked) {
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
    value_ = kUnknown;
  }
}

bool ConfirmationType::IsSupported() const {
  return value_ != kUnknown;
}

int ConfirmationType::value() const {
  return value_;
}

ConfirmationType::operator std::string() const {
  switch (value_) {
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

    case kUnknown: {
      return kConfirmationTypeUnknown;
    }
  }
}

bool ConfirmationType::operator==(
    const ConfirmationType type) const {
  return value_ == type.value_;
}

bool ConfirmationType::operator!=(
    const ConfirmationType type) const {
  return value_ != type.value_;
}

}  // namespace confirmations
