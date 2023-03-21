/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/test/bat_ledger_test.h"

namespace ledger {

BATLedgerTest::BATLedgerTest() = default;

BATLedgerTest::~BATLedgerTest() = default;

void BATLedgerTest::AddNetworkResultForTesting(const std::string& url,
                                               mojom::UrlMethod method,
                                               mojom::UrlResponsePtr response) {
  DCHECK(response);
  GetTestRewardsService()->AddNetworkResultForTesting(url, method,
                                                      std::move(response));
}

void BATLedgerTest::SetLogCallbackForTesting(
    TestRewardsService::LogCallback callback) {
  GetTestRewardsService()->SetLogCallbackForTesting(callback);
}

}  // namespace ledger
