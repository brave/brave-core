/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/scoped_observation.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/browser/brave_wallet/brave_wallet_service_delegate_base.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "build/build_config.h"
#include "chrome/browser/permissions/permission_manager_factory.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_profile.h"
#include "components/permissions/permission_manager.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"
#include "third_party/blink/public/common/storage_key/storage_key.h"
#include "url/gurl.h"
#include "url/origin.h"

using content::StoragePartition;

namespace {

constexpr char kBraveUrl[] = "https://brave.com";

class MockDataRemovalObserver : public StoragePartition::DataRemovalObserver {
 public:
  explicit MockDataRemovalObserver(StoragePartition* partition) {
    observation_.Observe(partition);
  }

  MOCK_METHOD4(OnStorageKeyDataCleared,
               void(uint32_t,
                    content::StoragePartition::StorageKeyMatcherFunction,
                    base::Time,
                    base::Time));

 private:
  base::ScopedObservation<StoragePartition,
                          StoragePartition::DataRemovalObserver>
      observation_{this};
};

}  // namespace

namespace brave_wallet {

// Covers BraveWalletService behavior that relies on the browser-layer
// BraveWalletServiceDelegateImpl: wallet permissions stored in a profile and
// the wallet WebUI StoragePartition.
class BraveWalletServiceDelegateImplUnitTest : public testing::Test {
 protected:
  void SetUp() override {
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());

    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));
    auto scoped_disable_autolock =
        BraveWalletServiceDelegateBase::GetScopedDisableAutolockForTesting();
    builder.AddTestingFactory(
        brave_wallet::BraveWalletServiceFactory::GetInstance(),
        base::BindRepeating(
            [](scoped_refptr<network::SharedURLLoaderFactory>
                   shared_url_loader_factory,
               TestingPrefServiceSimple* local_state,
               content::BrowserContext* context)
                -> std::unique_ptr<KeyedService> {
              auto* profile = Profile::FromBrowserContext(context);
              return std::make_unique<BraveWalletService>(
                  shared_url_loader_factory,
                  BraveWalletServiceDelegate::Create(profile),
                  profile->GetPrefs(), local_state);
            },
            url_loader_factory_.GetSafeWeakWrapper(), &local_state_));
    profile_ = builder.Build();
    service_ = brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
        profile_.get());
    ASSERT_TRUE(service_.get());
    keyring_service_ = service_->keyring_service();

    profile_->SetPermissionControllerDelegate(
        base::WrapUnique(static_cast<permissions::PermissionManager*>(
            PermissionManagerFactory::GetInstance()
                ->BuildServiceInstanceForBrowserContext(profile_.get())
                .release())));
  }

  void TearDown() override {
    profile_->SetPermissionControllerDelegate(nullptr);
  }

  PrefService* GetPrefs() { return profile_->GetPrefs(); }

  void SetupWallet() {
    AccountUtils(keyring_service_)
        .CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);
  }

  AccountUtils GetAccountUtils() { return AccountUtils(keyring_service_); }

  content::BrowserTaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  network::TestURLLoaderFactory url_loader_factory_;
  TestingPrefServiceSimple local_state_;
  std::unique_ptr<TestingProfile> profile_;
  raw_ptr<BraveWalletService> service_ = nullptr;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
};

