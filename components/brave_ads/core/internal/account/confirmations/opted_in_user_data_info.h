/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_OPTED_IN_USER_DATA_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_OPTED_IN_USER_DATA_INFO_H_

#include "base/values.h"

namespace brave_ads {

struct OptedInUserDataInfo final {
  OptedInUserDataInfo();

  OptedInUserDataInfo(const OptedInUserDataInfo&);
  OptedInUserDataInfo& operator=(const OptedInUserDataInfo&);

  OptedInUserDataInfo(OptedInUserDataInfo&&) noexcept;
  OptedInUserDataInfo& operator=(OptedInUserDataInfo&&) noexcept;

  ~OptedInUserDataInfo();

  base::Value::Dict dynamic;
  base::Value::Dict fixed;
};

bool operator==(const OptedInUserDataInfo&, const OptedInUserDataInfo&);
bool operator!=(const OptedInUserDataInfo&, const OptedInUserDataInfo&);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_OPTED_IN_USER_DATA_INFO_H_
