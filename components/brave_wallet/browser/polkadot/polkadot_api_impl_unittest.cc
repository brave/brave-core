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
#include "base/json/json_writer.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/strcat.h"  // IWYU pragma: keep
#include "base/strings/string_number_conversions.h"
#include "base/task/current_thread.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_dapp_utils.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_keyring.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_test_utils.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
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

constexpr uint32_t kSpecVersion = 0x001e9808;

constexpr std::string_view kPayloadJson = R"({
  "specVersion": "0x001e9808",
  "transactionVersion": "0x0000000f",
  "address": "16cM3vo8Ay8g9PxEaHFG2HZmZ8TYHf4gYAjBm3zAjezTCUMt",
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

constexpr std::string_view kSignaturePayloadHex =
    "1f03051c000401000007093581e70e3001000003890dff6f1608140d01020800010100f8"
    "224098b97b27e917689228c41f23e2b06cc48666bc6eaec030169b441578400f01010100"
    "00040201090704000f00163982d4df06000d00040201090704000b00f0ab75a40d000101"
    "002a975dad1c0fb19221a9288a0ae4b63ed29f3c625a73578117dde65482b8fc23310201"
    "090704010200040201090704000f00268d0c30d20601040100040100000700e40b540208"
    "0d01020c0001030083cf32ace9e4c47165934f9a695fc23f96dee73d2ccb4f8d0d25b94a"
    "f68380348f8eea289c3183f0f0bcb2a1fa2ee9ee89434eba012ccb4f8d0d25b94af68380"
    "348f8eea289c3183f0f0bcb2a1fa2ee9ee89434eba0103c83d82a9d2060300c400040000"
    "0008981e000f00000068d56f15f85d3136970ec16946040bc1752654e906147f7e43e9d5"
    "39d7c3de2fa4d849853e6d5f0df4e03b3fbc104d7937d8e85585fcbafa06290bfb4de081"
    "a900";

// Signifies sr25519 signature type.
constexpr std::string_view kSignaturePrefix = "0x01";

// Matches the on-chain signature here:
// https://assethub-polkadot.subscan.io/extrinsic/21550016-2
constexpr std::string_view kExpectedSignature =
    "0x01"  // sr25519 leading prefix.
    "f0fafb0678790218f20da6b6ea4d160222f6bad7d051ef936d2bae255c07547b"
    "01aa8300fffec58b954bcc4dd95b4d1dfd96559e32dbbd28c829873cf99a8389";

constexpr std::string_view kAliceAddress =
    "15oF4uVJwmo4TdGW7VfQxNLavjCXviqxT9S1MgbjMNHr6Sp5";

constexpr std::string_view kUnknownGenesisHash =
    "0x0000000000000000000000000000000000000000000000000000000000000000";

constexpr char kDappUrl[] = "https://brave.com";

std::vector<uint8_t> SignaturePayloadBytes() {
  std::vector<uint8_t> bytes;
  CHECK(base::HexStringToBytes(kSignaturePayloadHex, &bytes));
  return bytes;
}

// `kPayloadJson` with `key` set to `value`, as a dapp passing a different field
// would have sent it.
std::string PayloadWith(std::string_view key, base::Value value) {
  auto payload = base::test::ParseJsonDict(kPayloadJson);
  payload.Set(key, std::move(value));
  return base::WriteJson(payload).value();
}

std::string PayloadWithout(std::string_view key) {
  auto payload = base::test::ParseJsonDict(kPayloadJson);
  CHECK(payload.Remove(key));
  return base::WriteJson(payload).value();
}