// The scoped testing hook (also installed process-wide by the test suite)
// makes real delegates report autolock as disabled, which keeps the wallet
// unlocked, and restores the previous value when it goes out of scope.
TEST_F(BraveWalletServiceDelegateImplUnitTest, AutolockDisabledByTestingHook) {
  auto* delegate = service_->GetDelegateForTesting();
  ASSERT_TRUE(delegate);
  EXPECT_FALSE(delegate->IsAutolockEnabled());

  // IsLockedSync() is false when no wallet exists, so a wallet is required for
  // this to be meaningful.
  SetupWallet();
  ASSERT_TRUE(keyring_service_->IsWalletCreatedSync());
  ASSERT_FALSE(keyring_service_->IsLockedSync());
  task_environment_.FastForwardBy(base::Minutes(20));
  EXPECT_FALSE(keyring_service_->IsLockedSync());

  const bool before_hook =
      BraveWalletServiceDelegate::Create(profile_.get())->IsAutolockEnabled();
  {
    auto scoped_disable_autolock =
        BraveWalletServiceDelegateBase::GetScopedDisableAutolockForTesting();
    EXPECT_FALSE(BraveWalletServiceDelegate::Create(profile_.get())
                     ->IsAutolockEnabled());
  }
  EXPECT_EQ(
      before_hook,
      BraveWalletServiceDelegate::Create(profile_.get())->IsAutolockEnabled());
}

TEST_F(BraveWalletServiceDelegateImplUnitTest, Reset) {
  SetupWallet();

  service_->SetDefaultBaseCurrency("CAD");
  ASSERT_TRUE(GetPrefs()->HasPrefPath(kDefaultBaseCurrency));

#if !BUILDFLAG(IS_ANDROID)
  auto* partition = profile_->GetDefaultStoragePartition();
  MockDataRemovalObserver observer(partition);
  const auto page_storage_key_callback_valid =
      [&](StoragePartition::StorageKeyMatcherFunction callback) {
        return callback.Run(blink::StorageKey::CreateFirstParty(
            url::Origin::Create(GURL(kBraveUIWalletURL))));
      };
  const auto panel_storage_key_callback_valid =
      [&](StoragePartition::StorageKeyMatcherFunction callback) {
        return callback.Run(blink::StorageKey::CreateFirstParty(
            url::Origin::Create(GURL(kBraveUIWalletPanelURL))));
      };

  EXPECT_CALL(observer, OnStorageKeyDataCleared(
                            StoragePartition::REMOVE_DATA_MASK_ALL,
                            testing::Truly(page_storage_key_callback_valid),
                            base::Time(), base::Time::Max()));
  EXPECT_CALL(observer, OnStorageKeyDataCleared(
                            StoragePartition::REMOVE_DATA_MASK_ALL,
                            testing::Truly(panel_storage_key_callback_valid),
                            base::Time(), base::Time::Max()));
#endif

  const std::string eth_addr = "0x407637cC04893DA7FA4A7C0B58884F82d69eD448";
  const std::string sol_addr = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  const std::string ada_addr =
      "Ae2tdPwUPEZFSi1cTyL1ZL6bgixhc2vSy5heg6Zg9uP7PpumkAJ82Qprt8b";

  auto origin = url::Origin::Create(GURL(kBraveUrl));
  auto* delegate = service_->GetDelegateForTesting();
  ASSERT_TRUE(delegate);

  ASSERT_TRUE(permissions::BraveWalletPermissionContext::AddPermission(
      blink::PermissionType::BRAVE_ETHEREUM, profile_.get(), origin, eth_addr));
  ASSERT_TRUE(permissions::BraveWalletPermissionContext::AddPermission(
      blink::PermissionType::BRAVE_SOLANA, profile_.get(), origin, sol_addr));
  ASSERT_TRUE(permissions::BraveWalletPermissionContext::AddPermission(
      blink::PermissionType::BRAVE_CARDANO, profile_.get(), origin, ada_addr));

  ASSERT_TRUE(delegate->HasPermission(mojom::CoinType::ETH, origin, eth_addr));
  ASSERT_TRUE(delegate->HasPermission(mojom::CoinType::SOL, origin, sol_addr));
  ASSERT_TRUE(delegate->HasPermission(mojom::CoinType::ADA, origin, ada_addr));

  service_->Reset();

  EXPECT_FALSE(delegate->HasPermission(mojom::CoinType::ETH, origin, eth_addr));
  EXPECT_FALSE(delegate->HasPermission(mojom::CoinType::SOL, origin, sol_addr));
  EXPECT_FALSE(delegate->HasPermission(mojom::CoinType::ADA, origin, ada_addr));
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kDefaultBaseCurrency));

