/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CREATIVE_DEPOSIT_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CREATIVE_DEPOSIT_INFO_H_

#include "base/time/time.h"

namespace brave_ads {

struct CreativeDepositInfo final {
  double value;
  base::Time expire_at;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CREATIVE_DEPOSIT_INFO_H_
