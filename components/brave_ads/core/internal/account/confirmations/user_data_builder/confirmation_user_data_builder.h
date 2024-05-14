/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_USER_DATA_BUILDER_CONFIRMATION_USER_DATA_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_USER_DATA_BUILDER_CONFIRMATION_USER_DATA_BUILDER_H_

#include "base/values.h"

namespace brave_ads {

struct TransactionInfo;
struct UserDataInfo;

UserDataInfo BuildConfirmationUserData(const TransactionInfo& transaction,
                                       base::Value::Dict user_data);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_USER_DATA_BUILDER_CONFIRMATION_USER_DATA_BUILDER_H_
