/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ACCOUNT_OBSERVER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ACCOUNT_OBSERVER_MOCK_H_

#include <string>

#include "brave/components/brave_ads/core/internal/account/account_observer.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class AccountObserverMock : public AccountObserver {
 public:
  AccountObserverMock();

  AccountObserverMock(const AccountObserverMock&) = delete;
  AccountObserverMock& operator=(const AccountObserverMock&) = delete;

  ~AccountObserverMock() override;

  MOCK_METHOD(void, OnDidInitializeWallet, (const WalletInfo& wallet));

  MOCK_METHOD(void, OnFailedToInitializeWallet, ());

  MOCK_METHOD(void, OnDidProcessDeposit, (const TransactionInfo& transaction));

  MOCK_METHOD(void,
              OnFailedToProcessDeposit,
              (const std::string& creative_instance_id,
               mojom::AdType mojom_ad_type,
               mojom::ConfirmationType mojom_confirmation_type));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ACCOUNT_OBSERVER_MOCK_H_
