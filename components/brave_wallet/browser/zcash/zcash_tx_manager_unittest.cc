/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_tx_manager.h"

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/internal/orchard_sync_state.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/browser/tx_storage.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "brave/components/services/brave_wallet/public/mojom/zcash_decoder.mojom.h"
#include "build/build_config.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::NiceMock;
using testing::StrictMock;

namespace brave_wallet {

namespace {

// Testnet transparent address, reused from
// zcash_wallet_service_unittest.cc's GetTransactionType_Testnet
// "Normal transparent address - testnet" case. Distinct from any address
// derived for the test account created below.
constexpr char kExternalTransparentAddress[] =
    "tmP3uLtGx5GPddkq8a6ddmXhqJJ3vy6tpTE";

constexpr uint64_t kSendAmount = 50000u;

// Chain tip heights used to control ZCashTxManager's `ironwood_active`
// computation (see IsIronwoodActive/kIronwoodActivationHeightTestnet in
// zcash_utils.h).
constexpr uint32_t kIronwoodInactiveHeight = 1000u;
constexpr uint32_t kIronwoodActiveHeight = kIronwoodActivationHeightTestnet;

class MockZCashRpc : public ZCashRpc {
 public:
  MockZCashRpc() : ZCashRpc(nullptr, nullptr) {}
  ~MockZCashRpc() override = default;

  MOCK_METHOD(void,
              GetLatestBlock,
              (const std::string& chain_id, GetLatestBlockCallback callback),
              (override));
};

// Mocks all 8 ZCashWalletService::Create*Transaction routing targets so
// ZCashTxManager::ContinueAddUnapprovedTransactionWithHeight's switch can be
// exercised without any real signing/proving.
class MockZCashWalletService : public TestingZCashWalletService {
 public:
  using TestingZCashWalletService::TestingZCashWalletService;

  MOCK_METHOD(void,
              CreateFullyTransparentTransaction,
              (mojom::AccountIdPtr account_id,
               const std::string& address_to,
               uint64_t amount,
               CreateTransactionCallback callback),
              (override));

  MOCK_METHOD(void,
              CreateOrchardToOrchardTransaction,
              (mojom::AccountIdPtr account_id,
               const std::string& address_to,
               uint64_t amount,
               std::optional<OrchardMemo> memo,
               CreateTransactionCallback callback),
              (override));

  MOCK_METHOD(void,
              CreateTransparentToOrchardTransaction,
              (mojom::AccountIdPtr account_id,
               const std::string& address_to,
               uint64_t amount,
               std::optional<OrchardMemo> memo,
               CreateTransactionCallback callback),
              (override));

  MOCK_METHOD(void,
              CreateOrchardToTransparentTransaction,
              (mojom::AccountIdPtr account_id,
               const std::string& address_to,
               uint64_t amount,
               CreateTransactionCallback callback),
              (override));

  MOCK_METHOD(void,
              CreateTransparentToIronwoodTransaction,
              (mojom::AccountIdPtr account_id,
               const std::string& address_to,
               uint64_t amount,
               std::optional<OrchardMemo> memo,
               CreateTransactionCallback callback),
              (override));

  MOCK_METHOD(void,
              CreateIronwoodToIronwoodTransaction,
              (mojom::AccountIdPtr account_id,
               const std::string& address_to,
               uint64_t amount,
               std::optional<OrchardMemo> memo,
               CreateTransactionCallback callback),
              (override));

  MOCK_METHOD(void,
              CreateOrchardToIronwoodTransaction,
              (mojom::AccountIdPtr account_id,
               const std::string& address_to,
               uint64_t amount,
               std::optional<OrchardMemo> memo,
               CreateTransactionCallback callback),
              (override));

