/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/ledger_impl_mock.h"

namespace ledger {

AddMockRewardsService::AddMockRewardsService() = default;

AddMockRewardsService::~AddMockRewardsService() = default;

MockLedgerImpl::MockLedgerImpl()
    : LedgerImpl(mock_rewards_service_receiver_
                     .BindNewEndpointAndPassDedicatedRemote()) {}

MockLedgerImpl::~MockLedgerImpl() {
  mock_rewards_service_receiver_.FlushForTesting();
}

MockRewardsService* MockLedgerImpl::mock_rewards_service() {
  return &mock_rewards_service_;
}

}  // namespace ledger
