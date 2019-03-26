/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/confirmation_type.h"

namespace ads {

static const char kConfirmationTypeClick[] = "click";
static const char kConfirmationTypeDismiss[] = "dismiss";
static const char kConfirmationTypeView[] = "view";
static const char kConfirmationTypeLanded[] = "landed";

ConfirmationType::ConfirmationType(const std::string& value) {
  if (value == kConfirmationTypeClick) {
    value_ = CLICK;
  } else if (value == kConfirmationTypeDismiss) {
    value_ = DISMISS;
  } else if (value == kConfirmationTypeView) {
    value_ = VIEW;
  } else if (value == kConfirmationTypeLanded) {
    value_ = LANDED;
  } else {
    value_ = UNKNOWN;
  }
}

bool ConfirmationType::IsSupported() const {
  return value_ != UNKNOWN;
}

int ConfirmationType::value() const {
  return value_;
}

ConfirmationType::operator std::string() const {
  switch (value_) {
    case UNKNOWN: {
      return "";
    }

    case CLICK: {
      return kConfirmationTypeClick;
    }

    case DISMISS: {
      return kConfirmationTypeDismiss;
    }

    case VIEW: {
      return kConfirmationTypeView;
    }

    case LANDED: {
      return kConfirmationTypeLanded;
    }
  }
}

bool ConfirmationType::operator==(ConfirmationType type) const {
  return value_ == type.value_;
}

bool ConfirmationType::operator!=(ConfirmationType type) const {
  return value_ != type.value_;
}

}  // namespace ads