  MOCK_METHOD(void,
              CreateIronwoodToTransparentTransaction,
              (mojom::AccountIdPtr account_id,
               const std::string& address_to,
               uint64_t amount,
               CreateTransactionCallback callback),
              (override));
};

}  // namespace

// Exercises ZCashTxManager::ContinueAddUnapprovedTransactionWithHeight's
// routing switch end-to-end through the public
// TxService::AddUnapprovedZCashTransaction() entry point (mirrors the
// mojom::TxService interface used in production). All 8
// ZCashWalletService::Create*Transaction methods are mocked out via
// StrictMock so each test can assert that exactly the expected method was
// invoked and that no other Create* method fired.
//
// Note: TxService::GetZCashTxManager() is private and (unlike
// BitcoinTxManagerUnitTest/CardanoTxManagerUnitTest/etc.) TxService does not
// grant friendship to a ZCashTxManagerUnitTest class, so this test drives
// everything through the public mojom::TxService::AddUnapprovedZCashTransaction
// override instead of reaching into the manager directly.
class ZCashTxManagerUnitTest : public testing::Test {
 public:
  void SetUp() override {
    feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletZCashFeature,
          {{"zcash_shielded_transactions_enabled", "true"},
           {"zcash_ironwood_transaction_enabled", "true"}}},
#if BUILDFLAG(IS_IOS)
         {features::kBraveWalletWebUIFeature, {}}
#endif
        },
        {}  // disabled features
    );

    ASSERT_TRUE(tx_storage_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(orchard_sync_state_dir_.CreateUniqueTempDir());

    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterProfilePrefsForMigration(prefs_.registry());

    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    json_rpc_service_ = std::make_unique<JsonRpcService>(
        url_loader_factory_.GetSafeWeakWrapper(), network_manager_.get(),
        &prefs_, nullptr);
    keyring_service_ = std::make_unique<KeyringService>(
        json_rpc_service_.get(), &prefs_, &local_state_);

    zcash_wallet_service_ =
        std::make_unique<StrictMock<MockZCashWalletService>>(
            *keyring_service_, std::make_unique<NiceMock<MockZCashRpc>>());
    zcash_wallet_service_->SetupSyncState(
        OrchardSyncState::CreateSyncStateSequence(),
        std::make_unique<OrchardSyncState>(orchard_sync_state_dir_.GetPath()));

    tx_service_ = std::make_unique<TxService>(
        json_rpc_service_.get(), /*bitcoin_wallet_service=*/nullptr,
        zcash_wallet_service_.get(), /*cardano_wallet_service=*/nullptr,
        /*polkadot_wallet_service=*/nullptr, *keyring_service_, &prefs_,
        CreateTxStorageForTest(tx_storage_dir_.GetPath()));

    GetAccountUtils().CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);
    task_environment_.RunUntilIdle();

    account_id_ =
        GetAccountUtils().EnsureZecTestAccount(0)->account_id.Clone();

    auto account_info = keyring_service_->GetZCashAccountInfo(account_id_);
    ASSERT_TRUE(account_info);
    own_transparent_address_ =
        account_info->next_transparent_receive_address->address_string;
    ASSERT_TRUE(account_info->orchard_internal_address);
    own_orchard_internal_address_ = *account_info->orchard_internal_address;

    OrchardAddrRawPart external_orchard_raw{};
    external_orchard_raw.fill(0xAB);
    auto external_orchard_address =
        GetOrchardUnifiedAddress(external_orchard_raw, /*testnet=*/true);
    ASSERT_TRUE(external_orchard_address);
    external_orchard_address_ = *external_orchard_address;
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

  StrictMock<MockZCashWalletService>& zcash_wallet_service() {
    return *zcash_wallet_service_;
  }

  NiceMock<MockZCashRpc>& zcash_rpc() {
    return static_cast<NiceMock<MockZCashRpc>&>(
        zcash_wallet_service_->zcash_rpc());
  }

  // Controls the ZCashRpc::GetLatestBlock response so
  // ZCashTxManager::ContinueAddUnapprovedTransactionWithHeight computes
  // `ironwood_active` (via IsIronwoodActive) as desired for the test.
  void SetLatestBlockHeight(uint32_t height) {
    ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
        .WillByDefault([height](const std::string& chain_id,
                                ZCashRpc::GetLatestBlockCallback callback) {
          std::move(callback).Run(
              zcash::mojom::BlockID::New(height, std::vector<uint8_t>(32, 0)));
        });
  }

  mojom::NewZCashTransactionParamsPtr MakeParams(mojom::ZCashTokenType token_type,
                                                  const std::string& to) {
    return mojom::NewZCashTransactionParams::New(
        mojom::kZCashTestnet, account_id_.Clone(), to, kSendAmount,
        /*sending_max_amount=*/false, /*memo=*/std::nullopt,
        /*use_shielded_pool=*/false, token_type, /*swap_info=*/nullptr);
  }

  // Dispatches AddUnapprovedZCashTransaction and drains the task runner.
  // Used by routing tests that only care which Create*Transaction method was
  // invoked; the mocked Create*Transaction methods never run their own
  // callback, so the outer AddUnapprovedZCashTransactionCallback is never
  // invoked in these cases.
  void SendUnapprovedTransaction(mojom::ZCashTokenType token_type,
                                 const std::string& to) {
    base::MockCallback<mojom::TxService::AddUnapprovedZCashTransactionCallback>
        add_callback;
    tx_service_->AddUnapprovedZCashTransaction(MakeParams(token_type, to),
                                               add_callback.Get());
    task_environment_.RunUntilIdle();
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir tx_storage_dir_;
  base::ScopedTempDir orchard_sync_state_dir_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;

  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<StrictMock<MockZCashWalletService>> zcash_wallet_service_;
  std::unique_ptr<TxService> tx_service_;

  mojom::AccountIdPtr account_id_;
  std::string own_transparent_address_;
  std::string own_orchard_internal_address_;
  std::string external_orchard_address_;
};

