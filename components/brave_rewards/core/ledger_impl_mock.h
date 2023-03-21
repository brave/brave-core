/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEDGER_IMPL_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEDGER_IMPL_MOCK_H_

#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ledger {

class AddMockRewardsService {
 protected:
  AddMockRewardsService();

  ~AddMockRewardsService();

  MockRewardsService mock_rewards_service_;
  mojo::AssociatedReceiver<rewards::mojom::RewardsService>
      mock_rewards_service_receiver_{&mock_rewards_service_};
};

class MockLedgerImpl : private AddMockRewardsService, public LedgerImpl {
 public:
  MockLedgerImpl();

  ~MockLedgerImpl() override;

  MockRewardsService* mock_rewards_service();

  MOCK_CONST_METHOD0(database, database::Database*());

  MOCK_CONST_METHOD0(promotion, promotion::Promotion*());

  MOCK_METHOD2(Initialize, void(bool, LegacyResultCallback));
};

}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEDGER_IMPL_MOCK_H_