#if !BUILDFLAG(IS_ANDROID)
  // The observer is notified synchronously when ClearDataForOrigin starts.
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
  // The service passes no completion callback, so wait for the partition's
  // own deletion tasks. QuotaManager must be done with its database before
  // the profile directory is deleted, since Windows cannot delete open files.
  partition->WaitForDeletionTasksForTesting();
#endif
}

TEST_F(BraveWalletServiceDelegateImplUnitTest,
       RecoverWalletClearsImportedAccountPermissions) {
  SetupWallet();

  base::test::TestFuture<mojom::AccountInfoPtr> import_future;
  keyring_service_->ImportEthereumAccount(
      "Imported",
      "d118a12a1e3b595d7d9e5599370df4ddc58d246a3ae4a795597e50eb6a32afb5",
      import_future.GetCallback());
  auto imported_account = import_future.Take();
  ASSERT_TRUE(imported_account);

  auto derived_account = GetAccountUtils().EnsureEthAccount(0);
  const auto origin = url::Origin::Create(GURL(kBraveUrl));
  const auto imported_account_permission =
      GetAccountPermissionIdentifier(imported_account->account_id);
  const auto derived_account_permission =
      GetAccountPermissionIdentifier(derived_account->account_id);

  ASSERT_TRUE(permissions::BraveWalletPermissionContext::AddPermission(
      blink::PermissionType::BRAVE_ETHEREUM, profile_.get(), origin,
      imported_account_permission));
  ASSERT_TRUE(permissions::BraveWalletPermissionContext::AddPermission(
      blink::PermissionType::BRAVE_ETHEREUM, profile_.get(), origin,
      derived_account_permission));

  auto* delegate = service_->GetDelegateForTesting();
  ASSERT_TRUE(delegate);
  ASSERT_TRUE(delegate->HasPermission(mojom::CoinType::ETH, origin,
                                      imported_account_permission));
  ASSERT_TRUE(delegate->HasPermission(mojom::CoinType::ETH, origin,
                                      derived_account_permission));

  keyring_service_->Lock();
  base::test::TestFuture<mojom::RecoverWalletResult> recover_future;
  keyring_service_->RecoverWallet(kMnemonicDivideCruise, "new password", false,
                                  true, recover_future.GetCallback());
  EXPECT_EQ(recover_future.Get(), mojom::RecoverWalletResult::kSuccess);

  EXPECT_FALSE(delegate->HasPermission(mojom::CoinType::ETH, origin,
                                       imported_account_permission));
  EXPECT_TRUE(delegate->HasPermission(mojom::CoinType::ETH, origin,
                                      derived_account_permission));
}

TEST_F(BraveWalletServiceDelegateImplUnitTest, HasPermissionSync) {
  SetupWallet();

  auto account_id_1 = GetAccountUtils().EnsureEthAccount(0)->account_id.Clone();
  auto account_id_2 = GetAccountUtils().EnsureEthAccount(1)->account_id.Clone();

  // First account addr
  const std::string eth_addr = GetAccountUtils().EthAccount(0)->address;
  auto origin = url::Origin::Create(GURL(kBraveUrl));
  auto* delegate = service_->GetDelegateForTesting();
  ASSERT_TRUE(delegate);

  ASSERT_TRUE(permissions::BraveWalletPermissionContext::AddPermission(
      blink::PermissionType::BRAVE_ETHEREUM, profile_.get(), origin, eth_addr));

  EXPECT_TRUE(service_->HasPermissionSync(origin, account_id_1));
  EXPECT_FALSE(service_->HasPermissionSync(origin, account_id_2));
  EXPECT_FALSE(service_->HasPermissionSync(
      url::Origin::Create(GURL("https://brave.org")), account_id_1));
}

}  // namespace brave_wallet
