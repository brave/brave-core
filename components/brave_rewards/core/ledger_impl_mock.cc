/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "brave/components/brave_rewards/common/mojom/bat_ledger.mojom-test-utils.h"

using ::testing::_;
using ::testing::Return;

namespace brave_rewards::internal {

AddMockLedgerClient::AddMockLedgerClient() = default;

AddMockLedgerClient::~AddMockLedgerClient() = default;

MockLedgerImpl::MockLedgerImpl()
    : LedgerImpl(mock_ledger_client_receiver_
                     .BindNewEndpointAndPassDedicatedRemote()) {
  ON_CALL(*this, InitializeDatabase(_))
      .WillByDefault([](LegacyResultCallback callback) {
        callback(mojom::Result::LEDGER_OK);
      });
  ON_CALL(*this, database()).WillByDefault(Return(&mock_database_));

  const auto result = mojom::LedgerAsyncWaiter(this).Initialize();
  DCHECK(result == mojom::Result::LEDGER_OK);
}

MockLedgerImpl::~MockLedgerImpl() = default;

MockLedgerClient& MockLedgerImpl::mock_client() {
  return mock_ledger_client_;
}

database::MockDatabase& MockLedgerImpl::mock_database() {
  return mock_database_;
}

}  // namespace brave_rewards::internal
