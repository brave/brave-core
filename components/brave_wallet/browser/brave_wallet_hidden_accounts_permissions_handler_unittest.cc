/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_hidden_accounts_permissions_handler.h"

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/strings/string_util.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave_wallet {
namespace {

using testing::_;
using testing::Eq;

class MockKeyringService : public KeyringService {
 public:
  MockKeyringService(PrefService* profile_prefs, PrefService* local_state)
      : KeyringService(/*json_rpc_service=*/nullptr, profile_prefs, local_state) {
  }

  MOCK_METHOD(void,
              AddObserver,
              (::mojo::PendingRemote<mojom::KeyringServiceObserver> observer),
              (override));
  MOCK_METHOD(std::vector<mojom::AccountInfoPtr>,
              GetHiddenAccountsSync,
              (),
              (override));

  void RegisterObserver(
      ::mojo::PendingRemote<mojom::KeyringServiceObserver> observer) {
    observers_.Add(std::move(observer));
  }

  void NotifyAccountsChangedForTest() {
    for (const auto& observer : observers_) {
      observer->AccountsChanged();
    }
  }

 private:
  mojo::RemoteSet<mojom::KeyringServiceObserver> observers_;
};

class MockBraveWalletServiceDelegate : public BraveWalletServiceDelegate {
 public:
  MOCK_METHOD(void,
              GetWebSitesWithPermission,
              (mojom::CoinType coin, GetWebSitesWithPermissionCallback callback),
              (override));
  MOCK_METHOD(bool,
              ResetPermission,
              (mojom::CoinType coin,
               const url::Origin& origin,
               const std::string& account),
              (override));
  MOCK_METHOD(base::FilePath, GetWalletBaseDirectory, (), (override));
  MOCK_METHOD(bool, IsPrivateWindow, (), (override));
};

class BraveWalletHiddenAccountsPermissionsHandlerUnitTest : public testing::Test {
 public:
  BraveWalletHiddenAccountsPermissionsHandlerUnitTest() {
    RegisterProfilePrefs(profile_prefs_.registry());
    RegisterLocalStatePrefs(local_state_.registry());
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
};

TEST_F(BraveWalletHiddenAccountsPermissionsHandlerUnitTest,
       RevokesPermissionForNewlyHiddenAccount) {
  MockKeyringService keyring_service(&profile_prefs_, &local_state_);
  MockBraveWalletServiceDelegate delegate;

  const std::string address = "0x407637cC04893DA7FA4A7C0B58884F82d69eD448";
  const auto account_id =
      mojom::AccountId::New(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                            mojom::AccountKind::kDerived, address, 0,
                            "eth_unique_key");
  const auto hidden_account = mojom::AccountInfo::New(
      account_id->Clone(), address, "Account 1", nullptr);

  ON_CALL(keyring_service, AddObserver(_))
      .WillByDefault([&](::mojo::PendingRemote<mojom::KeyringServiceObserver>
                             observer) {
        keyring_service.RegisterObserver(std::move(observer));
      });
  EXPECT_CALL(keyring_service, AddObserver(_)).Times(1);
  EXPECT_CALL(keyring_service, GetHiddenAccountsSync())
      .Times(2)
      .WillRepeatedly(
          [&]() {
            std::vector<mojom::AccountInfoPtr> accounts;
            accounts.push_back(hidden_account->Clone());
            return accounts;
          });

  const auto expected_origin = url::Origin::Create(GURL("https://app.brave.com"));
  const std::string expected_account_identifier = base::ToLowerASCII(address);
  const std::string website = "https://app.brave.com" + address + "/";

  EXPECT_CALL(delegate, GetWebSitesWithPermission(mojom::CoinType::ETH, _))
      .Times(1)
      .WillOnce([&](mojom::CoinType,
                    BraveWalletServiceDelegate::GetWebSitesWithPermissionCallback cb) {
        std::move(cb).Run({website});
      });
  EXPECT_CALL(delegate,
              ResetPermission(mojom::CoinType::ETH, Eq(expected_origin),
                              Eq(expected_account_identifier)))
      .Times(1)
      .WillOnce(testing::Return(true));

  BraveWalletHiddenAccountsPermissionsHandler handler(keyring_service, delegate);

  keyring_service.NotifyAccountsChangedForTest();
  task_environment_.RunUntilIdle();
}

}  // namespace
}  // namespace brave_wallet
