/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_api_impl.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/json/json_writer.h"
#include "base/memory/raw_ptr.h"
#include "base/task/current_thread.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_dapp_utils.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_test_utils.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"  // IWYU pragma: keep
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"
#include "url/origin.h"

using base::test::TestFuture;
using testing::_;

namespace brave_wallet {

namespace {

class MockBraveWalletProviderDelegate : public BraveWalletProviderDelegate {
 public:
  MockBraveWalletProviderDelegate() = default;
  ~MockBraveWalletProviderDelegate() override {}

  MOCK_METHOD0(IsTabVisible, bool());
  MOCK_METHOD1(ShowPanel, void(const url::Origin&));
  MOCK_METHOD0(ShowWalletBackup, void());
  MOCK_METHOD0(UnlockWallet, void());
  MOCK_METHOD0(WalletInteractionDetected, void());
  MOCK_METHOD2(ShowAccountCreation,
               void(mojom::CoinType type, const url::Origin& origin));
  MOCK_METHOD4(RequestPermissions,
               void(mojom::CoinType type,
                    const std::vector<std::string>& accounts,
                    const url::Origin& origin,
                    RequestPermissionsCallback));
  MOCK_METHOD2(IsAccountAllowed,
               bool(mojom::CoinType type, const std::string& account));
  MOCK_METHOD2(GetAllowedAccounts,
               std::optional<std::vector<std::string>>(
                   mojom::CoinType type,
                   const std::vector<std::string>& accounts));
  MOCK_METHOD1(IsPermissionDenied, bool(mojom::CoinType type));
  MOCK_METHOD1(AddSolanaConnectedAccount, void(const std::string& account));
  MOCK_METHOD1(RemoveSolanaConnectedAccount, void(const std::string& account));
  MOCK_METHOD1(IsSolanaAccountConnected, bool(const std::string& account));
};

}  // namespace

class PolkadotApiImplUnitTest : public testing::Test {
 public:
  PolkadotApiImplUnitTest() {
    feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletPolkadotFeature,
          {{"polkadot_dapp_support", "true"}}}},
        {});
  }

  ~PolkadotApiImplUnitTest() override = default;

  void SetUp() override {
    RegisterLocalStatePrefs(local_state_.registry());
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());

    network_manager_ = std::make_unique<NetworkManager>(&prefs_);

    json_rpc_service_ = std::make_unique<JsonRpcService>(
        url_loader_factory_.GetSafeWeakWrapper(), network_manager_.get(),
        &prefs_, &local_state_);

    keyring_service_ = std::make_unique<KeyringService>(json_rpc_service_.get(),
                                                        &prefs_, &local_state_);

    service_delegate_ = TestBraveWalletServiceDelegate::Create();
    keyring_service_->SetDelegate(service_delegate_.get());

    polkadot_mock_rpc_ = std::make_unique<PolkadotMockRpc>(
        &url_loader_factory_, network_manager_.get());

    polkadot_wallet_service_ = std::make_unique<PolkadotWalletService>(
        *keyring_service_, *network_manager_, prefs_,
        url_loader_factory_.GetSafeWeakWrapper(), NoParachainUrlMap());
  }

  void TearDown() override {
    delegate_ = nullptr;
    api_.reset();
  }

  // Used to create an instance of the PolkadotApiImpl which always grants
  // permission for the provided account.
  void CreateApi(const mojom::AccountIdPtr& granted_account) {
    auto delegate =
        std::make_unique<testing::NiceMock<MockBraveWalletProviderDelegate>>();
    delegate_ = delegate.get();
    ON_CALL(*delegate_,
            IsAccountAllowed(mojom::CoinType::DOT,
                             GetAccountPermissionIdentifier(granted_account)))
        .WillByDefault(testing::Return(true));

    api_ = std::make_unique<PolkadotApiImpl>(
        *keyring_service_, *polkadot_wallet_service_, std::move(delegate),
        granted_account.Clone(),
        url::Origin::Create(GURL("https://brave.com")));
  }

  void CreateWallet() {
    AccountUtils(keyring_service())
        .CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);
  }

  mojom::AccountInfoPtr AddAccount() {
    return AccountUtils(keyring_service()).EnsureDotAccount(0);
  }

  mojom::AccountInfoPtr AddImportedAccount() {
    return AccountUtils(keyring_service()).EnsureDotImportAccount(0);
  }

  // Only for the imported accounts.
  void RemoveAccount(const mojom::AccountIdPtr& account_id) {
    TestFuture<bool> future;
    keyring_service()->RemoveAccount(account_id.Clone(), kTestWalletPassword,
                                     future.GetCallback());
    ASSERT_TRUE(future.Get());
  }

  auto GetAccounts(bool any_type = false) {
    TestFuture<std::optional<std::vector<mojom::PolkadotInjectedAccountPtr>>,
               mojom::PolkadotProviderErrorBundlePtr>
        future;
    api_->GetAccounts(any_type, future.GetCallback());
    return future.Take();
  }

  auto SignPayload(const std::string& payload_json) {
    TestFuture<mojom::PolkadotSignerResultPtr,
               mojom::PolkadotProviderErrorBundlePtr>
        future;
    api_->SignPayload(payload_json, future.GetCallback());
    return future;
  }

  MockBraveWalletProviderDelegate* delegate() { return delegate_; }

  KeyringService* keyring_service() { return keyring_service_.get(); }

  PolkadotWalletService* polkadot_wallet_service() {
    return polkadot_wallet_service_.get();
  }

  PolkadotMockRpc* polkadot_mock_rpc() { return polkadot_mock_rpc_.get(); }

 private:
  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<BraveWalletServiceDelegate> service_delegate_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<PolkadotWalletService> polkadot_wallet_service_;
  std::unique_ptr<PolkadotMockRpc> polkadot_mock_rpc_;

  raw_ptr<MockBraveWalletProviderDelegate> delegate_ = nullptr;
  std::unique_ptr<PolkadotApiImpl> api_;
};