TEST_F(ZCashTxManagerUnitTest,
       TransparentToTransparent_CreatesFullyTransparentTransaction) {
  SetLatestBlockHeight(kIronwoodInactiveHeight);

  EXPECT_CALL(zcash_wallet_service(),
              CreateFullyTransparentTransaction(_, _, _, _))
      .Times(1);

  SendUnapprovedTransaction(mojom::ZCashTokenType::kTransparent,
                            kExternalTransparentAddress);
}

TEST_F(ZCashTxManagerUnitTest,
       TransparentToOrchard_CreatesTransparentToOrchardTransaction) {
  SetLatestBlockHeight(kIronwoodInactiveHeight);

  EXPECT_CALL(zcash_wallet_service(),
              CreateTransparentToOrchardTransaction(_, _, _, _, _))
      .Times(1);

  SendUnapprovedTransaction(mojom::ZCashTokenType::kTransparent,
                            external_orchard_address_);
}

TEST_F(ZCashTxManagerUnitTest, Shielding_CreatesTransparentToOrchardTransaction) {
  SetLatestBlockHeight(kIronwoodInactiveHeight);

  EXPECT_CALL(zcash_wallet_service(),
              CreateTransparentToOrchardTransaction(_, _, _, _, _))
      .Times(1);

  // Sending from the transparent pool to the account's own Orchard internal
  // address classifies as kShielding rather than kTransparentToOrchard, but
  // both route to the same Create*Transaction call.
  SendUnapprovedTransaction(mojom::ZCashTokenType::kTransparent,
                            own_orchard_internal_address_);
}

TEST_F(ZCashTxManagerUnitTest,
       TransparentToIronwood_CreatesTransparentToIronwoodTransaction) {
  SetLatestBlockHeight(kIronwoodActiveHeight);

  EXPECT_CALL(zcash_wallet_service(),
              CreateTransparentToIronwoodTransaction(_, _, _, _, _))
      .Times(1);

  SendUnapprovedTransaction(mojom::ZCashTokenType::kTransparent,
                            external_orchard_address_);
}

TEST_F(ZCashTxManagerUnitTest,
       OrchardToOrchard_CreatesOrchardToOrchardTransaction) {
  SetLatestBlockHeight(kIronwoodInactiveHeight);

  EXPECT_CALL(zcash_wallet_service(),
              CreateOrchardToOrchardTransaction(_, _, _, _, _))
      .Times(1);

  SendUnapprovedTransaction(mojom::ZCashTokenType::kOrchard,
                            external_orchard_address_);
}

TEST_F(ZCashTxManagerUnitTest,
       OrchardToIronwood_CreatesOrchardToIronwoodTransaction) {
  SetLatestBlockHeight(kIronwoodActiveHeight);

  EXPECT_CALL(zcash_wallet_service(),
              CreateOrchardToIronwoodTransaction(_, _, _, _, _))
      .Times(1);

  SendUnapprovedTransaction(mojom::ZCashTokenType::kOrchard,
                            external_orchard_address_);
}

