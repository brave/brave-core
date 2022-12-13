/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/wallet_button_notification_source.h"

#include <memory>

#include "base/test/bind.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/browser/brave_wallet/tx_service_factory.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave {

class WalletButtonNotificationSourceTest : public InProcessBrowserTest {
 public:
  WalletButtonNotificationSourceTest() {}

  void SetUpOnMainThread() override {
    keyring_service_ =
        brave_wallet::KeyringServiceFactory::GetServiceForContext(
            browser()->profile());
    tx_service_ = brave_wallet::TxServiceFactory::GetServiceForContext(
        browser()->profile());
  }

  ~WalletButtonNotificationSourceTest() override = default;

  brave_wallet::TxService* tx_service() { return tx_service_; }

  brave_wallet::KeyringService* keyring_service() { return keyring_service_; }

  void RestoreWallet() {
    const char mnemonic[] =
        "drip caution abandon festival order clown oven regular absorb "
        "evidence crew where";
    base::RunLoop run_loop;
    keyring_service_->RestoreWallet(
        mnemonic, "brave123", false,
        base::BindLambdaForTesting([&](bool success) {
          ASSERT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void CreateWallet() {
    base::RunLoop run_loop;
    keyring_service_->CreateWallet(
        "brave123", base::BindLambdaForTesting(
                        [&](const std::string&) { run_loop.Quit(); }));
    run_loop.Run();
  }

 private:
  raw_ptr<brave_wallet::KeyringService> keyring_service_;
  raw_ptr<brave_wallet::TxService> tx_service_;
};

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       ShowBadge_WhenWalletNotCreated) {
  base::RunLoop run_loop;
  absl::optional<bool> show_badge_suggest_result;
  absl::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                [&show_badge_suggest_result, &count_result](
                                    bool show_badge_suggest, size_t count) {
                                  show_badge_suggest_result =
                                      show_badge_suggest;
                                  count_result = count;
                                })));

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_TRUE(show_badge_suggest_result.value());
  EXPECT_EQ(0u, count_result.value());
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       DontShowBadge_WhenWalletCreated) {
  RestoreWallet();

  base::RunLoop run_loop;
  absl::optional<bool> show_badge_suggest_result;
  absl::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                [&show_badge_suggest_result, &count_result](
                                    bool show_badge_suggest, size_t count) {
                                  show_badge_suggest_result =
                                      show_badge_suggest;
                                  count_result = count;
                                })));

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(0u, count_result.value());
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       HideBadge_WhenWalletButtonClicked) {
  base::RunLoop run_loop;
  absl::optional<bool> show_badge_suggest_result;
  absl::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                [&show_badge_suggest_result, &count_result](
                                    bool show_badge_suggest, size_t count) {
                                  show_badge_suggest_result =
                                      show_badge_suggest;
                                  count_result = count;
                                })));

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_TRUE(show_badge_suggest_result.value());
  EXPECT_EQ(0u, count_result.value());

  notification_source->MarkWalletButtonWasClicked();

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(0u, count_result.value());
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       HideBadge_WhenWalletCreated) {
  base::RunLoop run_loop;
  absl::optional<bool> show_badge_suggest_result;
  absl::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                [&show_badge_suggest_result, &count_result](
                                    bool show_badge_suggest, size_t count) {
                                  show_badge_suggest_result =
                                      show_badge_suggest;
                                  count_result = count;
                                })));

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_TRUE(show_badge_suggest_result.value());
  EXPECT_EQ(0u, count_result.value());

  CreateWallet();

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(0u, count_result.value());
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       PendingTransactionsCounter) {
  RestoreWallet();

  // Add initial transaction
  std::string first_tx_meta_id;
  {
    base::RunLoop run_loop;

    const std::string from_account =
        "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q";
    const std::string to_account = "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy";
    auto tx_data = brave_wallet::mojom::TxDataUnion::NewFilTxData(
        brave_wallet::mojom::FilTxData::New(
            "" /* nonce */, "10" /* gas_premium */, "10" /* gas_fee_cap */,
            "100" /* gas_limit */, "" /* max_fee */, to_account, from_account,
            "11"));
    tx_service()->AddUnapprovedTransaction(
        std::move(tx_data), from_account, absl::nullopt, absl::nullopt,
        base::BindLambdaForTesting([&](bool success, const std::string& id,
                                       const std::string& err_message) {
          first_tx_meta_id = id;
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));

    run_loop.Run();
  }

  base::RunLoop run_loop;
  absl::optional<bool> show_badge_suggest_result;
  absl::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                [&show_badge_suggest_result, &count_result](
                                    bool show_badge_suggest, size_t count) {
                                  show_badge_suggest_result =
                                      show_badge_suggest;
                                  count_result = count;
                                })));

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(1u, count_result.value());

  // Add second transaction
  std::string second_tx_meta_id;
  {
    base::RunLoop run_loop;

    const std::string from_account =
        "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q";
    const std::string to_account = "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy";
    auto tx_data = brave_wallet::mojom::TxDataUnion::NewFilTxData(
        brave_wallet::mojom::FilTxData::New(
            "" /* nonce */, "10" /* gas_premium */, "10" /* gas_fee_cap */,
            "100" /* gas_limit */, "" /* max_fee */, to_account, from_account,
            "11"));
    tx_service()->AddUnapprovedTransaction(
        std::move(tx_data), from_account, absl::nullopt, absl::nullopt,
        base::BindLambdaForTesting([&](bool success, const std::string& id,
                                       const std::string& err_message) {
          second_tx_meta_id = id;
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));

    run_loop.Run();
  }

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(2u, count_result.value());

  // Reject first transaction
  {
    base::RunLoop run_loop;
    tx_service()->RejectTransaction(
        brave_wallet::mojom::CoinType::FIL, first_tx_meta_id,
        base::BindLambdaForTesting([&](bool result) {
          EXPECT_TRUE(result);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(1u, count_result.value());

  // Reject second transaction
  {
    base::RunLoop run_loop;
    tx_service()->RejectTransaction(
        brave_wallet::mojom::CoinType::FIL, second_tx_meta_id,
        base::BindLambdaForTesting([&](bool result) {
          EXPECT_TRUE(result);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(0u, count_result.value());
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       CounterReset_WhenResetTxService) {
  RestoreWallet();

  // Add initial transaction
  std::string tx_meta_id;
  {
    base::RunLoop run_loop;

    const std::string from_account =
        "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q";
    const std::string to_account = "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy";
    auto tx_data = brave_wallet::mojom::TxDataUnion::NewFilTxData(
        brave_wallet::mojom::FilTxData::New(
            "" /* nonce */, "10" /* gas_premium */, "10" /* gas_fee_cap */,
            "100" /* gas_limit */, "" /* max_fee */, to_account, from_account,
            "11"));
    tx_service()->AddUnapprovedTransaction(
        std::move(tx_data), from_account, absl::nullopt, absl::nullopt,
        base::BindLambdaForTesting([&](bool success, const std::string& id,
                                       const std::string& err_message) {
          tx_meta_id = id;
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));

    run_loop.Run();
  }

  base::RunLoop run_loop;
  absl::optional<bool> show_badge_suggest_result;
  absl::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                [&show_badge_suggest_result, &count_result](
                                    bool show_badge_suggest, size_t count) {
                                  show_badge_suggest_result =
                                      show_badge_suggest;
                                  count_result = count;
                                })));

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(1u, count_result.value());

  tx_service()->Reset();

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(0u, count_result.value());
}

}  // namespace brave