mojom::PolkadotProviderErrorBundlePtr InternalErrorBundle() {
  return mojom::PolkadotProviderErrorBundle::New(
      mojom::PolkadotProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

mojom::PolkadotProviderErrorBundlePtr UserRejectedErrorBundle() {
  return mojom::PolkadotProviderErrorBundle::New(
      mojom::PolkadotProviderError::kUnknown,
      l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
}

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
        url_loader_factory_.GetSafeWeakWrapper(), NoParachainUrlMap(),
        NoOriginPermissionChecking());
  }

  void TearDown() override { DestroyApi(); }

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
        granted_account.Clone(), url::Origin::Create(GURL(kDappUrl)));
  }

  void DestroyApi() {
    // `api_` owns the delegate.
    delegate_ = nullptr;
    api_.reset();
  }

  void CreateWallet() {
    std::string mnemonic =
        "promote bronze dragon ankle spell bus quantum wheat drum captain "
        "frozen cheap";
    AccountUtils(keyring_service()).CreateWallet(mnemonic, kTestWalletPassword);
  }

  void SetKeyringSeed() {
    auto* keyring = keyring_service()->GetKeyring<PolkadotKeyring>(
        mojom::KeyringId::kPolkadotMainnet);
    ASSERT_TRUE(keyring);
    keyring->SetMockRndSeedForTesting(1234);
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

  mojom::AccountInfoPtr SetUpGrantedAccount() {
    CreateWallet();
    auto account = AddAccount();
    if (!account) {
      return nullptr;
    }
    SetKeyringSeed();
    CreateApi(account->account_id);
    return account;
  }

  mojom::SignPolkadotTransactionRequestPtr WaitForPendingRequest() {
    if (!base::test::RunUntil([this] {
          return !polkadot_wallet_service()
                      ->GetPendingSignPolkadotTransactionRequestsSync()
                      .empty();
        })) {
      return nullptr;
    }
    return std::move(polkadot_wallet_service()
                         ->GetPendingSignPolkadotTransactionRequestsSync()
                         .front());
  }

  mojom::SignPolkadotTransactionRequestPtr WaitForPendingRequest(
      size_t num_requests) {
    if (!base::test::RunUntil([this, num_requests] {
          return polkadot_wallet_service()
                     ->GetPendingSignPolkadotTransactionRequestsSync()
                     .size() == num_requests;
        })) {
      return nullptr;
    }
    return std::move(polkadot_wallet_service()
                         ->GetPendingSignPolkadotTransactionRequestsSync()
                         .front());
  }

  bool HasPendingRequests() {
    return !polkadot_wallet_service()
                ->GetPendingSignPolkadotTransactionRequestsSync()
                .empty();
  }

  void RevokeGrantedAccount() {
    ON_CALL(*delegate(), IsAccountAllowed(_, _))
        .WillByDefault(testing::Return(false));
  }

  void AddRuntimeVersionForPayload(uint32_t spec_version) {
    polkadot_mock_rpc_->AddGetLatestRuntimeVersion(spec_version);
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
  EXPECT_THAT(error, EqualsMojo(UserRejectedErrorBundle()));
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
  EXPECT_THAT(error, EqualsMojo(InternalErrorBundle()));
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_ApprovedRequestReturnsSignature) {
  // Idealized happy path where we create a signing request and sign it.

  AddRuntimeVersionForPayload(kSpecVersion);
  polkadot_mock_rpc()->FinalizeSetup();
  ASSERT_TRUE(SetUpGrantedAccount());

  auto future = SignPayload(std::string(kPayloadJson));

  auto request = WaitForPendingRequest();
  ASSERT_TRUE(request);

  polkadot_wallet_service()->SetSignPolkadotTransactionRequestSignaturePayload(
      request->id, SignaturePayloadBytes());
  polkadot_wallet_service()->NotifySignPolkadotTransactionRequestProcessed(
      true, request->id, std::nullopt);

  auto [result, error] = future.Take();
  EXPECT_FALSE(error);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->signature, kExpectedSignature);
  EXPECT_FALSE(HasPendingRequests());
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_ApprovedRequestPayloadBoundaries) {
  // Polkadot requires us to hash signature payloads larger than 256 bytes
  // before signing. This test proves these boundary conditions.

  AddRuntimeVersionForPayload(kSpecVersion);
  polkadot_mock_rpc()->FinalizeSetup();

  auto granted_account = SetUpGrantedAccount();
  ASSERT_TRUE(granted_account);

  {
    // Exactly 256 bytes.

    auto future = SignPayload(std::string(kPayloadJson));

    auto request = WaitForPendingRequest();
    ASSERT_TRUE(request);

    auto signature_payload = SignaturePayloadBytes();
    ASSERT_GT(signature_payload.size(),
              kPolkadotMaxUnhashedSignaturePayloadSize);

    signature_payload.erase(
        signature_payload.begin() + kPolkadotMaxUnhashedSignaturePayloadSize,
        signature_payload.end());

    ASSERT_EQ(signature_payload.size(),
              kPolkadotMaxUnhashedSignaturePayloadSize);

    polkadot_wallet_service()
        ->SetSignPolkadotTransactionRequestSignaturePayload(request->id,
                                                            signature_payload);
    polkadot_wallet_service()->NotifySignPolkadotTransactionRequestProcessed(
        true, request->id, std::nullopt);

    auto [result, error] = future.Take();
    EXPECT_FALSE(error);
    ASSERT_TRUE(result);
    EXPECT_FALSE(HasPendingRequests());

    auto signature = keyring_service()->SignMessageByPolkadotKeyring(
        granted_account->account_id, signature_payload);
    ASSERT_TRUE(signature);

    auto expected = base::StrCat(
        {kSignaturePrefix, base::HexEncodeLower(signature.value())});
    ASSERT_EQ(expected, result->signature);
  }

  {
    // Exactly 257 bytes.

    auto future = SignPayload(std::string(kPayloadJson));

    auto request = WaitForPendingRequest();
    ASSERT_TRUE(request);

    auto signature_payload = SignaturePayloadBytes();
    ASSERT_GT(signature_payload.size(),
              kPolkadotMaxUnhashedSignaturePayloadSize + 1);

    signature_payload.erase(signature_payload.begin() +
                                kPolkadotMaxUnhashedSignaturePayloadSize + 1,
                            signature_payload.end());

    ASSERT_EQ(signature_payload.size(),
              kPolkadotMaxUnhashedSignaturePayloadSize + 1);

    polkadot_wallet_service()
        ->SetSignPolkadotTransactionRequestSignaturePayload(request->id,
                                                            signature_payload);
    polkadot_wallet_service()->NotifySignPolkadotTransactionRequestProcessed(
        true, request->id, std::nullopt);

    auto [result, error] = future.Take();
    EXPECT_FALSE(error);
    ASSERT_TRUE(result);
    EXPECT_FALSE(HasPendingRequests());

    auto signature = keyring_service()->SignMessageByPolkadotKeyring(
        granted_account->account_id, Blake2bHash<32>({signature_payload}));
    ASSERT_TRUE(signature);

    auto expected = base::StrCat(
        {kSignaturePrefix, base::HexEncodeLower(signature.value())});
    ASSERT_EQ(expected, result->signature);
  }
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_QueuesTheRequestAndShowsThePanel) {
  // Prove the panel gets everything it needs to describe the request, and that
  // the request carries no signature payload until something describes it.

  AddRuntimeVersionForPayload(kSpecVersion);
  polkadot_mock_rpc()->FinalizeSetup();
  auto account = SetUpGrantedAccount();
  ASSERT_TRUE(account);

  EXPECT_CALL(*delegate(), ShowPanel(url::Origin::Create(GURL(kDappUrl))));

  auto future = SignPayload(std::string(kPayloadJson));

  auto request = WaitForPendingRequest();
  ASSERT_TRUE(request);

  EXPECT_THAT(request->account_id, EqualsMojo(account->account_id));
  EXPECT_THAT(
      request->chain_id,
      EqualsMojo(mojom::ChainId::New(
          mojom::CoinType::DOT, std::string(mojom::kPolkadotMainnetAssetHub))));
  EXPECT_EQ(request->dapp_origin_info->origin_spec, kDappUrl);
  EXPECT_EQ(base::test::ParseJsonDict(request->raw_payload_json),
            base::test::ParseJsonDict(kPayloadJson));
  EXPECT_FALSE(request->metadata_bytes.empty());
  ASSERT_TRUE(request->chain_properties);
  EXPECT_THAT(request->chain_properties->token_symbol,
              testing::ElementsAre("DOT"));
  EXPECT_THAT(request->chain_properties->token_decimals,
              testing::ElementsAre(10u));
  EXPECT_FALSE(request->signature_payload);
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_MetadataFetchFails) {
  // Without metadata nothing can decode the call, so there is nothing to show
  // the user. Never reaches the queue.

  AddRuntimeVersionForPayload(kSpecVersion);
  polkadot_mock_rpc()->UseInvalidChainMetadata();
  polkadot_mock_rpc()->FinalizeSetup();
  ASSERT_TRUE(SetUpGrantedAccount());

  EXPECT_CALL(*delegate(), ShowPanel(_)).Times(0);

  auto [result, error] = SignPayload(std::string(kPayloadJson)).Take();

  EXPECT_FALSE(result);
  ASSERT_TRUE(error);
  EXPECT_THAT(error, EqualsMojo(InternalErrorBundle()));
  EXPECT_FALSE(HasPendingRequests());
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_SystemPropertiesFetchFails) {
  // Without the system properties, we can't accurately use `.toHuman()` which
  // is a requisite for signing.

  AddRuntimeVersionForPayload(kSpecVersion);
  polkadot_mock_rpc()->RejectSystemPropertiesRequest();
  polkadot_mock_rpc()->FinalizeSetup();
  ASSERT_TRUE(SetUpGrantedAccount());

  EXPECT_CALL(*delegate(), ShowPanel(_)).Times(0);

  auto [result, error] = SignPayload(std::string(kPayloadJson)).Take();

  EXPECT_FALSE(result);
  ASSERT_TRUE(error);
  EXPECT_THAT(error, EqualsMojo(InternalErrorBundle()));
  EXPECT_FALSE(HasPendingRequests());
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_RuntimeVersionFetchFails) {
  // We need the runtime version to compare the specVersion in the provided
  // payload to what the chain is currently running.

  polkadot_mock_rpc()->RejectRuntimeVersion();
  AddRuntimeVersionForPayload(kSpecVersion);
  polkadot_mock_rpc()->FinalizeSetup();
  ASSERT_TRUE(SetUpGrantedAccount());

  EXPECT_CALL(*delegate(), ShowPanel(_)).Times(0);

  auto [result, error] = SignPayload(std::string(kPayloadJson)).Take();

  EXPECT_FALSE(result);
  ASSERT_TRUE(error);
  EXPECT_THAT(error, EqualsMojo(InternalErrorBundle()));
  EXPECT_FALSE(HasPendingRequests());
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_RuntimeVersionSpecMismatch) {
  // Reject the request when the specVersion in the payload doesn't match the
  // on-chain specVersion.

  AddRuntimeVersionForPayload(kSpecVersion + 2);
  polkadot_mock_rpc()->FinalizeSetup();
  ASSERT_TRUE(SetUpGrantedAccount());

  EXPECT_CALL(*delegate(), ShowPanel(_)).Times(0);

  auto [result, error] = SignPayload(std::string(kPayloadJson)).Take();

  EXPECT_FALSE(result);
  ASSERT_TRUE(error);
  EXPECT_THAT(error, EqualsMojo(UserRejectedErrorBundle()));
  EXPECT_FALSE(HasPendingRequests());
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_AddressIsAnotherOwnedAccount) {
  // A dapp holding a grant for one account cannot have us sign for another one
  // we hold, and the user is never asked about it.

  CreateWallet();
  auto granted = AddAccount();
  ASSERT_TRUE(granted);
  auto other = AccountUtils(keyring_service()).EnsureDotAccount(1);
  ASSERT_TRUE(other);
  ASSERT_NE(granted->address, other->address);
  SetKeyringSeed();
  CreateApi(granted->account_id);

  EXPECT_CALL(*delegate(), ShowPanel(_)).Times(0);

  auto [result, error] =
      SignPayload(PayloadWith("address", base::Value(other->address))).Take();

  EXPECT_FALSE(result);
  ASSERT_TRUE(error);
  EXPECT_THAT(error, EqualsMojo(UserRejectedErrorBundle()));
  EXPECT_FALSE(HasPendingRequests());
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_InvalidAddresses) {
  // Like SignPayload_AddressIsAnotherOwnedAccount, but this time with an
  // address not owned by our keyring and one that's not a valid ss58 encoding.

  ASSERT_TRUE(SetUpGrantedAccount());

  EXPECT_CALL(*delegate(), ShowPanel(_)).Times(0);

  for (std::string_view address :
       {kAliceAddress, std::string_view("not-an-ss58-address")}) {
    SCOPED_TRACE(address);

    auto [result, error] =
        SignPayload(PayloadWith("address", base::Value(address))).Take();

    EXPECT_FALSE(result);
    ASSERT_TRUE(error);
    EXPECT_THAT(error, EqualsMojo(UserRejectedErrorBundle()));
    EXPECT_FALSE(HasPendingRequests());
  }
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_ApprovedWithoutASignaturePayload) {
  // Prove that we can handle the edge case where we're attempting to sign a
  // request that's missing its signing payload.

  AddRuntimeVersionForPayload(kSpecVersion);
  polkadot_mock_rpc()->FinalizeSetup();
  ASSERT_TRUE(SetUpGrantedAccount());

  auto future = SignPayload(std::string(kPayloadJson));

  auto request = WaitForPendingRequest();
  ASSERT_TRUE(request);
  ASSERT_FALSE(request->signature_payload);

  polkadot_wallet_service()->NotifySignPolkadotTransactionRequestProcessed(
      true, request->id, std::nullopt);

  auto [result, error] = future.Take();
  EXPECT_FALSE(result);
  ASSERT_TRUE(error);
  EXPECT_THAT(error, EqualsMojo(InternalErrorBundle()));
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_UserRejects) {
  // User explicitly rejects the transaction.

  AddRuntimeVersionForPayload(kSpecVersion);
  polkadot_mock_rpc()->FinalizeSetup();
  ASSERT_TRUE(SetUpGrantedAccount());

  auto future = SignPayload(std::string(kPayloadJson));

  auto request = WaitForPendingRequest();
  ASSERT_TRUE(request);

  polkadot_wallet_service()->SetSignPolkadotTransactionRequestSignaturePayload(
      request->id, SignaturePayloadBytes());
  polkadot_wallet_service()->NotifySignPolkadotTransactionRequestProcessed(
      false, request->id, std::nullopt);

  auto [result, error] = future.Take();
  EXPECT_FALSE(result);
  ASSERT_TRUE(error);
  EXPECT_THAT(error, EqualsMojo(UserRejectedErrorBundle()));
  EXPECT_FALSE(HasPendingRequests());
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_PanelRejectsWithAnError) {
  // Assume that our panel UI component failed to decode the transaction and
  // that it auto-rejects the transaction on the user's behalf.

  AddRuntimeVersionForPayload(kSpecVersion);
  polkadot_mock_rpc()->FinalizeSetup();
  ASSERT_TRUE(SetUpGrantedAccount());

  auto future = SignPayload(std::string(kPayloadJson));

  auto request = WaitForPendingRequest();
  ASSERT_TRUE(request);

  polkadot_wallet_service()->NotifySignPolkadotTransactionRequestProcessed(
      false, request->id, "could not decode the call");

  auto [result, error] = future.Take();
  EXPECT_FALSE(result);
  ASSERT_TRUE(error);
  EXPECT_THAT(error, EqualsMojo(InternalErrorBundle()));
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_NotifyForAnotherRequestIsIgnored) {
  // Only the request at the front of the queue can be answered, so a stale id
  // from the panel must not resolve the pending call.

  AddRuntimeVersionForPayload(kSpecVersion);
  polkadot_mock_rpc()->FinalizeSetup();
  ASSERT_TRUE(SetUpGrantedAccount());

  auto future = SignPayload(std::string(kPayloadJson));
  auto future2 = SignPayload(std::string(kPayloadJson));

  auto request = WaitForPendingRequest(2);
  ASSERT_TRUE(request);

  polkadot_wallet_service()->NotifySignPolkadotTransactionRequestProcessed(
      true, request->id + 1, std::nullopt);

  EXPECT_FALSE(future.IsReady());
  EXPECT_FALSE(future2.IsReady());
  EXPECT_TRUE(HasPendingRequests());
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_PermissionRevokedUpFront) {
  // Prove that an up-front revocation of permissions doesn't enqueue a request.

  ASSERT_TRUE(SetUpGrantedAccount());
  RevokeGrantedAccount();

  auto [result, error] = SignPayload(std::string(kPayloadJson)).Take();

  EXPECT_FALSE(result);
  ASSERT_TRUE(error);
  EXPECT_THAT(error, EqualsMojo(UserRejectedErrorBundle()));
  EXPECT_FALSE(HasPendingRequests());
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_PermissionRevokedDuringFetches) {
  // Permission is rechecked after the fetches land, so a revoke in flight never
  // reaches the queue.

  AddRuntimeVersionForPayload(kSpecVersion);
  polkadot_mock_rpc()->FinalizeSetup();
  ASSERT_TRUE(SetUpGrantedAccount());

  EXPECT_CALL(*delegate(), IsAccountAllowed(mojom::CoinType::DOT, _))
      .WillOnce(testing::Return(true))
      .WillRepeatedly(testing::Return(false));
  EXPECT_CALL(*delegate(), ShowPanel(_)).Times(0);

  auto [result, error] = SignPayload(std::string(kPayloadJson)).Take();

  EXPECT_FALSE(result);
  ASSERT_TRUE(error);
  EXPECT_THAT(error, EqualsMojo(UserRejectedErrorBundle()));
  EXPECT_FALSE(HasPendingRequests());
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_PermissionRevokedWhileQueued) {
  // The user can take long enough over the panel for the grant to be revoked
  // underneath it.

  AddRuntimeVersionForPayload(kSpecVersion);
  polkadot_mock_rpc()->FinalizeSetup();
  ASSERT_TRUE(SetUpGrantedAccount());

  auto future = SignPayload(std::string(kPayloadJson));

  auto request = WaitForPendingRequest();
  ASSERT_TRUE(request);

  polkadot_wallet_service()->SetSignPolkadotTransactionRequestSignaturePayload(
      request->id, SignaturePayloadBytes());
  RevokeGrantedAccount();
  polkadot_wallet_service()->NotifySignPolkadotTransactionRequestProcessed(
      true, request->id, std::nullopt);

  auto [result, error] = future.Take();
  EXPECT_FALSE(result);
  ASSERT_TRUE(error);
  EXPECT_THAT(error, EqualsMojo(UserRejectedErrorBundle()));
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_WalletLockedUpFront) {
  // Prove that if our wallet is locked that nothing can enqueue a request.

  ASSERT_TRUE(SetUpGrantedAccount());
  keyring_service()->Lock();

  auto [result, error] = SignPayload(std::string(kPayloadJson)).Take();

  EXPECT_FALSE(result);
  ASSERT_TRUE(error);
  EXPECT_THAT(
      error,
      EqualsMojo(mojom::PolkadotProviderErrorBundle::New(
          mojom::PolkadotProviderError::kUnknown,
          l10n_util::GetStringUTF8(IDS_WALLET_REQUEST_PROCESSING_ERROR))));
  EXPECT_FALSE(HasPendingRequests());
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_WalletLockedWhileQueued) {
  // Prove that a Notify call when the wallet is locked doesn't actually yield a
  // signature.

  AddRuntimeVersionForPayload(kSpecVersion);
  polkadot_mock_rpc()->FinalizeSetup();
  ASSERT_TRUE(SetUpGrantedAccount());

  auto future = SignPayload(std::string(kPayloadJson));

  auto request = WaitForPendingRequest();
  ASSERT_TRUE(request);

  polkadot_wallet_service()->SetSignPolkadotTransactionRequestSignaturePayload(
      request->id, SignaturePayloadBytes());

  keyring_service()->Lock();
  polkadot_wallet_service()->NotifySignPolkadotTransactionRequestProcessed(
      true, request->id, std::nullopt);

  auto [result, error] = future.Take();
  EXPECT_FALSE(result);
  ASSERT_TRUE(error);
  EXPECT_THAT(error, EqualsMojo(InternalErrorBundle()));
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_InvalidPayloadIsInternalError) {
  ASSERT_TRUE(SetUpGrantedAccount());

  struct {
    std::string name;
    std::string payload;
  } test_cases[] = {
      // Prove that invalid JSON doesn't create a request.
      {"MalformedJSON", "{ not json"},
      // Without an address we cannot tell the transaction is meant for the
      // granted account, so there is nothing safe to sign.
      {"MissingAddress", PayloadWithout("address")},
      // The genesis hash is what tells us which chain's metadata to fetch.
      {"MissingGenesisHash", PayloadWithout("genesisHash")},
      // A chain we do not know is a chain we cannot fetch metadata for, so we
      // cannot decode the transaction.
      {"UnknownGenesisHash",
       PayloadWith("genesisHash", base::Value(kUnknownGenesisHash))}};

  for (const auto& [name, payload] : test_cases) {
    SCOPED_TRACE(name);

    auto [result, error] = SignPayload(payload).Take();

    EXPECT_FALSE(result);
    ASSERT_TRUE(error);
    EXPECT_THAT(error, EqualsMojo(InternalErrorBundle()));
    EXPECT_FALSE(HasPendingRequests());
  }
}

TEST_F(PolkadotApiImplUnitTest, SignPayload_RequestCleanUp) {
  AddRuntimeVersionForPayload(kSpecVersion);
  polkadot_mock_rpc()->FinalizeSetup();
  ASSERT_TRUE(SetUpGrantedAccount());

  auto future = SignPayload(std::string(kPayloadJson));

  WaitForPendingRequest();
  EXPECT_TRUE(HasPendingRequests());

  DestroyApi();
  EXPECT_FALSE(HasPendingRequests());
}

}  // namespace brave_wallet