TEST_F(ZCashTxManagerUnitTest,
       IronwoodToIronwood_CreatesIronwoodToIronwoodTransaction) {
  SetLatestBlockHeight(kIronwoodActiveHeight);

  EXPECT_CALL(zcash_wallet_service(),
              CreateIronwoodToIronwoodTransaction(_, _, _, _, _))
      .Times(1);

  SendUnapprovedTransaction(mojom::ZCashTokenType::kIronwood,
                            external_orchard_address_);
}

TEST_F(ZCashTxManagerUnitTest,
       Unshielding_CreatesOrchardToTransparentTransaction) {
  SetLatestBlockHeight(kIronwoodInactiveHeight);

  EXPECT_CALL(zcash_wallet_service(),
              CreateOrchardToTransparentTransaction(_, _, _, _))
      .Times(1);

  // Sending from the Orchard pool to the account's own transparent receive
  // address classifies as kUnshielding rather than kOrchardToTransparent, but
  // both route to the same Create*Transaction call (while Ironwood is not
  // active).
  SendUnapprovedTransaction(mojom::ZCashTokenType::kOrchard,
                            own_transparent_address_);
}

TEST_F(ZCashTxManagerUnitTest,
       OrchardToTransparent_CreatesOrchardToTransparentTransaction) {
  SetLatestBlockHeight(kIronwoodInactiveHeight);

  EXPECT_CALL(zcash_wallet_service(),
              CreateOrchardToTransparentTransaction(_, _, _, _))
      .Times(1);

  SendUnapprovedTransaction(mojom::ZCashTokenType::kOrchard,
                            kExternalTransparentAddress);
}

TEST_F(ZCashTxManagerUnitTest,
       IronwoodToTransparent_CreatesIronwoodToTransparentTransaction) {
  SetLatestBlockHeight(kIronwoodActiveHeight);

  EXPECT_CALL(zcash_wallet_service(),
              CreateIronwoodToTransparentTransaction(_, _, _, _))
      .Times(1);

  SendUnapprovedTransaction(mojom::ZCashTokenType::kIronwood,
                            kExternalTransparentAddress);
}

// Once Ironwood is active the Orchard pool is retired: a would-be
// kUnshielding (Orchard -> own transparent address) send must fail with the
// dedicated error instead of falling through to
// CreateOrchardToTransparentTransaction.
TEST_F(ZCashTxManagerUnitTest,
       Unshielding_WhenIronwoodActive_ReturnsOrchardPoolRetiredError) {
  SetLatestBlockHeight(kIronwoodActiveHeight);

  base::test::TestFuture<bool, const std::string&, const std::string&>
      add_tx_future;
  tx_service_->AddUnapprovedZCashTransaction(
      MakeParams(mojom::ZCashTokenType::kOrchard, own_transparent_address_),
      add_tx_future.GetCallback());

  auto [success, tx_meta_id, error_message] = add_tx_future.Take();
  EXPECT_FALSE(success);
  EXPECT_TRUE(tx_meta_id.empty());
  EXPECT_EQ(error_message,
            "Orchard pool is retired after Ironwood activation");
}

// Same as above but for the kOrchardToTransparent (external address)
// classification.
TEST_F(ZCashTxManagerUnitTest,
       OrchardToTransparent_WhenIronwoodActive_ReturnsOrchardPoolRetiredError) {
  SetLatestBlockHeight(kIronwoodActiveHeight);

  base::test::TestFuture<bool, const std::string&, const std::string&>
      add_tx_future;
  tx_service_->AddUnapprovedZCashTransaction(
      MakeParams(mojom::ZCashTokenType::kOrchard, kExternalTransparentAddress),
      add_tx_future.GetCallback());

  auto [success, tx_meta_id, error_message] = add_tx_future.Take();
  EXPECT_FALSE(success);
  EXPECT_TRUE(tx_meta_id.empty());
  EXPECT_EQ(error_message,
            "Orchard pool is retired after Ironwood activation");
}

}  // namespace brave_wallet
