/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet.h"

#include <utility>
#include <vector>

#include "bat/ledger/internal/core/bat_ledger_test.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "net/http/http_status_code.h"

namespace ledger {

class WalletTest : public BATLedgerTest {
 protected:
  mojom::Result CreateWalletIfNecessary() {
    auto* ledger = GetLedgerImpl();

    auto response = mojom::UrlResponse::New();
    response->status_code = net::HTTP_CREATED;
    response->body =
        "{\"paymentId\": \"37742974-3b80-461a-acfb-937e105e5af4\"}";

    AddNetworkResultForTesting(
        endpoint::promotion::GetServerUrl("/v3/wallet/brave"),
        mojom::UrlMethod::POST, std::move(response));

    base::RunLoop run_loop;
    mojom::Result result;
    ledger->wallet()->CreateWalletIfNecessary([&result, &run_loop](auto r) {
      result = r;
      run_loop.Quit();
    });

    run_loop.Run();
    return result;
  }
};

TEST_F(WalletTest, GetWallet) {
  auto* ledger = GetLedgerImpl();
  bool corrupted = false;

  // When there is no current wallet information, `GetWallet` returns empty and
  // sets the corrupted flag to false.
  GetTestLedgerClient()->SetStringState(state::kWalletBrave, "");
  corrupted = true;
  mojom::BraveWalletPtr wallet = ledger->wallet()->GetWallet(&corrupted);
  EXPECT_FALSE(wallet);
  EXPECT_FALSE(corrupted);

  // When there is invalid wallet information, `GetWallet` returns empty, sets
  // the corrupted flag to true, and does not modify prefs.
  GetTestLedgerClient()->SetStringState(state::kWalletBrave, "BAD-DATA");
  wallet = ledger->wallet()->GetWallet(&corrupted);
  EXPECT_FALSE(wallet);
  EXPECT_TRUE(corrupted);
  EXPECT_EQ(GetTestLedgerClient()->GetStringState(state::kWalletBrave),
            "BAD-DATA");
}

TEST_F(WalletTest, CreateWallet) {
  auto* ledger = GetLedgerImpl();

  // Create a wallet when there is no current wallet information.
  GetTestLedgerClient()->SetStringState(state::kWalletBrave, "");
  mojom::Result result = CreateWalletIfNecessary();
  EXPECT_EQ(result, mojom::Result::WALLET_CREATED);
  mojom::BraveWalletPtr wallet = ledger->wallet()->GetWallet();
  ASSERT_TRUE(wallet);
  EXPECT_TRUE(!wallet->payment_id.empty());
  EXPECT_TRUE(!wallet->recovery_seed.empty());

  // Create a wallet when there is corrupted wallet information.
  GetTestLedgerClient()->SetStringState(state::kWalletBrave, "BAD-DATA");
  result = CreateWalletIfNecessary();
  EXPECT_EQ(result, mojom::Result::WALLET_CREATED);
  wallet = ledger->wallet()->GetWallet();
  ASSERT_TRUE(wallet);
  EXPECT_TRUE(!wallet->payment_id.empty());
  EXPECT_TRUE(!wallet->recovery_seed.empty());
}

}  // namespace ledger
