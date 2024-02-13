/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder.h"

#include <utility>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/dynamic/confirmation_dynamic_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/fixed/confirmation_fixed_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/user_data/user_data_info.h"

namespace brave_ads {

namespace {

void BuildFixedUserDataCallback(base::Value::Dict dynamic_user_data,
                                BuildConfirmationUserDataCallback callback,
                                base::Value::Dict fixed_user_data) {
  UserDataInfo user_data;
  user_data.dynamic = std::move(dynamic_user_data);
  user_data.fixed = std::move(fixed_user_data);

  std::move(callback).Run(user_data);
}

void BuildDynamicUserDataCallback(const TransactionInfo& transaction,
                                  BuildConfirmationUserDataCallback callback,
                                  base::Value::Dict dynamic_user_data) {
  BuildFixedUserData(transaction, base::BindOnce(&BuildFixedUserDataCallback,
                                                 std::move(dynamic_user_data),
                                                 std::move(callback)));
}

}  // namespace

void BuildConfirmationUserData(const TransactionInfo& transaction,
                               BuildConfirmationUserDataCallback callback) {
  BuildDynamicUserData(base::BindOnce(&BuildDynamicUserDataCallback,
                                      transaction, std::move(callback)));
}

}  // namespace brave_ads
