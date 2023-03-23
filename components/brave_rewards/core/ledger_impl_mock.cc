/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/ledger_impl_mock.h"

#include <memory>

namespace ledger {

MockLedgerImpl::MockLedgerImpl()
    : LedgerImpl(std::make_unique<MockLedgerClient>()) {}

MockLedgerImpl::~MockLedgerImpl() = default;

MockLedgerClient* MockLedgerImpl::rewards_service() const {
  return static_cast<MockLedgerClient*>(LedgerImpl::rewards_service());
}

}  // namespace ledger