TEST_F(PolkadotApiImplUnitTest, GetAccounts_ReturnsTheGrantedAccount) {
  CreateWallet();
  auto account = AddAccount();
  ASSERT_TRUE(account);
  CreateApi(account->account_id);

  auto [accounts, error] = GetAccounts();

  ASSERT_FALSE(error);
  ASSERT_TRUE(accounts);
  ASSERT_EQ(accounts->size(), 1u);
  EXPECT_THAT(
      accounts->at(0),
      EqualsMojo(mojom::PolkadotInjectedAccount::New(
          account->address, /*genesis_hash=*/std::nullopt,
          kPolkadotObfuscatedAccountName, kPolkadotSr25519KeypairType)));
}

TEST_F(PolkadotApiImplUnitTest, GetAccounts_OnlyEverServesTheGrantedAccount) {
  // Prove that a dApp gets the one account it was granted, never the rest of
  // the wallet.

  CreateWallet();
  auto granted = AddAccount();
  ASSERT_TRUE(granted);
  ASSERT_TRUE(AccountUtils(keyring_service()).EnsureDotAccount(1));
  ASSERT_TRUE(AddImportedAccount());
  CreateApi(granted->account_id);

  auto [accounts, error] = GetAccounts();

  ASSERT_FALSE(error);
  ASSERT_TRUE(accounts);
  ASSERT_EQ(accounts->size(), 1u);
  EXPECT_THAT(
      accounts->at(0),
      EqualsMojo(mojom::PolkadotInjectedAccount::New(
          granted->address, /*genesis_hash=*/std::nullopt,
          kPolkadotObfuscatedAccountName, kPolkadotSr25519KeypairType)));
}

TEST_F(PolkadotApiImplUnitTest, GetAccounts_AnyTypeIsIgnored) {
  // Every Polkadot keypair we hold is sr25519 and can derive further keypairs,
  // so the filter has nothing to filter out.
  // See:
  // https://github.com/polkadot-js/extension/blob/d7c9ce214557e8bd359fac29c4bc38d0e329c1d4/packages/extension-base/src/background/handlers/Tabs.ts#L38

  CreateWallet();
  auto account = AddAccount();
  ASSERT_TRUE(account);
  CreateApi(account->account_id);

  auto [any_type_accounts, any_type_error] = GetAccounts(/*any_type=*/true);
  auto [accounts, error] = GetAccounts(/*any_type=*/false);

  ASSERT_FALSE(any_type_error);
  ASSERT_FALSE(error);
  ASSERT_TRUE(any_type_accounts);
  ASSERT_TRUE(accounts);
  ASSERT_EQ(any_type_accounts->size(), 1u);
  ASSERT_EQ(accounts->size(), 1u);
  EXPECT_THAT(any_type_accounts->at(0), EqualsMojo(accounts->at(0)));
}

TEST_F(PolkadotApiImplUnitTest, GetAccounts_ImportedAccount) {
  CreateWallet();
  auto imported = AddImportedAccount();
  ASSERT_TRUE(imported);
  CreateApi(imported->account_id);

  auto [accounts, error] = GetAccounts();

  ASSERT_FALSE(error);
  ASSERT_TRUE(accounts);
  ASSERT_EQ(accounts->size(), 1u);
  EXPECT_THAT(
      accounts->at(0),
      EqualsMojo(mojom::PolkadotInjectedAccount::New(
          imported->address, /*genesis_hash=*/std::nullopt,
          kPolkadotObfuscatedAccountName, kPolkadotSr25519KeypairType)));
}

TEST_F(PolkadotApiImplUnitTest, GetAccounts_RechecksPermissionOnEveryCall) {
  // Prove that our PolkadotApiImpl will recheck permissions on each call to
  // GetAccounts.

  CreateWallet();
  auto account = AddAccount();
  ASSERT_TRUE(account);
  CreateApi(account->account_id);

  EXPECT_CALL(
      *delegate(),
      IsAccountAllowed(mojom::CoinType::DOT,
                       GetAccountPermissionIdentifier(account->account_id)))
      .Times(2)
      .WillOnce(testing::Return(true))
      .WillOnce(testing::Return(false));

  auto [first_accounts, first_error] = GetAccounts();
  auto [second_accounts, second_error] = GetAccounts();

  EXPECT_TRUE(first_accounts);
  EXPECT_FALSE(second_accounts);
}

