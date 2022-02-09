/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/exclusion_rule_util.h"

#include "base/notreached.h"
#include "bat/ads/ad_type.h"

namespace ads {

bool DoesAdTypeSupportFrequencyCapping(const AdType& type) {
  switch (type.value()) {
    case AdType::kAdNotification:
    case AdType::kInlineContentAd: {
      return true;
    }

    case AdType::kNewTabPageAd:
    case AdType::kPromotedContentAd: {
      return false;
    }

    case AdType::kUndefined: {
      NOTREACHED();
      return false;
    }
  }
}

}  // namespace ads
