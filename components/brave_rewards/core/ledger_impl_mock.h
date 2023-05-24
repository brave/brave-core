/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEDGER_IMPL_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEDGER_IMPL_MOCK_H_

#include "brave/components/brave_rewards/core/database/database_mock.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_rewards::internal {

class AddMockLedgerClient {
 protected:
  AddMockLedgerClient();

  ~AddMockLedgerClient();

  MockLedgerClient mock_ledger_client_;
  mojo::AssociatedReceiver<mojom::LedgerClient> mock_ledger_client_receiver_{
      &mock_ledger_client_};
};

class MockLedgerImpl : private AddMockLedgerClient, public LedgerImpl {
 public:
  MockLedgerImpl();

  ~MockLedgerImpl() override;

  MockLedgerClient& mock_client();

  database::MockDatabase& mock_database();

  MOCK_METHOD1(InitializeDatabase, void(LegacyResultCallback));

  MOCK_METHOD0(database, database::Database*());

 private:
  database::MockDatabase mock_database_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEDGER_IMPL_MOCK_H_