TEST_F(PolkadotApiImplUnitTest, GetAccounts_PermissionRevoked) {
  CreateWallet();
  auto account = AddAccount();
  ASSERT_TRUE(account);
  CreateApi(account->account_id);

  ON_CALL(*delegate(), IsAccountAllowed(_, _))
      .WillByDefault(testing::Return(false));

  auto [accounts, error] = GetAccounts();

  EXPECT_FALSE(accounts);
  ASSERT_TRUE(error);
  EXPECT_THAT(error,
              EqualsMojo(mojom::PolkadotProviderErrorBundle::New(
                  mojom::PolkadotProviderError::kUnknown,
                  l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST))));
}

TEST_F(PolkadotApiImplUnitTest, GetAccounts_WalletLocked) {
  CreateWallet();
  auto account = AddAccount();
  ASSERT_TRUE(account);
  CreateApi(account->account_id);
  keyring_service()->Lock();

  auto [accounts, error] = GetAccounts();

  EXPECT_FALSE(accounts);
  ASSERT_TRUE(error);
  EXPECT_THAT(
      error,
      EqualsMojo(mojom::PolkadotProviderErrorBundle::New(
          mojom::PolkadotProviderError::kUnknown,
          l10n_util::GetStringUTF8(IDS_WALLET_REQUEST_PROCESSING_ERROR))));
}

TEST_F(PolkadotApiImplUnitTest, GetAccounts_AccountRemovedAfterGrant) {
  // Prove that if a user removes an imported account, we correctly do not
  // return it.

  CreateWallet();
  auto imported = AddImportedAccount();
  ASSERT_TRUE(imported);
  CreateApi(imported->account_id);

  RemoveAccount(imported->account_id);

  auto [accounts, error] = GetAccounts();

  EXPECT_FALSE(accounts);
  ASSERT_TRUE(error);
  EXPECT_THAT(error, EqualsMojo(mojom::PolkadotProviderErrorBundle::New(
                         mojom::PolkadotProviderError::kInternalError,
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR))));
}

static constexpr std::string_view kPayloadJson = R"({
  "specVersion": "0x001e9808",
  "transactionVersion": "0x0000000f",
  "address": "1UC3h7uQVraXVhGhfPqmk6F7syCLrxMSVbJBuQqW7R8SHdK",
  "assetId": null,
  "blockHash": "0xa4d849853e6d5f0df4e03b3fbc104d7937d8e85585fcbafa06290bfb4de081a9",
  "blockNumber": "0x0148d3ac",
  "era": "0xc400",
  "genesisHash": "0x68d56f15f85d3136970ec16946040bc1752654e906147f7e43e9d539d7c3de2f",
  "metadataHash": null,
  "method": "0x1f03051c000401000007093581e70e3001000003890dff6f1608140d01020800010100f8224098b97b27e917689228c41f23e2b06cc48666bc6eaec030169b441578400f0101010000040201090704000f00163982d4df06000d00040201090704000b00f0ab75a40d000101002a975dad1c0fb19221a9288a0ae4b63ed29f3c625a73578117dde65482b8fc23310201090704010200040201090704000f00268d0c30d20601040100040100000700e40b5402080d01020c0001030083cf32ace9e4c47165934f9a695fc23f96dee73d2ccb4f8d0d25b94af68380348f8eea289c3183f0f0bcb2a1fa2ee9ee89434eba012ccb4f8d0d25b94af68380348f8eea289c3183f0f0bcb2a1fa2ee9ee89434eba0103c83d82a9d2060300",
  "mode": 0,
  "nonce": "0x00000001",
  "signedExtensions": [
    "AuthorizeCall",
    "CheckNonZeroSender",
    "CheckSpecVersion",
    "CheckTxVersion",
    "CheckGenesis",
    "CheckMortality",
    "CheckNonce",
    "CheckWeight",
    "ChargeAssetTxPayment",
    "PrevalidateAttests",
    "CheckMetadataHash",
    "EthSetOrigin",
    "StorageWeightReclaim",
    "UnitTransactionExtension",
    "VerifyMultiSignature",
    "AsPgas",
    "AsDotnsGateway",
    "RestrictOrigins"
  ],
  "tip": "0x00000000000000000000000000000000",
  "version": 4,
  "withSignedTransaction": false
})";

TEST_F(PolkadotApiImplUnitTest, SignPayload) {
  polkadot_mock_rpc()->FinalizeSetup();

  CreateWallet();

  auto account = AddAccount();
  ASSERT_TRUE(account);
  CreateApi(account->account_id);

  auto future = SignPayload(std::string(kPayloadJson));

  base::test::RunUntil([this] {
    return !polkadot_wallet_service()
                ->GetPendingSignPolkadotTransactionRequestsSync()
                .empty();
  });

  polkadot_wallet_service()->NotifySignPolkadotTransactionRequestProcessed(
      true, 0, std::nullopt);

  auto [signature, error] = future.Take();
  EXPECT_FALSE(error);
  ASSERT_TRUE(signature);
}

}  // namespace brave_wallet
