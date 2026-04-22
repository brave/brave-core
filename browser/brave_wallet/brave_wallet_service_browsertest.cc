/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"

#include <optional>

#include "base/command_line.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/run_until.h"
#include "base/test/test_future.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_observer_base.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/notifications/notification_display_service_tester.h"
#include "chrome/browser/notifications/notification_handler.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/message_center/public/cpp/notification.h"
#include "url/origin.h"

using base::test::TestFuture;

namespace brave_wallet {

namespace {

void OnGetActiveOrigin(bool* callback_called,
                       mojom::OriginInfoPtr expected_active_origin,
                       mojom::OriginInfoPtr active_origin) {
  EXPECT_EQ(expected_active_origin, active_origin);
  *callback_called = true;
}

}  // namespace

class TestBraveWalletServiceObserver
    : public brave_wallet::BraveWalletServiceObserverBase {
 public:
  TestBraveWalletServiceObserver() = default;

  void OnActiveOriginChanged(mojom::OriginInfoPtr origin_info) override {
    active_origin_info_ = origin_info->Clone();
  }

  const mojom::OriginInfoPtr& active_origin_info() const {
    return active_origin_info_;
  }

  mojo::PendingRemote<brave_wallet::mojom::BraveWalletServiceObserver>
  GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }

  void Reset() { active_origin_info_ = {}; }

 private:
  mojom::OriginInfoPtr active_origin_info_;
  mojo::Receiver<brave_wallet::mojom::BraveWalletServiceObserver>
      observer_receiver_{this};
};

class BraveWalletServiceTest : public InProcessBrowserTest {
 public:
  BraveWalletServiceTest()
      : https_server_for_rpc_(net::EmbeddedTestServer::TYPE_HTTPS),
        https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    notification_tester_ = std::make_unique<NotificationDisplayServiceTester>(
        browser()->profile());
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    EXPECT_TRUE(https_server_.Start());
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  void TearDownOnMainThread() override {
    InProcessBrowserTest::TearDownOnMainThread();
    incognito_browser_ = nullptr;
  }

  void TestIsPrivateWindow(BraveWalletService* wallet_service,
                           bool expected_result) {
    base::MockCallback<base::OnceCallback<void(bool)>> callback;
    EXPECT_CALL(callback, Run(expected_result)).Times(1);

    wallet_service->IsPrivateWindow(callback.Get());
  }

  BraveWalletService* wallet_service() {
    return BraveWalletServiceFactory::GetServiceForContext(
        browser()->profile());
  }

  TxService* tx_service() { return wallet_service()->tx_service(); }

  BraveWalletService* incognito_wallet_service() {
    if (!incognito_browser_) {
      incognito_browser_ = CreateIncognitoBrowser(browser()->profile());
    }
    return brave_wallet::BraveWalletServiceFactory::GetInstance()
        ->GetServiceForContext(incognito_browser_->profile());
  }

  void CloseIncognitoBrowser() {
    CloseBrowserSynchronously(incognito_browser_.ExtractAsDangling());
  }

  const net::EmbeddedTestServer* https_server() const { return &https_server_; }

  std::vector<message_center::Notification> GetWalletNotifications() {
    return notification_tester_->GetDisplayedNotificationsForType(
        NotificationHandler::Type::BRAVE_WALLET);
  }

  void SimulateWalletNotificationClick(const std::string& notification_id) {
    notification_tester_->SimulateClick(NotificationHandler::Type::BRAVE_WALLET,
                                        notification_id, std::nullopt,
                                        std::nullopt);
  }

