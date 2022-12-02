/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/notifications/wallet_notification_service.h"

#include <memory>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/eth_tx_meta.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/notifications/notification_display_service_tester.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

class WalletNotificationServiceUnitTest : public testing::Test {
 public:
  WalletNotificationServiceUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  void SetUp() override {
    json_rpc_service_ =
        std::make_unique<JsonRpcService>(shared_url_loader_factory_, prefs());
    keyring_service_ = std::make_unique<KeyringService>(json_rpc_service_.get(),
                                                        prefs(), local_state());
    tx_service_ = std::make_unique<TxService>(json_rpc_service_.get(),
                                              keyring_service_.get(), prefs());
    notification_service_ =
        std::make_unique<WalletNotificationService>(profile());
    tester_ = std::make_unique<NotificationDisplayServiceTester>(profile());
  }
  Profile* profile() { return &profile_; }
  PrefService* prefs() { return profile_.GetPrefs(); }
  PrefService* local_state() { return &local_state_; }

  bool ShouldDisplayNotifications(mojom::TransactionStatus status) {
    return notification_service_->ShouldDisplayUserNotification(status);
  }

  bool WasNotificationDisplayedOnStatusChange(mojom::TransactionStatus status) {
    std::unique_ptr<EthTransaction> tx = std::make_unique<EthTransaction>(
        *EthTransaction::FromTxData(mojom::TxData::New(
            "0x01", "0x4a817c800", "0x5208",
            "0x3535353535353535353535353535353535353535", "0x0de0b6b3a7640000",
            std::vector<uint8_t>(), false, absl::nullopt)));
    EthTxMeta meta(std::move(tx));
    meta.set_status(status);
    notification_service_->OnTransactionStatusChanged(meta.ToTransactionInfo());
    auto notification = tester_->GetNotification(meta.id());
    tester_->RemoveAllNotifications(NotificationHandler::Type::SEND_TAB_TO_SELF,
                                    true /* by_user */);
    return notification.has_value();
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<NotificationDisplayServiceTester> tester_;
  TestingProfile profile_;
  TestingPrefServiceSimple local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<WalletNotificationService> notification_service_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<TxService> tx_service_;
};

TEST_F(WalletNotificationServiceUnitTest, ShouldShowNotifications) {
  EXPECT_TRUE(ShouldDisplayNotifications(mojom::TransactionStatus::Confirmed));
  EXPECT_TRUE(ShouldDisplayNotifications(mojom::TransactionStatus::Error));
  EXPECT_TRUE(ShouldDisplayNotifications(mojom::TransactionStatus::Dropped));

  EXPECT_FALSE(ShouldDisplayNotifications(mojom::TransactionStatus::Approved));
  EXPECT_FALSE(ShouldDisplayNotifications(mojom::TransactionStatus::Rejected));
  EXPECT_FALSE(ShouldDisplayNotifications(mojom::TransactionStatus::Submitted));
}

TEST_F(WalletNotificationServiceUnitTest, TransactionStatusChanged) {
  EXPECT_TRUE(WasNotificationDisplayedOnStatusChange(
      mojom::TransactionStatus::Confirmed));
  EXPECT_TRUE(
      WasNotificationDisplayedOnStatusChange(mojom::TransactionStatus::Error));
  EXPECT_TRUE(WasNotificationDisplayedOnStatusChange(
      mojom::TransactionStatus::Dropped));

  EXPECT_FALSE(WasNotificationDisplayedOnStatusChange(
      mojom::TransactionStatus::Approved));
  EXPECT_FALSE(WasNotificationDisplayedOnStatusChange(
      mojom::TransactionStatus::Rejected));
  EXPECT_FALSE(WasNotificationDisplayedOnStatusChange(
      mojom::TransactionStatus::Submitted));
}

}  // namespace brave_wallet
