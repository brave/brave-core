/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/dynamic/confirmation_dynamic_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/fixed/confirmation_fixed_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/user_data/user_data_info.h"

namespace brave_ads {

UserDataInfo BuildConfirmationUserData(const TransactionInfo& transaction,
                                       base::Value::Dict user_data) {
  UserDataInfo confirmation_user_data;

  confirmation_user_data.dynamic = BuildDynamicUserData();

  confirmation_user_data.fixed = BuildFixedUserData(transaction);
  confirmation_user_data.fixed.Merge(std::move(user_data));

  return confirmation_user_data;
}

}  // namespace brave_ads
