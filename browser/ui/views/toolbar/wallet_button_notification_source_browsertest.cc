/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/wallet_button_notification_source.h"

#include <memory>
#include <optional>

#include "base/test/bind.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/browser/brave_wallet/tx_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave {

class WalletButtonNotificationSourceTest : public InProcessBrowserTest {
 public:
  WalletButtonNotificationSourceTest() = default;

  ~WalletButtonNotificationSourceTest() override = default;

  brave_wallet::TxService* GetTxService() const {
    return brave_wallet::TxServiceFactory::GetServiceForContext(
        browser()->profile());
  }

  brave_wallet::KeyringService* GetKeyRingService() const {
    return brave_wallet::KeyringServiceFactory::GetServiceForContext(
        browser()->profile());
  }

  brave_wallet::AccountUtils GetAccountUtils() {
    return brave_wallet::AccountUtils(GetKeyRingService());
  }

  void RestoreWallet() {
    ASSERT_TRUE(GetKeyRingService()->RestoreWalletSync(
        brave_wallet::kMnemonicDripCaution, brave_wallet::kTestWalletPassword,
        false));
  }

  void CreateWallet() {
    base::RunLoop run_loop;
    GetKeyRingService()->CreateWallet(
        brave_wallet::kTestWalletPassword,
        base::BindLambdaForTesting(
            [&](const std::string&) { run_loop.Quit(); }));
    run_loop.Run();
  }
};

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       ShowBadge_WhenWalletNotCreated) {
  base::RunLoop run_loop;
  std::optional<bool> show_badge_suggest_result;
  std::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                [&show_badge_suggest_result, &count_result](
                                    bool show_badge_suggest, size_t count) {
                                  show_badge_suggest_result =
                                      show_badge_suggest;
                                  count_result = count;
                                })));
  notification_source->Init();

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_TRUE(show_badge_suggest_result.value());
  EXPECT_EQ(0u, count_result.value());
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       DontShowBadge_WhenWalletCreated) {
  RestoreWallet();

  base::RunLoop run_loop;
  std::optional<bool> show_badge_suggest_result;
  std::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                [&show_badge_suggest_result, &count_result](
                                    bool show_badge_suggest, size_t count) {
                                  show_badge_suggest_result =
                                      show_badge_suggest;
                                  count_result = count;
                                })));
  notification_source->Init();

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(0u, count_result.value());
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       HideBadge_WhenWalletButtonClicked) {
  base::RunLoop run_loop;
  std::optional<bool> show_badge_suggest_result;
  std::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                [&show_badge_suggest_result, &count_result](
                                    bool show_badge_suggest, size_t count) {
                                  show_badge_suggest_result =
                                      show_badge_suggest;
                                  count_result = count;
                                })));
  notification_source->Init();

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
  std::optional<bool> show_badge_suggest_result;
  std::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                [&show_badge_suggest_result, &count_result](
                                    bool show_badge_suggest, size_t count) {
                                  show_badge_suggest_result =
                                      show_badge_suggest;
                                  count_result = count;
                                })));
  notification_source->Init();

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

  // Add initial FIL transaction
  std::string first_tx_meta_id;
  {
    base::RunLoop run_loop;

    const auto from_account = GetAccountUtils().EnsureFilTestAccount(0);
    const std::string to_account = "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy";
    auto tx_data = brave_wallet::mojom::TxDataUnion::NewFilTxData(
        brave_wallet::mojom::FilTxData::New(
            "" /* nonce */, "10" /* gas_premium */, "10" /* gas_fee_cap */,
            "100" /* gas_limit */, "" /* max_fee */, to_account, "11"));
    GetTxService()->AddUnapprovedTransaction(
        std::move(tx_data),
        brave_wallet::GetCurrentChainId(browser()->profile()->GetPrefs(),
                                        brave_wallet::mojom::CoinType::FIL,
                                        std::nullopt),
        from_account->account_id.Clone(),
        base::BindLambdaForTesting([&](bool success, const std::string& id,
                                       const std::string& err_message) {
          first_tx_meta_id = id;
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));

    run_loop.Run();
  }

  base::RunLoop idle_loop;
  std::optional<bool> show_badge_suggest_result;
  std::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                [&show_badge_suggest_result, &count_result](
                                    bool show_badge_suggest, size_t count) {
                                  show_badge_suggest_result =
                                      show_badge_suggest;
                                  count_result = count;
                                })));
  notification_source->Init();

  // Wait until WalletButtonNotificationSource checks are finished
  idle_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(1u, count_result.value());

  // Add second ETH transaction
  std::string second_tx_meta_id;
  {
    base::RunLoop run_loop;

    const auto from_account = GetAccountUtils().EnsureEthAccount(0);
    const std::string to_account = "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c";

    auto tx_data = brave_wallet::mojom::TxData::New(
        "0x06", "0x09184e72a000", "0x0974", to_account, "0x016345785d8a0000",
        std::vector<uint8_t>(), false, std::nullopt);
    GetTxService()->AddUnapprovedTransaction(
        brave_wallet::mojom::TxDataUnion::NewEthTxData(std::move(tx_data)),
        brave_wallet::GetCurrentChainId(browser()->profile()->GetPrefs(),
                                        brave_wallet::mojom::CoinType::ETH,
                                        std::nullopt),
        from_account->account_id.Clone(),
        base::BindLambdaForTesting([&](bool success, const std::string& id,
                                       const std::string& err_message) {
          second_tx_meta_id = id;
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));

    run_loop.Run();
  }

  // Add third SOL transaction
  std::string third_tx_meta_id;
  {
    base::RunLoop run_loop;

    auto from_account = GetAccountUtils().EnsureSolAccount(0);
    std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";

    auto tx_data = brave_wallet::mojom::SolanaTxData::New(
        "" /* recent_blockhash */, 0, from_account->address, to_account,
        "" /* spl_token_mint_address */, 10000000u /* lamport */,
        0 /* amount */,
        brave_wallet::mojom::TransactionType::SolanaSystemTransfer,
        std::vector<brave_wallet::mojom::SolanaInstructionPtr>(),
        brave_wallet::mojom::SolanaMessageVersion::kLegacy,
        brave_wallet::mojom::SolanaMessageHeader::New(0, 0, 0),
        std::vector<std::string>(),
        std::vector<brave_wallet::mojom::SolanaMessageAddressTableLookupPtr>(),
        nullptr, nullptr);

    GetTxService()->AddUnapprovedTransaction(
        brave_wallet::mojom::TxDataUnion::NewSolanaTxData(std::move(tx_data)),
        brave_wallet::GetCurrentChainId(browser()->profile()->GetPrefs(),
                                        brave_wallet::mojom::CoinType::SOL,
                                        std::nullopt),
        from_account->account_id.Clone(),
        base::BindLambdaForTesting([&](bool success, const std::string& id,
                                       const std::string& err_message) {
          third_tx_meta_id = id;
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));

    run_loop.Run();
  }

  // Wait until WalletButtonNotificationSource checks are finished
  idle_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(3u, count_result.value());

  // Reject first transaction
  {
    base::RunLoop run_loop;
    GetTxService()->RejectTransaction(
        brave_wallet::mojom::CoinType::FIL,
        brave_wallet::GetCurrentChainId(browser()->profile()->GetPrefs(),
                                        brave_wallet::mojom::CoinType::FIL,
                                        std::nullopt),
        first_tx_meta_id, base::BindLambdaForTesting([&](bool result) {
          EXPECT_TRUE(result);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  // Wait until WalletButtonNotificationSource checks are finished
  idle_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(2u, count_result.value());

  // Reject second transaction
  {
    base::RunLoop run_loop;
    GetTxService()->RejectTransaction(
        brave_wallet::mojom::CoinType::ETH,
        brave_wallet::GetCurrentChainId(browser()->profile()->GetPrefs(),
                                        brave_wallet::mojom::CoinType::ETH,
                                        std::nullopt),
        second_tx_meta_id, base::BindLambdaForTesting([&](bool result) {
          EXPECT_TRUE(result);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  // Wait until WalletButtonNotificationSource checks are finished
  idle_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(1u, count_result.value());

  // Reject third transaction
  {
    base::RunLoop run_loop;
    GetTxService()->RejectTransaction(
        brave_wallet::mojom::CoinType::SOL,
        brave_wallet::GetCurrentChainId(browser()->profile()->GetPrefs(),
                                        brave_wallet::mojom::CoinType::SOL,
                                        std::nullopt),
        third_tx_meta_id, base::BindLambdaForTesting([&](bool result) {
          EXPECT_TRUE(result);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  // Wait until WalletButtonNotificationSource checks are finished
  idle_loop.RunUntilIdle();

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

    const auto from_account = GetAccountUtils().EnsureFilTestAccount(0);
    const std::string to_account = "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy";
    auto tx_data = brave_wallet::mojom::TxDataUnion::NewFilTxData(
        brave_wallet::mojom::FilTxData::New(
            "" /* nonce */, "10" /* gas_premium */, "10" /* gas_fee_cap */,
            "100" /* gas_limit */, "" /* max_fee */, to_account, "11"));
    GetTxService()->AddUnapprovedTransaction(
        std::move(tx_data),
        brave_wallet::GetCurrentChainId(browser()->profile()->GetPrefs(),
                                        brave_wallet::mojom::CoinType::FIL,
                                        std::nullopt),
        from_account->account_id.Clone(),
        base::BindLambdaForTesting([&](bool success, const std::string& id,
                                       const std::string& err_message) {
          tx_meta_id = id;
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));

    run_loop.Run();
  }

  base::RunLoop run_loop;
  std::optional<bool> show_badge_suggest_result;
  std::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                [&show_badge_suggest_result, &count_result](
                                    bool show_badge_suggest, size_t count) {
                                  show_badge_suggest_result =
                                      show_badge_suggest;
                                  count_result = count;
                                })));
  notification_source->Init();

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(1u, count_result.value());

  GetTxService()->Reset();

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(0u, count_result.value());
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       EmptyBadgeNotShownAfterRestart_IfClicked) {
  {
    base::RunLoop run_loop;
    std::optional<bool> show_badge_suggest_result;
    std::optional<size_t> count_result;
    auto notification_source = std::make_unique<WalletButtonNotificationSource>(
        browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                  [&show_badge_suggest_result, &count_result](
                                      bool show_badge_suggest, size_t count) {
                                    show_badge_suggest_result =
                                        show_badge_suggest;
                                    count_result = count;
                                  })));
    notification_source->Init();
    run_loop.RunUntilIdle();

    EXPECT_TRUE(show_badge_suggest_result.value());
    notification_source->MarkWalletButtonWasClicked();
  }

  {
    base::RunLoop run_loop;
    std::optional<bool> show_badge_suggest_result;
    std::optional<size_t> count_result;
    auto notification_source = std::make_unique<WalletButtonNotificationSource>(
        browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                  [&show_badge_suggest_result, &count_result](
                                      bool show_badge_suggest, size_t count) {
                                    show_badge_suggest_result =
                                        show_badge_suggest;
                                    count_result = count;
                                  })));
    notification_source->Init();
    run_loop.RunUntilIdle();

    EXPECT_FALSE(show_badge_suggest_result.value());
  }
}

}  // namespace brave
