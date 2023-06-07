/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_USER_DATA_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_USER_DATA_BUILDER_H_

#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/user_data/build_user_data_callback.h"

namespace brave_ads {

void BuildConfirmationUserData(const TransactionInfo& transaction,
                               BuildUserDataCallback callback);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_USER_DATA_BUILDER_H_
