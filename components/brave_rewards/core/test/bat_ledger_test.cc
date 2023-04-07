/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/test/bat_ledger_test.h"
#include "brave/components/brave_rewards/common/mojom/bat_ledger.mojom-test-utils.h"

namespace ledger {

BATLedgerTest::BATLedgerTest()
    : ledger_(client_receiver_.BindNewEndpointAndPassDedicatedRemote()) {}

BATLedgerTest::~BATLedgerTest() = default;

void BATLedgerTest::InitializeLedger() {
  const auto result = mojom::LedgerAsyncWaiter(&ledger_).Initialize();
  DCHECK(result == mojom::Result::LEDGER_OK);
}

void BATLedgerTest::AddNetworkResultForTesting(const std::string& url,
                                               mojom::UrlMethod method,
                                               mojom::UrlResponsePtr response) {
  DCHECK(response);
  client_.AddNetworkResultForTesting(url, method, std::move(response));
}

void BATLedgerTest::SetLogCallbackForTesting(
    TestLedgerClient::LogCallback callback) {
  client_.SetLogCallbackForTesting(callback);
}

}  // namespace ledger
