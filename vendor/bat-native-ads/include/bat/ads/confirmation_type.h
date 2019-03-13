/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_CONFIRMATION_TYPE_H_
#define BAT_ADS_CONFIRMATION_TYPE_H_

#include <string>

#include "bat/ads/export.h"

namespace ads {

static char kConfirmationTypeClick[] = "click";
static char kConfirmationTypeDismiss[] = "dismiss";
static char kConfirmationTypeView[] = "view";
static char kConfirmationTypeLanded[] = "landed";

enum class ADS_EXPORT ConfirmationType {
  UNKNOWN,
  CLICK,
  DISMISS,
  VIEW,
  LANDED
};

}  // namespace ads

#endif  // BAT_ADS_CONFIRMATION_TYPE_H_
