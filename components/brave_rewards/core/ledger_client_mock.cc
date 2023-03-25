/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/ledger_client_mock.h"

#include "brave/components/brave_rewards/core/test/test_ledger_client.h"

namespace ledger {

MockRewardsService::MockRewardsService() = default;

MockRewardsService::~MockRewardsService() = default;

void MockRewardsService::EncryptString(const std::string& value,
                                       EncryptStringCallback callback) {
  std::move(callback).Run(FakeEncryption::EncryptString(value));
}

void MockRewardsService::DecryptString(const std::string& value,
                                       DecryptStringCallback callback) {
  std::move(callback).Run(FakeEncryption::DecryptString(value));
}

}  // namespace ledger