 protected:
  net::EmbeddedTestServer https_server_for_rpc_;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<NotificationDisplayServiceTester> notification_tester_;
  raw_ptr<Browser> incognito_browser_ = nullptr;
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(BraveWalletServiceTest, ActiveOrigin) {
  GURL url = https_server()->GetURL("a.test", "/simple.html");
  auto expected_origin_info = MakeOriginInfo(url::Origin::Create(url));
  TestBraveWalletServiceObserver observer;
  wallet_service()->AddObserver(observer.GetReceiver());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  bool callback_called = false;
  wallet_service()->GetActiveOrigin(base::BindOnce(
      &OnGetActiveOrigin, &callback_called, expected_origin_info->Clone()));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(observer.active_origin_info(), expected_origin_info);

  url = https_server()->GetURL("b.test", "/simple.html");
  expected_origin_info = MakeOriginInfo(url::Origin::Create(url));
  callback_called = false;
  observer.Reset();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  wallet_service()->GetActiveOrigin(base::BindOnce(
      &OnGetActiveOrigin, &callback_called, expected_origin_info->Clone()));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(observer.active_origin_info(), expected_origin_info);

  url = https_server()->GetURL("c.test", "/simple.html");
  expected_origin_info = MakeOriginInfo(url::Origin::Create(url));
  observer.Reset();
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  wallet_service()->GetActiveOrigin(base::BindOnce(
      &OnGetActiveOrigin, &callback_called, expected_origin_info->Clone()));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(observer.active_origin_info(), expected_origin_info);

  url = https_server()->GetURL("d.test", "/simple.html");
  expected_origin_info = MakeOriginInfo(url::Origin::Create(url));
  observer.Reset();
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_WINDOW,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  wallet_service()->GetActiveOrigin(base::BindOnce(
      &OnGetActiveOrigin, &callback_called, expected_origin_info->Clone()));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(observer.active_origin_info(), expected_origin_info);
}

IN_PROC_BROWSER_TEST_F(BraveWalletServiceTest, IsPrivateWindow) {
  TestIsPrivateWindow(wallet_service(), false);
  wallet_service()->SetPrivateWindowsEnabled(true);
  TestIsPrivateWindow(incognito_wallet_service(), true);
  TestIsPrivateWindow(wallet_service(), false);
}

IN_PROC_BROWSER_TEST_F(BraveWalletServiceTest, DisplayTxNotification) {
  AccountUtils account_utils(wallet_service()->keyring_service());
  account_utils.CreateWallet(kMnemonicDripCaution, kTestWalletPassword);
  auto account = account_utils.EnsureEthAccount(0);
  ASSERT_TRUE(account);

  const GURL expected_tx_url("chrome://wallet/crypto/accounts/" +
                             account->address + "/transactions");

  const std::string tx_meta_id = "tx_meta_id";
  auto tx_info = mojom::TransactionInfo::New(
      tx_meta_id, account->account_id.Clone(), "",
      mojom::TxDataUnion::NewEthTxData(
          mojom::TxData::New(mojom::kLocalhostChainId, "0x0", "0x1", "0x5208",
                             "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                             "0x0", std::vector<uint8_t>())),
      mojom::TransactionStatus::Confirmed, mojom::TransactionType::ETHSend,
      std::vector<std::string>(), std::vector<std::string>(),
      base::Milliseconds(0), base::Milliseconds(0), base::Milliseconds(0),
      nullptr, mojom::kLocalhostChainId, std::nullopt, false, nullptr, nullptr);
  tx_service()->OnTransactionStatusChanged(std::move(tx_info));

  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return GetWalletNotifications().size() == 1u; }));

  auto notifications = GetWalletNotifications();
  ASSERT_EQ(notifications.size(), 1u);
  const auto& notification = notifications.front();

  EXPECT_EQ(notification.id(), tx_meta_id);
  EXPECT_EQ(notification.title(),
            l10n_util::GetStringUTF16(
                IDS_WALLET_TRANSACTION_STATUS_UPDATE_MESSAGE_TITLE_CONFIRMED));
  EXPECT_EQ(notification.message(),
            l10n_util::GetStringFUTF16(
                IDS_WALLET_TRANSACTION_STATUS_UPDATE_MESSAGE_TEXT,
                base::UTF8ToUTF16(account->name)));
  EXPECT_EQ(notification.origin_url(), expected_tx_url);
  EXPECT_TRUE(notification.rich_notification_data().remove_on_click);
  EXPECT_EQ(notification.rich_notification_data().context_message, u" ");

