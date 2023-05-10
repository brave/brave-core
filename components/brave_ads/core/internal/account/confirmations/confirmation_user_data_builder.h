/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_USER_DATA_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_USER_DATA_BUILDER_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/user_data/user_data_builder_interface.h"

namespace brave_ads {

class ConfirmationUserDataBuilder final : public UserDataBuilderInterface {
 public:
  explicit ConfirmationUserDataBuilder(TransactionInfo transaction);
  ~ConfirmationUserDataBuilder() override;

  void Build(UserDataBuilderCallback callback) const override;

 private:
  void GetConversionCallback(UserDataBuilderCallback callback,
                             base::Value::Dict user_data) const;

  TransactionInfo transaction_;

  base::WeakPtrFactory<ConfirmationUserDataBuilder> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_USER_DATA_BUILDER_H_
