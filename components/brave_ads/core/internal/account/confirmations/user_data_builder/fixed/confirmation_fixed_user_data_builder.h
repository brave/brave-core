/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_USER_DATA_BUILDER_FIXED_CONFIRMATION_FIXED_USER_DATA_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_USER_DATA_BUILDER_FIXED_CONFIRMATION_FIXED_USER_DATA_BUILDER_H_

#include "base/values.h"

namespace brave_ads {

struct TransactionInfo;

base::Value::Dict BuildFixedUserData(const TransactionInfo& transaction);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_USER_DATA_BUILDER_FIXED_CONFIRMATION_FIXED_USER_DATA_BUILDER_H_