  content::TestNavigationObserver nav_observer(expected_tx_url);
  nav_observer.WatchExistingWebContents();
  nav_observer.StartWatchingNewWebContents();
  SimulateWalletNotificationClick(tx_meta_id);
  nav_observer.Wait();
  EXPECT_EQ(browser()
                ->tab_strip_model()
                ->GetActiveWebContents()
                ->GetLastCommittedURL(),
            expected_tx_url);
}

IN_PROC_BROWSER_TEST_F(BraveWalletServiceTest,
                       IsolateTransactionInPrivateWindow) {
  AccountUtils account_utils(wallet_service()->keyring_service());
  account_utils.CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);

  // No transactions in regular profile.
  EXPECT_EQ(0u,
            wallet_service()->tx_service()->GetPendingTransactionsCountSync());

  // Add 1 transaction in regular profile.
  wallet_service()->json_rpc_service()->SetGasPriceForTesting("0x123");
  TestFuture<bool, const std::string&, const std::string&> tx_add_future;
  wallet_service()->tx_service()->AddUnapprovedEvmTransaction(
      mojom::NewEvmTransactionParams::New(
          mojom::kBnbSmartChainMainnetChainId,
          account_utils.EnsureEthAccount(0)->account_id.Clone(),
          "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c", "0x016345785d8a0000",
          "0x0974", std::vector<uint8_t>(), nullptr),
      tx_add_future.GetCallback());
  auto [success, tx_meta_id, error_message] = tx_add_future.Take();
  EXPECT_TRUE(success);

  // 1 transaction in regular profile.
  EXPECT_EQ(1u,
            wallet_service()->tx_service()->GetPendingTransactionsCountSync());

  // No transactions in incognito profile.
  wallet_service()->SetPrivateWindowsEnabled(true);
  WaitForTxStorageInitialized(
      incognito_wallet_service()->tx_service()->GetTxStorageForTesting());
  EXPECT_EQ(0u, incognito_wallet_service()
                    ->tx_service()
                    ->GetPendingTransactionsCountSync());

  // Add 2 transactions in incognito profile.
  incognito_wallet_service()->json_rpc_service()->SetGasPriceForTesting(
      "0x123");
  incognito_wallet_service()->tx_service()->AddUnapprovedEvmTransaction(
      mojom::NewEvmTransactionParams::New(
          mojom::kBnbSmartChainMainnetChainId,
          account_utils.EnsureEthAccount(0)->account_id.Clone(),
          "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c", "0x016345785d8a0000",
          "0x0974", std::vector<uint8_t>(), nullptr),
      tx_add_future.GetCallback());
  std::tie(success, tx_meta_id, error_message) = tx_add_future.Take();
  EXPECT_TRUE(success);
  incognito_wallet_service()->tx_service()->AddUnapprovedEvmTransaction(
      mojom::NewEvmTransactionParams::New(
          mojom::kBnbSmartChainMainnetChainId,
          account_utils.EnsureEthAccount(0)->account_id.Clone(),
          "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c", "0x016345785d8a0000",
          "0x0974", std::vector<uint8_t>(), nullptr),
      tx_add_future.GetCallback());
  std::tie(success, tx_meta_id, error_message) = tx_add_future.Take();
  EXPECT_TRUE(success);

  // 2 transactions in incognito profile.
  EXPECT_EQ(2u, incognito_wallet_service()
                    ->tx_service()
                    ->GetPendingTransactionsCountSync());
  // Still 1 transaction in regular profile.
  EXPECT_EQ(1u,
            wallet_service()->tx_service()->GetPendingTransactionsCountSync());

  CloseIncognitoBrowser();

  // Still 1 transaction in regular profile after incognito closed.
  EXPECT_EQ(1u,
            wallet_service()->tx_service()->GetPendingTransactionsCountSync());

  // No transactions for new incognito browser.
  EXPECT_EQ(0u, incognito_wallet_service()
                    ->tx_service()
                    ->GetPendingTransactionsCountSync());
}

}  // namespace brave_wallet
