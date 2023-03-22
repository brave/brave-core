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

  OptedInUserDataInfo(const OptedInUserDataInfo& other);
  OptedInUserDataInfo& operator=(const OptedInUserDataInfo& other);

  OptedInUserDataInfo(OptedInUserDataInfo&& other) noexcept;
  OptedInUserDataInfo& operator=(OptedInUserDataInfo&& other) noexcept;

  ~OptedInUserDataInfo();

  base::Value::Dict dynamic;
  base::Value::Dict fixed;
};

bool operator==(const OptedInUserDataInfo& lhs, const OptedInUserDataInfo& rhs);
bool operator!=(const OptedInUserDataInfo& lhs, const OptedInUserDataInfo& rhs);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_OPTED_IN_USER_DATA_INFO_H_
