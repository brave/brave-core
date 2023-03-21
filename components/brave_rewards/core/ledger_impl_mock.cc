/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "brave/components/brave_rewards/common/mojom/bat_ledger.mojom-test-utils.h"

using ::testing::_;
using ::testing::Return;

namespace ledger {

AddMockRewardsService::AddMockRewardsService() = default;

AddMockRewardsService::~AddMockRewardsService() = default;

MockLedgerImpl::MockLedgerImpl() : LedgerImpl({}) {
  ON_CALL(*this, InitializeDatabase(_, _))
      .WillByDefault([](bool, LegacyResultCallback callback) {
        callback(mojom::Result::LEDGER_OK);
      });
  ON_CALL(*this, database()).WillByDefault(Return(&mock_database_));

  rewards::mojom::RewardsUtilityServiceAsyncWaiter sync(this);
  const auto result = sync.InitializeLedger(
      mock_rewards_service_receiver_.BindNewEndpointAndPassDedicatedRemote(),
      false);
  DCHECK(result == mojom::Result::LEDGER_OK);
}

MockLedgerImpl::~MockLedgerImpl() = default;

MockRewardsService* MockLedgerImpl::mock_rewards_service() {
  return &mock_rewards_service_;
}

database::MockDatabase* MockLedgerImpl::mock_database() {
  return &mock_database_;
}

}  // namespace ledger
