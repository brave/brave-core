/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/test/bat_ledger_test.h"

#include <string>

namespace ledger {

BATLedgerTest::BATLedgerTest()
    : ledger_(std::make_unique<TestLedgerClient>()) {}

BATLedgerTest::~BATLedgerTest() = default;

void BATLedgerTest::AddNetworkResultForTesting(const std::string& url,
                                               mojom::UrlMethod method,
                                               mojom::UrlResponsePtr response) {
  DCHECK(response);
  GetTestLedgerClient()->AddNetworkResultForTesting(url, method,
                                                    std::move(response));
}

void BATLedgerTest::SetLogCallbackForTesting(
    TestLedgerClient::LogCallback callback) {
  GetTestLedgerClient()->SetLogCallbackForTesting(callback);
}

}  // namespace ledger
