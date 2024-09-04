/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/keyring_service.h"

#include <optional>
#include <string_view>
#include <utility>

#include "base/base64.h"
#include "base/containers/span.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/ranges/algorithm.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_hd_keyring.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_test_utils.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/fil_transaction.h"
#include "brave/components/brave_wallet/browser/filecoin_keyring.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service_migrations.h"
#include "brave/components/brave_wallet/browser/keyring_service_prefs.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/switches.h"
#include "build/build_config.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

using base::test::ParseJsonDict;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::NiceMock;
using ::testing::Truly;

namespace brave_wallet {

namespace {

const char kPasswordBrave[] = "brave";
const char kPasswordBrave123[] = "brave123";

struct ImportData {
  const char* network;
  const char* name;
  const char* import_payload;
  const char* address;
  const char* private_key;
};

}  // namespace

class TestKeyringServiceObserver : public mojom::KeyringServiceObserver {
 public:
  TestKeyringServiceObserver(KeyringService& service,
                             base::test::TaskEnvironment& task_environment)
      : task_environment_(&task_environment) {
    service.AddObserver(observer_receiver_.BindNewPipeAndPassRemote());
  }
  ~TestKeyringServiceObserver() override = default;

  MOCK_METHOD(void, AutoLockMinutesChanged, (), (override));
  MOCK_METHOD(void, WalletCreated, (), (override));
  MOCK_METHOD(void, WalletRestored, (), (override));
  MOCK_METHOD(void, WalletReset, (), (override));
  MOCK_METHOD(void, Locked, (), (override));
  MOCK_METHOD(void, Unlocked, (), (override));
  MOCK_METHOD(void, BackedUp, (), (override));
  MOCK_METHOD(void,
              SelectedWalletAccountChanged,
              (mojom::AccountInfoPtr account),
              (override));
  MOCK_METHOD(void,
              SelectedDappAccountChanged,
              (mojom::CoinType coin, mojom::AccountInfoPtr account),
              (override));
  MOCK_METHOD(void, AccountsChanged, (), (override));
  MOCK_METHOD(void,
              AccountsAdded,
              (std::vector<mojom::AccountInfoPtr> accounts),
              (override));

  void WaitAndVerify() {
    task_environment_->RunUntilIdle();
    testing::Mock::VerifyAndClearExpectations(this);
  }

 private:
  mojo::Receiver<mojom::KeyringServiceObserver> observer_receiver_{this};
  raw_ptr<base::test::TaskEnvironment> task_environment_;
};

class KeyringServiceUnitTest : public testing::Test {
 public:
  KeyringServiceUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~KeyringServiceUnitTest() override = default;

 protected:
  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
    RegisterLocalStatePrefs(local_state_.registry());
    RegisterLocalStatePrefsForMigration(local_state_.registry());

    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    json_rpc_service_ = std::make_unique<JsonRpcService>(
        shared_url_loader_factory_, network_manager_.get(), &prefs_, nullptr);
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  }

  void SetInterceptor(const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, content](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          std::string header;
          request.headers.GetHeader("Authorization", &header);
          url_loader_factory_.AddResponse(request.url.spec(), content);
        }));
  }

  AccountUtils GetAccountUtils(KeyringService* service) {
    return AccountUtils(service);
  }

  PrefService* GetPrefs() { return &prefs_; }

  PrefService* GetLocalState() { return &local_state_; }

  JsonRpcService* json_rpc_service() { return json_rpc_service_.get(); }

  network::TestURLLoaderFactory& url_loader_factory() {
    return url_loader_factory_;
  }

  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory() {
    return shared_url_loader_factory_;
  }

  std::string GetStringPrefForKeyring(const std::string& key,
                                      mojom::KeyringId keyring_id) {
    const base::Value* value = GetPrefForKeyring(GetPrefs(), key, keyring_id);
    if (!value) {
      return std::string();
    }

    return value->GetString();
  }

  static std::optional<std::string> GetWalletMnemonic(
      const std::string& password,
      KeyringService* service) {
    base::RunLoop run_loop;
    std::optional<std::string> mnemonic;
    service->GetWalletMnemonic(
        password,
        base::BindLambdaForTesting([&](const std::optional<std::string>& v) {
          mnemonic = v;
          run_loop.Quit();
        }));
    run_loop.Run();
    return mnemonic;
  }

  static bool ValidatePassword(KeyringService* service,
                               const std::string& password) {
    base::RunLoop run_loop;
    bool validation_result = false;
    service->ValidatePassword(password,
                              base::BindLambdaForTesting([&](bool result) {
                                validation_result = result;
                                run_loop.Quit();
                              }));
    run_loop.Run();
    return validation_result;
  }

  static bool SetSelectedAccount(KeyringService* service,
                                 const mojom::AccountIdPtr& account_id) {
    bool success = false;
    base::RunLoop run_loop;
    service->SetSelectedAccount(account_id.Clone(),
                                base::BindLambdaForTesting([&](bool v) {
                                  success = v;
                                  run_loop.Quit();
                                }));
    run_loop.Run();
    return success;
  }

  static bool SetAccountName(KeyringService* service,
                             mojom::AccountIdPtr account_id,
                             const std::string& name) {
    bool success = false;
    base::RunLoop run_loop;
    service->SetAccountName(std::move(account_id), name,
                            base::BindLambdaForTesting([&](bool v) {
                              success = v;
                              run_loop.Quit();
                            }));
    run_loop.Run();
    return success;
  }

  static bool RemoveAccount(KeyringService* service,
                            const mojom::AccountIdPtr& account_id,
                            const std::string& password) {
    bool success;
    base::RunLoop run_loop;
    service->RemoveAccount(account_id.Clone(), password,
                           base::BindLambdaForTesting([&](bool v) {
                             success = v;
                             run_loop.Quit();
                           }));
    run_loop.Run();
    return success;
  }

  static std::optional<std::string> EncodePrivateKeyForExport(
      KeyringService* service,
      mojom::AccountIdPtr account_id,
      const std::string& password = kPasswordBrave) {
    std::optional<std::string> private_key;
    base::RunLoop run_loop;
    service->EncodePrivateKeyForExport(
        std::move(account_id), password,
        base::BindLambdaForTesting([&](const std::string& key) {
          if (!key.empty()) {
            private_key = key;
          }
          run_loop.Quit();
        }));
    run_loop.Run();
    return private_key;
  }

  static mojom::AccountInfoPtr ImportFilecoinAccount(
      KeyringService* service,
      const std::string& account_name,
      const std::string& private_key_hex,
      const std::string& network) {
    mojom::AccountInfoPtr result;
    base::RunLoop run_loop;
    service->ImportFilecoinAccount(
        account_name, private_key_hex, network,
        base::BindLambdaForTesting([&](mojom::AccountInfoPtr account) {
          result = std::move(account);
          run_loop.Quit();
        }));
    run_loop.Run();
    return result;
  }

  static mojom::AccountInfoPtr ImportAccount(KeyringService* service,
                                             const std::string& name,
                                             const std::string& private_key,
                                             mojom::CoinType coin) {
    mojom::AccountInfoPtr result;
    base::RunLoop run_loop;
    service->ImportAccount(
        name, private_key, coin,
        base::BindLambdaForTesting([&](mojom::AccountInfoPtr account) {
          result = std::move(account);
          run_loop.Quit();
        }));
    run_loop.Run();
    return result;
  }

  static mojom::AccountInfoPtr ImportAccountFromJson(
      KeyringService* service,
      const std::string& name,
      const std::string& password,
      const std::string& json) {
    mojom::AccountInfoPtr result;
    base::RunLoop run_loop;
    service->ImportAccountFromJson(
        name, password, json,
        base::BindLambdaForTesting([&](mojom::AccountInfoPtr account) {
          result = std::move(account);
          run_loop.Quit();
        }));
    run_loop.Run();
    return result;
  }

  static std::optional<std::string> CreateWallet(KeyringService* service,
                                                 const std::string& password) {
    std::string mnemonic;
    base::RunLoop run_loop;
    service->CreateWallet(
        password,
        base::BindLambdaForTesting([&](const std::optional<std::string>& v) {
          ASSERT_TRUE(v);
          mnemonic = *v;
          run_loop.Quit();
        }));
    run_loop.Run();
    return mnemonic;
  }

  static bool RestoreWallet(KeyringService* service,
                            const std::string& mnemonic,
                            const std::string& password,
                            bool is_legacy_brave_wallet) {
    return service->RestoreWalletSync(mnemonic, password,
                                      is_legacy_brave_wallet);
  }

  static mojom::AccountInfoPtr AddAccount(KeyringService* service,
                                          mojom::CoinType coin,
                                          mojom::KeyringId keyring_id,
                                          const std::string& name) {
    return service->AddAccountSync(coin, keyring_id, name);
  }

  static void ImportFilecoinAccounts(
      KeyringService* service,
      TestKeyringServiceObserver* observer,
      const std::vector<ImportData>& imported_accounts,
      mojom::KeyringId keyring_id) {
    EXPECT_CALL(*observer, WalletCreated()).Times(0);

    for (size_t i = 0; i < imported_accounts.size(); ++i) {
      auto account = ImportFilecoinAccount(service, imported_accounts[i].name,
                                           imported_accounts[i].import_payload,
                                           imported_accounts[i].network);
      ASSERT_TRUE(account);
      EXPECT_EQ(account->address, imported_accounts[i].address);

      auto payload = EncodePrivateKeyForExport(
          service, MakeAccountId(mojom::CoinType::FIL, keyring_id,
                                 mojom::AccountKind::kImported,
                                 imported_accounts[i].address));
      EXPECT_TRUE(payload);
      EXPECT_EQ(imported_accounts[i].import_payload, *payload);

      EXPECT_EQ(account, service->GetSelectedWalletAccount());
    }
    observer->WaitAndVerify();
  }

  static bool IsWalletBackedUp(KeyringService* service) {
    bool backed_up = false;
    base::RunLoop run_loop;
    service->IsWalletBackedUp(base::BindLambdaForTesting([&](bool v) {
      backed_up = v;
      run_loop.Quit();
    }));
    run_loop.Run();
    return backed_up;
  }

  static bool Unlock(KeyringService* service, const std::string& password) {
    bool success = false;
    base::RunLoop run_loop;
    service->Unlock(password, base::BindLambdaForTesting([&](bool v) {
                      success = v;
                      run_loop.Quit();
                    }));
    run_loop.Run();
    return success;
  }

  static int32_t GetAutoLockMinutes(KeyringService* service) {
    int32_t minutes;
    base::RunLoop run_loop;
    service->GetAutoLockMinutes(base::BindLambdaForTesting([&](int32_t v) {
      minutes = v;
      run_loop.Quit();
    }));
    run_loop.Run();
    return minutes;
  }

  static bool SetAutoLockMinutes(KeyringService* service, int32_t minutes) {
    bool success = false;
    base::RunLoop run_loop;
    service->SetAutoLockMinutes(minutes,
                                base::BindLambdaForTesting([&](bool v) {
                                  success = v;
                                  run_loop.Quit();
                                }));
    run_loop.Run();
    return success;
  }

  static bool IsStrongPassword(KeyringService* service,
                               const std::string& password) {
    bool result;
    base::RunLoop run_loop;
    service->IsStrongPassword(password, base::BindLambdaForTesting([&](bool v) {
                                result = v;
                                run_loop.Quit();
                              }));
    run_loop.Run();
    return result;
  }

  static std::string GetChecksumEthAddress(KeyringService* service,
                                           const std::string& address) {
    std::string checksum_address;
    base::RunLoop run_loop;
    service->GetChecksumEthAddress(
        address, base::BindLambdaForTesting([&](const std::string& v) {
          checksum_address = v;
          run_loop.Quit();
        }));
    run_loop.Run();
    return checksum_address;
  }

  bool SetNetwork(const std::string& chain_id, mojom::CoinType coin) {
    return json_rpc_service_->SetNetwork(chain_id, coin, std::nullopt);
  }

  static bool Lock(KeyringService* service) {
    service->Lock();
    return service->IsLockedSync();
  }

  mojom::AccountInfoPtr FirstSolAccount(KeyringService* service) {
    return base::ranges::find_if(service->GetAllAccountsSync()->accounts,
                                 [](auto& acc) {
                                   return acc->account_id->coin ==
                                          mojom::CoinType::SOL;
                                 })
        ->Clone();
  }

  content::BrowserTaskEnvironment task_environment_;

 private:
  base::test::ScopedFeatureList scoped_btc_ledger_feature_{
      features::kBraveWalletBitcoinLedgerFeature};

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  base::ScopedTempDir temp_dir_;
};  // namespace brave_wallet

TEST_F(KeyringServiceUnitTest, CreateWallet_DoubleCall) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  base::MockCallback<KeyringService::CreateWalletCallback> callback1;
  EXPECT_CALL(callback1,
              Run(Truly([](const std::optional<std::string>& mnemonic) {
                EXPECT_TRUE(mnemonic);
                return IsValidMnemonic(*mnemonic);
              })));
  service.CreateWallet(kTestWalletPassword, callback1.Get());

  base::MockCallback<KeyringService::CreateWalletCallback> callback2;
  // Does not DCHECK and fails with no mnemonic.
  EXPECT_CALL(callback2, Run(std::optional<std::string>()));
  service.CreateWallet(kTestWalletPassword, callback2.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(KeyringServiceUnitTest, SetPrefForKeyring) {
  SetPrefForKeyring(GetPrefs(), "pref1", base::Value("123"),
                    mojom::kDefaultKeyringId);
  const auto& keyrings_pref = GetPrefs()->GetDict(kBraveWalletKeyrings);
  const std::string* value =
      keyrings_pref.FindStringByDottedPath("default.pref1");
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(*value, "123");
  SetPrefForKeyring(GetPrefs(), "pref1", base::Value(),
                    mojom::kDefaultKeyringId);
  ASSERT_EQ(nullptr, GetPrefs()
                         ->GetDict(kBraveWalletKeyrings)
                         .FindStringByDottedPath("default.pref1"));

  EXPECT_EQ(keyrings_pref.FindByDottedPath("default.pref2"), nullptr);
  EXPECT_EQ(keyrings_pref.FindByDottedPath("keyring2.pref1"), nullptr);
}

TEST_F(KeyringServiceUnitTest, UnlockResumesDefaultKeyring) {
  std::string salt;
  base::Value::Dict mnemonic;
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    ASSERT_TRUE(CreateWallet(&service, "brave"));
    ASSERT_TRUE(AddAccount(&service, mojom::CoinType::ETH,
                           mojom::kDefaultKeyringId, "Account2"));

    salt = GetPrefs()->GetString(kBraveWalletEncryptorSalt);
    mnemonic = GetPrefs()->GetDict(kBraveWalletMnemonic).Clone();
  }
  {
    // KeyringService is now destructed, simulating relaunch
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    EXPECT_TRUE(Unlock(&service, "brave"));
    ASSERT_FALSE(service.IsLockedSync());

    EXPECT_EQ(GetPrefs()->GetString(kBraveWalletEncryptorSalt), salt);
    EXPECT_EQ(GetPrefs()->GetDict(kBraveWalletMnemonic), mnemonic);
    EXPECT_EQ(
        2u, service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId).size());
  }
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    // wrong password
    EXPECT_FALSE(Unlock(&service, "brave123"));
    ASSERT_TRUE(service.IsLockedSync());
    // empty password
    EXPECT_FALSE(Unlock(&service, ""));
    ASSERT_TRUE(service.IsLockedSync());
  }
}

TEST_F(KeyringServiceUnitTest, UnlockResumesNewKeyring) {
  std::string first_sol_account_address;
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    service.CreateWallet(kTestWalletPassword, base::DoNothing());
    auto all_sol_accounts = GetAccountUtils(&service).AllSolAccounts();
    EXPECT_EQ(1u, all_sol_accounts.size());
    first_sol_account_address = all_sol_accounts[0]->address;
  }

  {
    ScopedDictPrefUpdate keyrings_update(GetPrefs(), kBraveWalletKeyrings);
    // Remove whole Solana keyring.
    keyrings_update.Get().Remove("solana");
  }

  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    EXPECT_TRUE(Unlock(&service, kTestWalletPassword));

    // After restart  Solana looks like a 'new' coin with no accounts.
    EXPECT_EQ(0u, GetAccountUtils(&service).AllSolAccounts().size());

    EXPECT_TRUE(GetAccountUtils(&service).EnsureSolAccount(0));

    auto all_sol_accounts = GetAccountUtils(&service).AllSolAccounts();
    EXPECT_EQ(1u, all_sol_accounts.size());
    // Created account matches default created account which means same mnemonic
    // is used for Solana keyring.
    EXPECT_EQ(first_sol_account_address, all_sol_accounts[0]->address);
  }

  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    EXPECT_TRUE(Unlock(&service, kTestWalletPassword));

    // Still works after another restart.
    auto all_sol_accounts = GetAccountUtils(&service).AllSolAccounts();
    EXPECT_EQ(1u, all_sol_accounts.size());
    EXPECT_EQ(first_sol_account_address, all_sol_accounts[0]->address);
  }
}

TEST_F(KeyringServiceUnitTest, GetWalletMnemonic) {
  // Needed to skip unnecessary migration in CreateEncryptorForKeyring.
  GetPrefs()->SetBoolean(kBraveWalletKeyringEncryptionKeysMigrated, true);
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  // no pref exists yet
  EXPECT_EQ(GetWalletMnemonic(kPasswordBrave, &service), std::nullopt);

  AccountUtils(&service).CreateWallet(kMnemonicDivideCruise, kPasswordBrave);
  EXPECT_EQ(GetWalletMnemonic(kPasswordBrave, &service), kMnemonicDivideCruise);

  // Lock service
  service.Lock();
  ASSERT_TRUE(service.IsLockedSync());
  EXPECT_EQ(GetWalletMnemonic(kPasswordBrave, &service), std::nullopt);

  // unlock with wrong password
  ASSERT_FALSE(Unlock(&service, kPasswordBrave123));
  ASSERT_TRUE(service.IsLockedSync());

  EXPECT_EQ(GetWalletMnemonic(kPasswordBrave, &service), std::nullopt);

  ASSERT_TRUE(Unlock(&service, kPasswordBrave));
  ASSERT_FALSE(service.IsLockedSync());

  // Can only get mnemonic when password is correct.
  EXPECT_EQ(GetWalletMnemonic(kPasswordBrave123, &service), std::nullopt);
  EXPECT_EQ(GetWalletMnemonic(kPasswordBrave, &service), kMnemonicDivideCruise);
}

TEST_F(KeyringServiceUnitTest, ValidatePassword) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(CreateWallet(&service, "brave"));

  EXPECT_TRUE(ValidatePassword(&service, "brave"));
  EXPECT_FALSE(service.IsLockedSync());
  EXPECT_FALSE(ValidatePassword(&service, "brave123"));
  EXPECT_FALSE(service.IsLockedSync());

  service.Lock();
  EXPECT_TRUE(ValidatePassword(&service, "brave"));
}

TEST_F(KeyringServiceUnitTest, LockAndUnlock) {
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    // No encryptor but there is no keyring created so they should be unlocked.
    // And Lock() has no effect here.
    service.Lock();
    EXPECT_FALSE(service.IsLockedSync());
  }
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);
    AccountUtils(&service).CreateWallet(GenerateMnemonic(16), kPasswordBrave);
    ASSERT_TRUE(AddAccount(&service, mojom::CoinType::ETH,
                           mojom::kDefaultKeyringId, "ETH Account 1"));
    EXPECT_FALSE(service.IsLockedSync());
    ASSERT_TRUE(AddAccount(&service, mojom::CoinType::FIL,
                           mojom::kFilecoinKeyringId, "FIL Account 1"));
    ASSERT_TRUE(AddAccount(&service, mojom::CoinType::SOL,
                           mojom::kSolanaKeyringId, "SOL Account 1"));
    EXPECT_FALSE(service.IsLockedSync());

    EXPECT_CALL(observer, Locked());
    service.Lock();
    observer.WaitAndVerify();
    EXPECT_TRUE(service.IsLockedSync());
    EXPECT_FALSE(service.GetHDKeyringById(mojom::kDefaultKeyringId));
    EXPECT_FALSE(service.GetHDKeyringById(mojom::kFilecoinKeyringId));
    EXPECT_FALSE(service.GetHDKeyringById(mojom::kSolanaKeyringId));

    EXPECT_CALL(observer, Unlocked()).Times(0);
    EXPECT_FALSE(Unlock(&service, "abc"));
    observer.WaitAndVerify();
    EXPECT_TRUE(service.IsLockedSync());

    EXPECT_CALL(observer, Unlocked());
    EXPECT_TRUE(Unlock(&service, "brave"));
    observer.WaitAndVerify();
    EXPECT_FALSE(service.IsLockedSync());
  }
}

TEST_F(KeyringServiceUnitTest, Reset) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(CreateWallet(&service, "brave"));
  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

  ASSERT_TRUE(AddAccount(&service, mojom::CoinType::ETH,
                         mojom::kDefaultKeyringId, "Account 1"));
  // Trigger account number saving
  service.Lock();

  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletKeyrings));
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletEncryptorSalt));
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletMnemonic));
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletLegacyEthSeedFormat));

  EXPECT_CALL(observer, WalletReset());
  service.Reset();
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletKeyrings));
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletEncryptorSalt));
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletMnemonic));
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletLegacyEthSeedFormat));
  EXPECT_FALSE(service.GetHDKeyringById(mojom::kDefaultKeyringId));
  EXPECT_FALSE(service.encryptor_);
  observer.WaitAndVerify();
}

TEST_F(KeyringServiceUnitTest, BackupComplete) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  EXPECT_FALSE(IsWalletBackedUp(&service));

  service.NotifyWalletBackupComplete();

  EXPECT_TRUE(IsWalletBackedUp(&service));

  service.Reset();

  EXPECT_FALSE(IsWalletBackedUp(&service));
}

TEST_F(KeyringServiceUnitTest, AccountMetasForKeyring) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  ASSERT_TRUE(RestoreWallet(&service, kMnemonicDivideCruise, "brave", false));
  EXPECT_TRUE(AddAccount(&service, mojom::CoinType::ETH,
                         mojom::kDefaultKeyringId, "AccountETH"));
  EXPECT_TRUE(AddAccount(&service, mojom::CoinType::SOL,
                         mojom::kSolanaKeyringId, "AccountSOL"));
  EXPECT_TRUE(AddAccount(&service, mojom::CoinType::FIL,
                         mojom::kFilecoinKeyringId, "AccountFIL"));
  EXPECT_TRUE(AddAccount(&service, mojom::CoinType::FIL,
                         mojom::kFilecoinTestnetKeyringId, "AccountFILTest"));

  EXPECT_EQ(
      *GetPrefForKeyring(GetPrefs(), kAccountMetas, mojom::kDefaultKeyringId),
      base::test::ParseJson(R"(
  [
    {
        "account_index" : "0",
        "account_address": "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db",
        "account_name": "Account 1"
    },
    {
        "account_index" : "1",
        "account_address": "0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0",
        "account_name": "AccountETH"
    }
  ]
  )"));

  EXPECT_EQ(
      *GetPrefForKeyring(GetPrefs(), kAccountMetas, mojom::kSolanaKeyringId),
      base::test::ParseJson(R"(
  [
    {
        "account_index" : "0",
        "account_address": "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
        "account_name": "Solana Account 1"
    },
    {
        "account_index" : "1",
        "account_address": "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV",
        "account_name": "AccountSOL"
    }
  ]
  )"));

  EXPECT_EQ(
      *GetPrefForKeyring(GetPrefs(), kAccountMetas, mojom::kFilecoinKeyringId),
      base::test::ParseJson(R"(
  [
    {
        "account_index" : "0",
        "account_address": "f1qjidlytseoouzfhsgzczf3ettbhuaezorczeava",
        "account_name": "AccountFIL"
    }
  ]
  )"));

  EXPECT_EQ(*GetPrefForKeyring(GetPrefs(), kAccountMetas,
                               mojom::kFilecoinTestnetKeyringId),
            base::test::ParseJson(R"(
  [
    {
      "account_index" : "0",
      "account_address": "t1dca7adhz5lbvin5n3qlw67munu6xhn5fpb77nly",
      "account_name": "AccountFILTest"
    }
  ]
  )"));
}

TEST_F(KeyringServiceUnitTest, MigrateDerivedAccountIndex) {
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    ASSERT_TRUE(RestoreWallet(&service, kMnemonicDivideCruise, "brave", false));
  }

  SetPrefForKeyring(GetPrefs(), kAccountMetas, base::test::ParseJson(R"(
  {
    "m/44'/60'/0'/0/0": {
        "account_address": "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db",
        "account_name": "Account 1"
    },
    "m/44'/60'/0'/0/1": {
        "account_address": "0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0",
        "account_name": "AccountETH"
    }
  })"),
                    mojom::kDefaultKeyringId);

  SetPrefForKeyring(GetPrefs(), kAccountMetas, base::test::ParseJson(R"(
  {
    "m/44'/501'/0'/0'": {
        "account_address": "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
        "account_name": "Solana Account 1"
    },
    "m/44'/501'/1'/0'": {
        "account_address": "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV",
        "account_name": "AccountSOL"
    }
  })"),
                    mojom::kSolanaKeyringId);

  SetPrefForKeyring(GetPrefs(), kAccountMetas, base::test::ParseJson(R"(
  {
    "m/44'/461'/0'/0/0": {
        "account_address": "f1qjidlytseoouzfhsgzczf3ettbhuaezorczeava",
        "account_name": "AccountFIL"
    }
  })"),
                    mojom::kFilecoinKeyringId);

  SetPrefForKeyring(GetPrefs(), kAccountMetas, base::test::ParseJson(R"(
  {
    "m/44'/1'/0'/0/0": {
      "account_address": "t1dca7adhz5lbvin5n3qlw67munu6xhn5fpb77nly",
      "account_name": "AccountFILTest"
    }
  })"),
                    mojom::kFilecoinTestnetKeyringId);

  MigrateDerivedAccountIndex(GetPrefs());

  EXPECT_EQ(
      *GetPrefForKeyring(GetPrefs(), kAccountMetas, mojom::kDefaultKeyringId),
      base::test::ParseJson(R"(
  [
    {
        "account_index" : "0",
        "account_address": "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db",
        "account_name": "Account 1"
    },
    {
        "account_index" : "1",
        "account_address": "0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0",
        "account_name": "AccountETH"
    }
  ]
  )"));

  EXPECT_EQ(
      *GetPrefForKeyring(GetPrefs(), kAccountMetas, mojom::kSolanaKeyringId),
      base::test::ParseJson(R"(
  [
    {
        "account_index" : "0",
        "account_address": "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
        "account_name": "Solana Account 1"
    },
    {
        "account_index" : "1",
        "account_address": "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV",
        "account_name": "AccountSOL"
    }
  ]
  )"));

  EXPECT_EQ(
      *GetPrefForKeyring(GetPrefs(), kAccountMetas, mojom::kFilecoinKeyringId),
      base::test::ParseJson(R"(
  [
    {
        "account_index" : "0",
        "account_address": "f1qjidlytseoouzfhsgzczf3ettbhuaezorczeava",
        "account_name": "AccountFIL"
    }
  ]
  )"));

  EXPECT_EQ(*GetPrefForKeyring(GetPrefs(), kAccountMetas,
                               mojom::kFilecoinTestnetKeyringId),
            base::test::ParseJson(R"(
  [
    {
      "account_index" : "0",
      "account_address": "t1dca7adhz5lbvin5n3qlw67munu6xhn5fpb77nly",
      "account_name": "AccountFILTest"
    }
  ]
  )"));

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  const auto& accounts = service.GetAllAccountInfos();
  EXPECT_EQ(accounts.size(), 6u);
  EXPECT_EQ(accounts[0]->address, "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db");
  EXPECT_EQ(accounts[1]->address, "0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0");
  EXPECT_EQ(accounts[2]->address, "f1qjidlytseoouzfhsgzczf3ettbhuaezorczeava");
  EXPECT_EQ(accounts[3]->address, "t1dca7adhz5lbvin5n3qlw67munu6xhn5fpb77nly");
  EXPECT_EQ(accounts[4]->address,
            "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8");
  EXPECT_EQ(accounts[5]->address,
            "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV");
}

TEST_F(KeyringServiceUnitTest, CreateAndRestoreWallet) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

  EXPECT_CALL(observer, WalletRestored()).Times(0);
  EXPECT_CALL(observer, WalletCreated());
  std::optional<std::string> mnemonic_to_be_restored =
      CreateWallet(&service, "brave");
  ASSERT_TRUE(mnemonic_to_be_restored.has_value());
  observer.WaitAndVerify();

  std::vector<mojom::AccountInfoPtr> account_infos =
      service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
  std::vector<mojom::AccountInfoPtr> solana_account_infos =
      service.GetAccountInfosForKeyring(mojom::kSolanaKeyringId);
  EXPECT_EQ(account_infos.size(), 1u);
  EXPECT_EQ(account_infos[0], service.GetSelectedEthereumDappAccount());
  EXPECT_EQ(solana_account_infos.size(), 1u);
  EXPECT_EQ(solana_account_infos[0], service.GetSelectedWalletAccount());
  EXPECT_EQ(solana_account_infos[0], service.GetSelectedSolanaDappAccount());

  EXPECT_FALSE(account_infos[0]->address.empty());
  const std::string address0 = account_infos[0]->address;
  EXPECT_EQ(account_infos[0]->name, "Account 1");

  service.Reset();

  auto verify_restore_wallet = base::BindLambdaForTesting(
      [&mnemonic_to_be_restored, &service, &address0]() {
        EXPECT_TRUE(
            RestoreWallet(&service, *mnemonic_to_be_restored, "brave1", false));
        {
          std::vector<mojom::AccountInfoPtr> account_infos =
              service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
          EXPECT_EQ(account_infos.size(), 1u);
          EXPECT_EQ(account_infos[0], service.GetSelectedEthereumDappAccount());
          EXPECT_EQ(account_infos[0]->address, address0);
          EXPECT_EQ(account_infos[0]->name, "Account 1");
        }

        {
          std::vector<mojom::AccountInfoPtr> account_infos =
              service.GetAccountInfosForKeyring(
                  mojom::kFilecoinTestnetKeyringId);
          EXPECT_EQ(account_infos.size(), 0u);
        }

        {
          std::vector<mojom::AccountInfoPtr> account_infos =
              service.GetAccountInfosForKeyring(mojom::kFilecoinKeyringId);
          EXPECT_EQ(account_infos.size(), 0u);
        }

        {
          std::vector<mojom::AccountInfoPtr> account_infos =
              service.GetAccountInfosForKeyring(mojom::kSolanaKeyringId);
          EXPECT_EQ(account_infos.size(), 1u);
          EXPECT_EQ(account_infos[0], service.GetSelectedWalletAccount());
          EXPECT_EQ(account_infos[0], service.GetSelectedSolanaDappAccount());
        }
      });
  observer.WaitAndVerify();

  EXPECT_CALL(observer, WalletRestored());
  EXPECT_CALL(observer, WalletCreated()).Times(0);
  verify_restore_wallet.Run();
  observer.WaitAndVerify();

  // Restore twice consecutively should succeed and be just an unlock with only
  // one account
  EXPECT_CALL(observer, WalletCreated()).Times(0);
  EXPECT_CALL(observer, WalletRestored()).Times(0);
  EXPECT_CALL(observer, Unlocked());
  verify_restore_wallet.Run();
  observer.WaitAndVerify();
}

TEST_F(KeyringServiceUnitTest, DefaultSolanaAccountCreated) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_NE(CreateWallet(&service, "brave"), std::nullopt);

  std::vector<mojom::AccountInfoPtr> account_infos =
      service.GetAccountInfosForKeyring(mojom::kSolanaKeyringId);
  EXPECT_EQ(account_infos.size(), 1u);
  EXPECT_FALSE(account_infos[0]->address.empty());
  EXPECT_EQ(account_infos[0]->name, "Solana Account 1");
}

TEST_F(KeyringServiceUnitTest, DefaultSolanaAccountRestored) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(RestoreWallet(&service, kMnemonicDivideCruise, "brave", false));

  std::vector<mojom::AccountInfoPtr> account_infos =
      service.GetAccountInfosForKeyring(mojom::kSolanaKeyringId);
  EXPECT_EQ(account_infos.size(), 1u);
  EXPECT_FALSE(account_infos[0]->address.empty());
  EXPECT_EQ(account_infos[0]->name, "Solana Account 1");
}

TEST_F(KeyringServiceUnitTest, AddAccount) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_NE(CreateWallet(&service, "brave"), std::nullopt);
  EXPECT_TRUE(AddAccount(&service, mojom::CoinType::ETH,
                         mojom::kDefaultKeyringId, "Account5566"));

  std::vector<mojom::AccountInfoPtr> account_infos =
      service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
  EXPECT_EQ(account_infos.size(), 2u);
  EXPECT_FALSE(account_infos[0]->address.empty());
  EXPECT_EQ(account_infos[0]->name, "Account 1");
  EXPECT_FALSE(account_infos[1]->address.empty());
  EXPECT_EQ(account_infos[1]->name, "Account5566");
}

TEST_F(KeyringServiceUnitTest, ImportedAccounts) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  ASSERT_TRUE(CreateWallet(&service, "brave"));
  for (const std::string invalid_private_key :
       {"0x", "0x0", "0", "0x123abc", "123abc", "", "invalid"}) {
    EXPECT_FALSE(ImportAccount(&service, "invalid account", invalid_private_key,
                               mojom::CoinType::ETH));
  }

  const struct {
    const char* name;
    const char* private_key;
    const char* address;
    const char* encoded_private_key;
  } imported_accounts[] = {
      {
          "Imported account1",
          "d118a12a1e3b595d7d9e5599370df4ddc58d246a3ae4a795597e50eb6a32afb5",
          "0xDc06aE500aD5ebc5972A0D8Ada4733006E905976",
          "d118a12a1e3b595d7d9e5599370df4ddc58d246a3ae4a795597e50eb6a32afb5",
      },
      {
          "Imported account2",
          "cca1e9643efc5468789366e4fb682dba57f2e97540981095bc6d9a962309d912",
          "0x6D59205FADC892333cb945AD563e74F83f3dBA95",
          "cca1e9643efc5468789366e4fb682dba57f2e97540981095bc6d9a962309d912",
      },
      {
          "Imported account3",
          "0xddc33eef7cc4c5170c3ba4021cc22fd888856cf8bf846f48db6d11d15efcd652",
          "0xeffF78040EdeF86A9be71ce89c74A35C4cd5D2eA",
          "ddc33eef7cc4c5170c3ba4021cc22fd888856cf8bf846f48db6d11d15efcd652",
      }};
  for (const auto& account : imported_accounts) {
    auto imported_account = ImportAccount(
        &service, account.name, account.private_key, mojom::CoinType::ETH);
    ASSERT_TRUE(imported_account);
    EXPECT_EQ(account.address, imported_account->address);

    auto private_key = EncodePrivateKeyForExport(
        &service, MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                                mojom::AccountKind::kDerived, account.address));
    EXPECT_TRUE(private_key);
    EXPECT_EQ(account.encoded_private_key, private_key);
  }
  task_environment_.RunUntilIdle();

  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

  EXPECT_CALL(observer, AccountsChanged()).Times(0);
  EXPECT_FALSE(RemoveAccount(
      &service,
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kImported, ""),
      kPasswordBrave));
  observer.WaitAndVerify();

  EXPECT_CALL(observer, AccountsChanged()).Times(0);
  EXPECT_FALSE(RemoveAccount(
      &service,
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kImported,
                    imported_accounts[1].address),
      kPasswordBrave123));
  observer.WaitAndVerify();

  EXPECT_CALL(observer, AccountsChanged());
  EXPECT_TRUE(RemoveAccount(
      &service,
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kImported,
                    imported_accounts[1].address),
      kPasswordBrave));
  observer.WaitAndVerify();

  // remove invalid address
  EXPECT_CALL(observer, AccountsChanged()).Times(0);
  EXPECT_FALSE(RemoveAccount(
      &service,
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kImported, "0xxxxxxxxxx0"),
      kPasswordBrave));

  auto account_infos = GetAccountUtils(&service).AllEthAccounts();
  EXPECT_EQ(account_infos.size(), 3u);
  EXPECT_FALSE(account_infos[0]->address.empty());
  EXPECT_EQ(account_infos[0]->name, "Account 1");
  EXPECT_EQ(account_infos[0]->account_id->kind, mojom::AccountKind::kDerived);
  // imported accounts
  EXPECT_EQ(account_infos[1]->address, imported_accounts[0].address);
  EXPECT_EQ(account_infos[1]->name, imported_accounts[0].name);
  EXPECT_EQ(account_infos[1]->account_id->kind, mojom::AccountKind::kImported);
  EXPECT_EQ(account_infos[2]->address, imported_accounts[2].address);
  EXPECT_EQ(account_infos[2]->name, imported_accounts[2].name);
  EXPECT_EQ(account_infos[2]->account_id->kind, mojom::AccountKind::kImported);

  service.Lock();
  // cannot get private key when locked
  auto private_key = EncodePrivateKeyForExport(
      &service, MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                              mojom::AccountKind::kImported,
                              imported_accounts[0].address));
  EXPECT_FALSE(private_key);

  EXPECT_TRUE(Unlock(&service, "brave"));

  account_infos = GetAccountUtils(&service).AllEthAccounts();
  // Imported accounts should be restored
  EXPECT_EQ(account_infos.size(), 3u);
  EXPECT_EQ(account_infos[1]->address, imported_accounts[0].address);
  EXPECT_EQ(account_infos[1]->name, imported_accounts[0].name);
  EXPECT_EQ(account_infos[1]->account_id->kind, mojom::AccountKind::kImported);
  EXPECT_EQ(account_infos[2]->address, imported_accounts[2].address);
  EXPECT_EQ(account_infos[2]->name, imported_accounts[2].name);
  EXPECT_EQ(account_infos[2]->account_id->kind, mojom::AccountKind::kImported);

  // Unlocked but with wrong password won't get private key.
  EXPECT_FALSE(EncodePrivateKeyForExport(
      &service,
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kImported,
                    imported_accounts[0].address),
      kPasswordBrave123));

  // private key should also be available now
  private_key = EncodePrivateKeyForExport(
      &service, MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                              mojom::AccountKind::kImported,
                              imported_accounts[0].address));
  EXPECT_TRUE(private_key);
  EXPECT_EQ(imported_accounts[0].private_key, *private_key);

  // Imported accounts should also be restored in default keyring
  auto* default_keyring = service.GetHDKeyringById(mojom::kDefaultKeyringId);
  EXPECT_EQ(default_keyring->GetImportedAccountsForTesting().size(), 2u);

  const base::Value* imported_accounts_value = GetPrefForKeyring(
      GetPrefs(), kImportedAccounts, mojom::kDefaultKeyringId);
  ASSERT_TRUE(imported_accounts_value);
  EXPECT_EQ(*imported_accounts_value->GetList()[0].GetDict().FindString(
                kAccountAddress),
            imported_accounts[0].address);
  // private key is encrypted
  const base::Value::Dict encrypted_private_key =
      imported_accounts_value->GetList()[0]
          .GetDict()
          .FindDict("encrypted_private_key")
          ->Clone();

  std::vector<uint8_t> private_key0;
  ASSERT_TRUE(
      base::HexStringToBytes(imported_accounts[0].private_key, &private_key0));
  ASSERT_TRUE(encrypted_private_key.FindString("ciphertext"));
  EXPECT_NE(*encrypted_private_key.FindString("ciphertext"),
            base::Base64Encode(private_key0));
}

TEST_F(KeyringServiceUnitTest, ImportedAccountFromJson) {
  const std::string json(
      R"({
          "address":"b14ab53e38da1c172f877dbc6d65e4a1b0474c3c",
          "crypto" : {
              "cipher" : "aes-128-ctr",
              "cipherparams" : {
                  "iv" : "cecacd85e9cb89788b5aab2f93361233"
              },
              "ciphertext" : "c52682025b1e5d5c06b816791921dbf439afe7a053abb9fac19f38a57499652c",
              "kdf" : "scrypt",
              "kdfparams" : {
                  "dklen" : 32,
                  "n" : 262144,
                  "p" : 1,
                  "r" : 8,
                  "salt" : "dc9e4a98886738bd8aae134a1f89aaa5a502c3fbd10e336136d4d5fe47448ad6"
              },
              "mac" : "27b98c8676dc6619d077453b38db645a4c7c17a3e686ee5adaf53c11ac1b890e"
          },
          "id" : "7e59dc02-8d42-409d-b29a-a8a0f862cc81",
          "version" : 3
      })");
  const std::string expected_private_key =
      "efca4cdd31923b50f4214af5d2ae10e7ac45a5019e9431cc195482d707485378";
  const std::string expected_address =
      "0xB14Ab53E38DA1C172f877DBC6d65e4a1B0474C3c";

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(CreateWallet(&service, "brave"));

  EXPECT_FALSE(
      ImportAccountFromJson(&service, "Imported 1", "wrong password", json));

  EXPECT_FALSE(ImportAccountFromJson(&service, "Imported 1", "testtest",
                                     "{crypto: 123}"));

  auto account =
      ImportAccountFromJson(&service, "Imported 1", "testtest", json);
  ASSERT_TRUE(account);
  EXPECT_EQ(account->address, expected_address);

  service.Lock();
  EXPECT_TRUE(Unlock(&service, "brave"));

  // check restore by getting private key
  auto private_key = EncodePrivateKeyForExport(
      &service, MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                              mojom::AccountKind::kImported, expected_address));
  EXPECT_TRUE(private_key);
  EXPECT_EQ(expected_private_key, *private_key);

  // private key is encrypted
  const base::Value* imported_accounts_value = GetPrefForKeyring(
      GetPrefs(), kImportedAccounts, mojom::kDefaultKeyringId);
  ASSERT_TRUE(imported_accounts_value);
  const base::Value::Dict encrypted_private_key =
      imported_accounts_value->GetList()[0]
          .GetDict()
          .FindDict("encrypted_private_key")
          ->Clone();

  std::vector<uint8_t> private_key_bytes;
  ASSERT_TRUE(base::HexStringToBytes(expected_private_key, &private_key_bytes));
  ASSERT_TRUE(encrypted_private_key.FindString("ciphertext"));
  EXPECT_NE(*encrypted_private_key.FindString("ciphertext"),
            base::Base64Encode(private_key_bytes));
}

TEST_F(KeyringServiceUnitTest, EncodePrivateKeyForExport) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(RestoreWallet(&service, kMnemonicDivideCruise, "brave", false));

  // Can't get private key with wrong password.
  EXPECT_FALSE(EncodePrivateKeyForExport(
      &service,
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kDerived,
                    "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db"),
      kPasswordBrave123));

  std::optional<std::string> private_key = EncodePrivateKeyForExport(
      &service, MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                              mojom::AccountKind::kDerived,
                              "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db"));
  ASSERT_TRUE(private_key.has_value());
  EXPECT_EQ(*private_key,
            "919af8081ce2a02d9650bf3e10ffb6b7cbadbb1dca749122d7d982cdb6cbcc50");

  // account not added yet
  EXPECT_FALSE(EncodePrivateKeyForExport(
      &service, MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                              mojom::AccountKind::kDerived,
                              "0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0")));
  ASSERT_TRUE(AddAccount(&service, mojom::CoinType::ETH,
                         mojom::kDefaultKeyringId, "Account 2"));

  private_key = EncodePrivateKeyForExport(
      &service, MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                              mojom::AccountKind::kDerived,
                              "0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0"));
  ASSERT_TRUE(private_key.has_value());
  EXPECT_EQ(*private_key,
            "17c31fdade7d84f22462f398df300405a76fc11b1fe5a9e286dc8c3b0913e31c");

  EXPECT_FALSE(EncodePrivateKeyForExport(
      &service, MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                              mojom::AccountKind::kDerived, "")));
  EXPECT_FALSE(EncodePrivateKeyForExport(
      &service, MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                              mojom::AccountKind::kDerived, "0x123")));

  // Other keyrings
  // Wrong password.
  EXPECT_FALSE(EncodePrivateKeyForExport(
      &service,
      MakeAccountId(mojom::CoinType::SOL, mojom::kSolanaKeyringId,
                    mojom::AccountKind::kDerived,
                    "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8"),
      kPasswordBrave123));
  private_key = EncodePrivateKeyForExport(
      &service, MakeAccountId(mojom::CoinType::SOL, mojom::kSolanaKeyringId,
                              mojom::AccountKind::kDerived,
                              "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8"));
  ASSERT_TRUE(private_key.has_value());
  EXPECT_EQ(*private_key,
            "LNWjgQq8NhxWTUhz9jAD7koZfsKDwdJuLmVHyMxfjaFAamqXbtyUd3TcYQV2vPeRoM"
            "58gw7Ez8qsvKSZee6KdUQ");
}

TEST_F(KeyringServiceUnitTest, SetDefaultKeyringDerivedAccountMeta) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

  const std::string updated_name = "Updated";

  EXPECT_CALL(observer, AccountsChanged()).Times(0);
  // no keyring yet
  EXPECT_FALSE(SetAccountName(
      &service,
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kDerived,
                    "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db"),
      updated_name));
  observer.WaitAndVerify();

  EXPECT_CALL(observer, AccountsChanged());
  ASSERT_TRUE(RestoreWallet(&service, kMnemonicDivideCruise, "brave", false));
  AddAccount(&service, mojom::CoinType::ETH, mojom::kDefaultKeyringId,
             "New Account");
  observer.WaitAndVerify();

  auto account_infos =
      service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
  EXPECT_EQ(2u, account_infos.size());
  EXPECT_EQ("Account 1", account_infos[0]->name);
  EXPECT_EQ("New Account", account_infos[1]->name);
  const std::string address2 = account_infos[1]->address;

  // empty address
  EXPECT_CALL(observer, AccountsChanged()).Times(0);
  EXPECT_FALSE(SetAccountName(
      &service,
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kDerived, ""),
      updated_name));
  observer.WaitAndVerify();

  // empty name
  EXPECT_CALL(observer, AccountsChanged()).Times(0);
  EXPECT_FALSE(SetAccountName(
      &service,
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kDerived, address2),
      ""));
  observer.WaitAndVerify();

  EXPECT_CALL(observer, AccountsChanged());
  EXPECT_TRUE(SetAccountName(
      &service,
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kDerived, address2),
      updated_name));
  observer.WaitAndVerify();

  account_infos = service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
  EXPECT_EQ(2u, account_infos.size());
  EXPECT_EQ("Account 1", account_infos[0]->name);
  EXPECT_EQ(updated_name, account_infos[1]->name);
}

TEST_F(KeyringServiceUnitTest, SetDefaultKeyringImportedAccountName) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

  ASSERT_TRUE(CreateWallet(&service, kPasswordBrave));

  const struct {
    const char* name;
    const char* private_key;
    const char* address;
  } imported_accounts[] = {
      {"Imported account1",
       "d118a12a1e3b595d7d9e5599370df4ddc58d246a3ae4a795597e50eb6a32afb5",
       "0xDc06aE500aD5ebc5972A0D8Ada4733006E905976"},
      {"Imported account2",
       "cca1e9643efc5468789366e4fb682dba57f2e97540981095bc6d9a962309d912",
       "0x6D59205FADC892333cb945AD563e74F83f3dBA95"},
      {"Imported account3",
       "ddc33eef7cc4c5170c3ba4021cc22fd888856cf8bf846f48db6d11d15efcd652",
       "0xeffF78040EdeF86A9be71ce89c74A35C4cd5D2eA"}};

  const std::string kUpdatedName = "Updated imported accoount 2";

  // Fail when no imported accounts.
  EXPECT_FALSE(SetAccountName(
      &service,
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kImported,
                    imported_accounts[1].address),
      kUpdatedName));

  // Add import accounts.
  for (const auto& account : imported_accounts) {
    EXPECT_CALL(observer, AccountsChanged());
    auto imported_account = ImportAccount(
        &service, account.name, account.private_key, mojom::CoinType::ETH);
    ASSERT_TRUE(imported_account);
    EXPECT_EQ(account.address, imported_account->address);
    observer.WaitAndVerify();
  }

  // Empty address should fail.
  EXPECT_FALSE(SetAccountName(
      &service,
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kImported, ""),
      kUpdatedName));

  // Empty name should fail.
  EXPECT_FALSE(SetAccountName(
      &service,
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kImported,
                    imported_accounts[1].address),
      ""));

  // Update second imported account's name.
  EXPECT_TRUE(SetAccountName(
      &service,
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kImported,
                    imported_accounts[1].address),
      kUpdatedName));

  // Private key of imported accounts should not be changed.
  for (const auto& imported_account : imported_accounts) {
    auto private_key = EncodePrivateKeyForExport(
        &service,
        MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                      mojom::AccountKind::kImported, imported_account.address));
    EXPECT_TRUE(private_key);
    EXPECT_EQ(imported_account.private_key, *private_key);
  }

  auto account_infos = GetAccountUtils(&service).AllEthAccounts();

  // Only second imported account's name is updated.
  EXPECT_EQ(account_infos.size(), 4u);
  EXPECT_FALSE(account_infos[0]->address.empty());
  EXPECT_EQ(account_infos[0]->name, "Account 1");
  EXPECT_EQ(account_infos[0]->account_id->kind, mojom::AccountKind::kDerived);
  EXPECT_EQ(account_infos[1]->address, imported_accounts[0].address);
  EXPECT_EQ(account_infos[1]->name, imported_accounts[0].name);
  EXPECT_EQ(account_infos[1]->account_id->kind, mojom::AccountKind::kImported);
  EXPECT_EQ(account_infos[2]->address, imported_accounts[1].address);
  EXPECT_EQ(account_infos[2]->name, kUpdatedName);
  EXPECT_EQ(account_infos[2]->account_id->kind, mojom::AccountKind::kImported);
  EXPECT_EQ(account_infos[3]->address, imported_accounts[2].address);
  EXPECT_EQ(account_infos[3]->name, imported_accounts[2].name);
  EXPECT_EQ(account_infos[3]->account_id->kind, mojom::AccountKind::kImported);
}

TEST_F(KeyringServiceUnitTest, RestoreLegacyBraveWallet) {
  const char* mnemonic24 =
      "cushion pitch impact album daring marine much annual budget social "
      "clarify balance rose almost area busy among bring hidden bind later "
      "capable pulp laundry";
  const char* mnemonic12 = kMnemonicDripCaution;
  BraveWalletService brave_wallet_service(
      shared_url_loader_factory(), TestBraveWalletServiceDelegate::Create(),
      GetPrefs(), GetLocalState());
  KeyringService& service = *brave_wallet_service.keyring_service();
  auto verify_restore_wallet = base::BindLambdaForTesting(
      [&service](const char* mnemonic, const char* address, bool is_legacy,
                 bool expect_result) {
        service.Reset();
        EXPECT_EQ(expect_result,
                  RestoreWallet(&service, mnemonic, "brave1", is_legacy));
        if (expect_result) {
          std::vector<mojom::AccountInfoPtr> account_infos =
              service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
          ASSERT_EQ(account_infos.size(), 1u);
          EXPECT_EQ(account_infos[0]->address, address);
          EXPECT_EQ(account_infos[0]->name, "Account 1");

          // Test lock & unlock to check if it read the right
          // legacy_brave_wallet pref so it will use the right seed
          service.Lock();
          EXPECT_TRUE(Unlock(&service, "brave1"));
          account_infos.clear();
          account_infos =
              service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
          ASSERT_EQ(account_infos.size(), 1u);
          EXPECT_EQ(account_infos[0]->address, address);
        }
      });
  verify_restore_wallet.Run(
      mnemonic24, "0xea3C17c81E3baC3472d163b2c8b12ddDAa027874", true, true);
  verify_restore_wallet.Run(
      mnemonic24, "0xe026eBd81C1A64807F9Cbf21d89a67211eF48717", false, true);
  // brave legacy mnemonic can only be 24 words
  verify_restore_wallet.Run(mnemonic12, "", true, false);
  verify_restore_wallet.Run(
      mnemonic12, "0x084DCb94038af1715963F149079cE011C4B22961", false, true);
}

TEST_F(KeyringServiceUnitTest, HardwareAccounts) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  SetNetwork(mojom::kFilecoinMainnet, mojom::CoinType::FIL);

  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

  // Wallet is unlocked when there is no accounts of any types.
  EXPECT_FALSE(service.IsLockedSync());

  // TODO(apaymyshev): make this follow ui behavior when all accounts in batch
  // have same coin/keyring.

  // We don't need to create wallet to use hardwareaccounts
  std::vector<mojom::HardwareWalletAccountPtr> new_accounts;
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0x111", "m/44'/60'/1'/0/0", "name 1", mojom::HardwareVendor::kLedger,
      "device1", mojom::kDefaultKeyringId));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0x264", "m/44'/461'/0'/0/0", "name 2", mojom::HardwareVendor::kLedger,
      "device1", mojom::kFilecoinKeyringId));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0xEA0", "m/44'/60'/2'/0/0", "name 3", mojom::HardwareVendor::kLedger,
      "device2", mojom::kDefaultKeyringId));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0xFIL", "m/44'/461'/2'/0/0", "filecoin 1",
      mojom::HardwareVendor::kLedger, "device2", mojom::kFilecoinKeyringId));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0x222", "m/44'/60'/3'/0/0", "name 4", mojom::HardwareVendor::kLedger,
      "device1", mojom::kDefaultKeyringId));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0xFILTEST", "m/44'/1'/2'/0/0", "filecoin testnet 1",
      mojom::HardwareVendor::kLedger, "device2",
      mojom::kFilecoinTestnetKeyringId));
  const auto new_accounts_func = [&] {
    std::vector<mojom::HardwareWalletAccountPtr> result;
    for (auto& a : new_accounts) {
      result.push_back(a->Clone());
    }
    return result;
  };

  std::vector<mojom::HardwareWalletAccountPtr> accounts;
  base::ranges::transform(new_accounts, std::back_inserter(accounts),
                          [](const auto& account) { return account.Clone(); });

  EXPECT_CALL(observer, AccountsChanged()).Times(0);
  EXPECT_TRUE(service.AddHardwareAccountsSync(new_accounts_func()).empty());
  observer.WaitAndVerify();

  AccountUtils(&service).CreateWallet(kMnemonicDivideCruise, kPasswordBrave);
  service.Lock();
  EXPECT_CALL(observer, AccountsChanged()).Times(0);
  EXPECT_TRUE(service.AddHardwareAccountsSync(new_accounts_func()).empty());
  observer.WaitAndVerify();

  Unlock(&service, kPasswordBrave);

  EXPECT_CALL(observer, AccountsChanged());
  EXPECT_FALSE(service.AddHardwareAccountsSync(new_accounts_func()).empty());
  observer.WaitAndVerify();

  // ETH and FIL have hardware accounts
  EXPECT_FALSE(service.IsLockedSync());

  // First added hw account is selected.
  EXPECT_EQ(service.GetSelectedWalletAccount()->address, "0x111");
  EXPECT_EQ(service.GetSelectedEthereumDappAccount()->address, "0x111");
  EXPECT_EQ(service.GetSelectedSolanaDappAccount()->address,
            "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8");

  // Wallet is unlocked when the user has only hardware accounts
  EXPECT_FALSE(service.IsLockedSync());
  observer.WaitAndVerify();

  for (const auto& account : accounts) {
    std::string keyring_id_pref_key;
    if (account->keyring_id == mojom::kDefaultKeyringId) {
      keyring_id_pref_key = "default";
    } else if (account->keyring_id == mojom::kFilecoinKeyringId) {
      keyring_id_pref_key = "filecoin";
    } else if (account->keyring_id == mojom::kFilecoinTestnetKeyringId) {
      keyring_id_pref_key = "filecoin_testnet";
    } else if (account->keyring_id == mojom::kSolanaKeyringId) {
      keyring_id_pref_key = "solana";
    }
    auto path = keyring_id_pref_key + ".hardware." + account->device_id +
                ".account_metas." + account->address;
    ASSERT_TRUE(
        GetPrefs()->GetDict(kBraveWalletKeyrings).FindByDottedPath(path));
  }
  {
    // Checking Default keyring accounts
    auto account_infos = GetAccountUtils(&service).AllEthAccounts();

    EXPECT_EQ(account_infos.size(), 4u);

    EXPECT_EQ(account_infos[1]->address, "0x111");
    EXPECT_EQ(account_infos[1]->name, "name 1");
    EXPECT_EQ(account_infos[1]->account_id->kind,
              mojom::AccountKind::kHardware);
    ASSERT_TRUE(account_infos[1]->hardware);
    EXPECT_EQ(account_infos[1]->hardware->device_id, "device1");
    EXPECT_EQ(account_infos[1]->account_id->coin, mojom::CoinType::ETH);

    EXPECT_EQ(account_infos[2]->address, "0x222");
    EXPECT_EQ(account_infos[2]->name, "name 4");
    EXPECT_EQ(account_infos[2]->account_id->kind,
              mojom::AccountKind::kHardware);
    ASSERT_TRUE(account_infos[2]->hardware);
    EXPECT_EQ(account_infos[2]->hardware->device_id, "device1");
    EXPECT_EQ(account_infos[2]->account_id->coin, mojom::CoinType::ETH);

    EXPECT_EQ(account_infos[3]->address, "0xEA0");
    EXPECT_EQ(account_infos[3]->name, "name 3");
    EXPECT_EQ(account_infos[3]->account_id->kind,
              mojom::AccountKind::kHardware);
    ASSERT_TRUE(account_infos[3]->hardware);
    EXPECT_EQ(account_infos[3]->hardware->device_id, "device2");
    EXPECT_EQ(account_infos[3]->account_id->coin, mojom::CoinType::ETH);
  }
  {
    // Checking Filecoin keyring accounts
    auto account_infos = GetAccountUtils(&service).AllFilAccounts();
    EXPECT_EQ(account_infos.size(), 2u);

    EXPECT_EQ(account_infos[0]->address, "0x264");
    EXPECT_EQ(account_infos[0]->name, "name 2");
    EXPECT_EQ(account_infos[0]->account_id->kind,
              mojom::AccountKind::kHardware);
    ASSERT_TRUE(account_infos[0]->hardware);
    EXPECT_EQ(account_infos[0]->hardware->device_id, "device1");
    EXPECT_EQ(account_infos[0]->account_id->coin, mojom::CoinType::FIL);

    EXPECT_EQ(account_infos[1]->address, "0xFIL");
    EXPECT_EQ(account_infos[1]->name, "filecoin 1");
    EXPECT_EQ(account_infos[1]->account_id->kind,
              mojom::AccountKind::kHardware);
    ASSERT_TRUE(account_infos[1]->hardware);
    EXPECT_EQ(account_infos[1]->hardware->device_id, "device2");
    EXPECT_EQ(account_infos[1]->account_id->coin, mojom::CoinType::FIL);
  }
  {
    // Checking Filecoin keyring testnet accounts
    auto account_infos = GetAccountUtils(&service).AllFilTestAccounts();
    EXPECT_EQ(account_infos.size(), 1u);

    EXPECT_EQ(account_infos[0]->address, "0xFILTEST");
    EXPECT_EQ(account_infos[0]->name, "filecoin testnet 1");
    EXPECT_EQ(account_infos[0]->account_id->kind,
              mojom::AccountKind::kHardware);
    ASSERT_TRUE(account_infos[0]->hardware);
    EXPECT_EQ(account_infos[0]->hardware->device_id, "device2");
    EXPECT_EQ(account_infos[0]->account_id->coin, mojom::CoinType::FIL);
  }

  EXPECT_CALL(observer, AccountsChanged()).Times(0);
  EXPECT_FALSE(RemoveAccount(
      &service,
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kHardware, ""),
      ""));
  observer.WaitAndVerify();

  EXPECT_CALL(observer, AccountsChanged());
  EXPECT_TRUE(RemoveAccount(
      &service,
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kHardware, "0x111"),
      ""));
  observer.WaitAndVerify();

  ASSERT_FALSE(
      GetPrefs()
          ->GetDict(kBraveWalletKeyrings)
          .FindByDottedPath("default.hardware.device1.account_metas.0x111"));

  ASSERT_FALSE(
      GetPrefs()
          ->GetDict(kBraveWalletKeyrings)
          .FindByDottedPath("default.hardware.device1.account_metas.0x264"));

  ASSERT_TRUE(
      GetPrefs()
          ->GetDict(kBraveWalletKeyrings)
          .FindByDottedPath("default.hardware.device2.account_metas.0xEA0"));

  ASSERT_TRUE(
      GetPrefs()
          ->GetDict(kBraveWalletKeyrings)
          .FindByDottedPath("filecoin.hardware.device2.account_metas.0xFIL"));

  ASSERT_TRUE(
      GetPrefs()
          ->GetDict(kBraveWalletKeyrings)
          .FindByDottedPath(
              "filecoin_testnet.hardware.device2.account_metas.0xFILTEST"));

  EXPECT_CALL(observer, AccountsChanged());
  EXPECT_TRUE(RemoveAccount(
      &service,
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kHardware, "0xEA0"),
      ""));
  observer.WaitAndVerify();

  auto account_infos = GetAccountUtils(&service).AllEthAccounts();
  EXPECT_EQ(account_infos.size(), size_t(2));

  EXPECT_EQ(account_infos[1]->address, "0x222");
  EXPECT_EQ(account_infos[1]->name, "name 4");
  EXPECT_EQ(account_infos[1]->account_id->kind, mojom::AccountKind::kHardware);
  ASSERT_TRUE(account_infos[1]->hardware);
  EXPECT_EQ(account_infos[1]->hardware->device_id, "device1");
  EXPECT_EQ(account_infos[1]->account_id->coin, mojom::CoinType::ETH);

  EXPECT_CALL(observer, AccountsChanged());
  EXPECT_TRUE(RemoveAccount(
      &service,
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kHardware, "0x222"),
      ""));
  observer.WaitAndVerify();

  ASSERT_FALSE(
      GetPrefs()
          ->GetDict(kBraveWalletKeyrings)
          .FindByDottedPath("default.hardware.device2.account_metas.0xEA0"));

  ASSERT_FALSE(GetPrefs()
                   ->GetDict(kBraveWalletKeyrings)
                   .FindByDottedPath("default.hardware.device2"));

  EXPECT_CALL(observer, AccountsChanged());
  EXPECT_TRUE(RemoveAccount(
      &service,
      MakeAccountId(mojom::CoinType::FIL, mojom::kFilecoinKeyringId,
                    mojom::AccountKind::kHardware, "0xFIL"),
      ""));
  observer.WaitAndVerify();

  ASSERT_FALSE(
      GetPrefs()
          ->GetDict(kBraveWalletKeyrings)
          .FindByDottedPath("filecoin.hardware.device2.account_metas.0xFIL"));

  EXPECT_CALL(observer, AccountsChanged());
  EXPECT_TRUE(RemoveAccount(
      &service,
      MakeAccountId(mojom::CoinType::FIL, mojom::kFilecoinTestnetKeyringId,
                    mojom::AccountKind::kHardware, "0xFILTEST"),
      ""));
  observer.WaitAndVerify();
  ASSERT_FALSE(
      GetPrefs()
          ->GetDict(kBraveWalletKeyrings)
          .FindByDottedPath(
              "filecoin_testnet.hardware.device2.account_metas.0xFILTEST"));
}

TEST_F(KeyringServiceUnitTest, AutoLock) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  std::optional<std::string> mnemonic = CreateWallet(&service, "brave");
  ASSERT_TRUE(mnemonic.has_value());
  ASSERT_FALSE(service.IsLockedSync());

  // Should not be locked yet after 9 minutes
  task_environment_.FastForwardBy(base::Minutes(9));
  ASSERT_FALSE(service.IsLockedSync());

  // After the 10th minute, it should be locked
  task_environment_.FastForwardBy(base::Minutes(1));
  ASSERT_TRUE(service.IsLockedSync());
  // Locking after it is auto locked won't cause a crash
  service.Lock();
  ASSERT_TRUE(service.IsLockedSync());

  // Unlocking will reset the timer
  EXPECT_TRUE(Unlock(&service, "brave"));
  ASSERT_FALSE(service.IsLockedSync());
  task_environment_.FastForwardBy(base::Minutes(10));
  ASSERT_TRUE(service.IsLockedSync());

  // Locking before the timer fires won't cause any problems after the
  // timer fires.
  EXPECT_TRUE(Unlock(&service, "brave"));
  ASSERT_FALSE(service.IsLockedSync());
  task_environment_.FastForwardBy(base::Minutes(1));
  service.Lock();
  ASSERT_TRUE(service.IsLockedSync());
  task_environment_.FastForwardBy(base::Minutes(4));
  ASSERT_TRUE(service.IsLockedSync());

  // Restoring keyring will auto lock too
  service.Reset();
  ASSERT_TRUE(RestoreWallet(&service, *mnemonic, "brave", false));
  ASSERT_FALSE(service.IsLockedSync());
  task_environment_.FastForwardBy(base::Minutes(11));
  ASSERT_TRUE(service.IsLockedSync());

  // Changing the auto lock pref should reset the timer
  EXPECT_TRUE(Unlock(&service, "brave"));
  ASSERT_FALSE(service.IsLockedSync());
  task_environment_.FastForwardBy(base::Minutes(4));
  GetPrefs()->SetInteger(kBraveWalletAutoLockMinutes, 3);
  task_environment_.FastForwardBy(base::Minutes(2));
  EXPECT_FALSE(service.IsLockedSync());
  task_environment_.FastForwardBy(base::Minutes(1));
  EXPECT_TRUE(service.IsLockedSync());

  // Changing the auto lock pref should reset the timer even if higher
  // for simplicity of logic
  EXPECT_TRUE(Unlock(&service, "brave"));
  ASSERT_FALSE(service.IsLockedSync());
  task_environment_.FastForwardBy(base::Minutes(2));
  EXPECT_FALSE(service.IsLockedSync());
  GetPrefs()->SetInteger(kBraveWalletAutoLockMinutes, 10);
  task_environment_.FastForwardBy(base::Minutes(9));
  EXPECT_FALSE(service.IsLockedSync());
  task_environment_.FastForwardBy(base::Minutes(1));
  EXPECT_TRUE(service.IsLockedSync());
}

TEST_F(KeyringServiceUnitTest, NotifyUserInteraction) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(CreateWallet(&service, "brave"));
  ASSERT_FALSE(service.IsLockedSync());

  // Notifying of user interaction should keep the wallet unlocked
  task_environment_.FastForwardBy(base::Minutes(9));
  service.NotifyUserInteraction();
  task_environment_.FastForwardBy(base::Minutes(1));
  service.NotifyUserInteraction();
  task_environment_.FastForwardBy(base::Minutes(9));
  ASSERT_FALSE(service.IsLockedSync());
  task_environment_.FastForwardBy(base::Minutes(1));
  ASSERT_TRUE(service.IsLockedSync());
}

TEST_F(KeyringServiceUnitTest, SelectAddedAccount) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(CreateWallet(&service, "brave"));

  AddAccount(&service, mojom::CoinType::ETH, mojom::kDefaultKeyringId,
             "eth acc 1");
  AddAccount(&service, mojom::CoinType::ETH, mojom::kDefaultKeyringId,
             "eth acc 2");
  auto last_eth = AddAccount(&service, mojom::CoinType::ETH,
                             mojom::kDefaultKeyringId, "eth acc 3");

  AddAccount(&service, mojom::CoinType::SOL, mojom::kSolanaKeyringId,
             "sol acc 1");
  AddAccount(&service, mojom::CoinType::SOL, mojom::kSolanaKeyringId,
             "sol acc 2");
  auto last_sol = AddAccount(&service, mojom::CoinType::SOL,
                             mojom::kSolanaKeyringId, "sol acc 3");

  // Last added eth account becomes selected for eth dapp.
  ASSERT_EQ(service.GetSelectedEthereumDappAccount(), last_eth);

  // Last added sol account becomes selected for sol dapp.
  ASSERT_EQ(service.GetSelectedSolanaDappAccount(), last_sol);

  // Last added account becomes selected.
  ASSERT_EQ(service.GetSelectedWalletAccount(), last_sol);
}

TEST_F(KeyringServiceUnitTest, SelectAddedFilecoinAccount) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(CreateWallet(&service, "brave"));

  AddAccount(&service, mojom::CoinType::FIL, mojom::kFilecoinKeyringId,
             "fil acc 1");
  AddAccount(&service, mojom::CoinType::FIL, mojom::kFilecoinKeyringId,
             "fil acc 2");
  AddAccount(&service, mojom::CoinType::FIL, mojom::kFilecoinKeyringId,
             "fil acc 3");

  AddAccount(&service, mojom::CoinType::FIL, mojom::kFilecoinTestnetKeyringId,
             "fil acc 1");
  AddAccount(&service, mojom::CoinType::FIL, mojom::kFilecoinTestnetKeyringId,
             "fil acc 2");
  auto last_fil = AddAccount(&service, mojom::CoinType::FIL,
                             mojom::kFilecoinTestnetKeyringId, "fil acc 3");

  ASSERT_EQ(service.GetSelectedWalletAccount(), last_fil);
}

TEST_F(KeyringServiceUnitTest, SelectImportedFilecoinAccount) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(CreateWallet(&service, "brave"));
  ASSERT_FALSE(service.IsLockedSync());

  ImportFilecoinAccount(&service, "fil m acc 1",
                        "7b2254797065223a22736563703235366b31222c22507269766174"
                        "654b6579223a224169776f6a344469323155316844776835735348"
                        "434d7a37342b346c45303472376e5349454d706d6258493d227d",
                        mojom::kFilecoinMainnet);

  auto imported = ImportFilecoinAccount(
      &service, "fil m acc 1",
      "7b2254797065223a22736563703235366b31222c22507269766174"
      "654b6579223a226376414367502f53344f3274796c4f42466a6348"
      "33583154373677696661456c6646435057612b6a474a453d227d",
      mojom::kFilecoinMainnet);

  ASSERT_EQ(service.GetSelectedWalletAccount(), imported);

  ImportFilecoinAccount(&service, "fil t acc 2",
                        "7b2254797065223a22736563703235366b31222c22507269766174"
                        "654b6579223a226376414367502f53344f3274796c4f42466a6348"
                        "33583154373677696661456c6646435057612b6a474a453d227d",
                        mojom::kFilecoinTestnet);

  imported = ImportFilecoinAccount(
      &service, "fil t acc 2",
      "7b2254797065223a22736563703235366b31222c22507269766174"
      "654b6579223a224169776f6a344469323155316844776835735348"
      "434d7a37342b346c45303472376e5349454d706d6258493d227d",
      mojom::kFilecoinTestnet);

  ASSERT_EQ(service.GetSelectedWalletAccount(), imported);
}

TEST_F(KeyringServiceUnitTest, SelectImportedAccount) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(CreateWallet(&service, "brave"));

  ImportAccount(
      &service, "Best Evil Son",
      "d118a12a1e3b595d7d9e5599370df4ddc58d246a3ae4a795597e50eb6a32afb5",
      mojom::CoinType::ETH);

  auto imported = ImportAccount(
      &service, "Best Evil Son 2",
      "5b48615b7e43d015c3de46cbe9bc01bff9e106277a91bd44a55f9c4b1a268314",
      mojom::CoinType::ETH);

  ASSERT_EQ(service.GetSelectedWalletAccount(), imported);
}

TEST_F(KeyringServiceUnitTest, SelectHardwareAccount) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(CreateWallet(&service, "brave"));

  std::vector<mojom::HardwareWalletAccountPtr> new_accounts;
  std::string hardware_account1 = "0x1111111111111111111111111111111111111111";
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      hardware_account1, "m/44'/60'/1'/0/0", "name 1",
      mojom::HardwareVendor::kLedger, "device1", mojom::kDefaultKeyringId));
  std::string hardware_account2 = "0x2222222222222222222222222222222222222222";
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      hardware_account2, "m/44'/60'/1'/0/0", "name 2",
      mojom::HardwareVendor::kLedger, "device1", mojom::kDefaultKeyringId));

  auto imported = service.AddHardwareAccountsSync(std::move(new_accounts));

  // First account gets selected.
  ASSERT_EQ(service.GetSelectedWalletAccount(), imported.front());
}

TEST_F(KeyringServiceUnitTest, SetSelectedAccount) {
  const mojom::AccountInfoPtr empty_account;

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  SetNetwork(mojom::kFilecoinTestnet, mojom::CoinType::FIL);

  ASSERT_TRUE(CreateWallet(&service, "brave"));

  auto first_account = service.GetAllAccountsSync()->accounts[0]->Clone();
  auto first_sol_account = FirstSolAccount(&service);
  auto second_account =
      AddAccount(&service, mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                 "Who does number 2 work for");
  ASSERT_TRUE(second_account);

  // This does not depend on being locked
  EXPECT_TRUE(Lock(&service));

  // Added account is selected.
  EXPECT_EQ(second_account, service.GetSelectedWalletAccount());
  EXPECT_EQ(second_account, service.GetSelectedEthereumDappAccount());
  EXPECT_EQ(first_sol_account, service.GetSelectedSolanaDappAccount());

  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

  // Can select SOL account. dApp selections don't change.
  EXPECT_CALL(observer,
              SelectedWalletAccountChanged(Eq(std::ref(first_sol_account))));
  EXPECT_CALL(observer, SelectedDappAccountChanged(_, _)).Times(0);
  EXPECT_TRUE(SetSelectedAccount(&service, first_sol_account->account_id));
  EXPECT_EQ(first_sol_account, service.GetSelectedWalletAccount());
  EXPECT_EQ(second_account, service.GetSelectedEthereumDappAccount());
  EXPECT_EQ(first_sol_account, service.GetSelectedSolanaDappAccount());
  observer.WaitAndVerify();

  // Select back to ETH. dApp selections don't change.
  EXPECT_CALL(observer,
              SelectedWalletAccountChanged(Eq(std::ref(second_account))));
  EXPECT_CALL(observer, SelectedDappAccountChanged(_, _)).Times(0);
  EXPECT_TRUE(SetSelectedAccount(&service, second_account->account_id));
  EXPECT_EQ(second_account, service.GetSelectedWalletAccount());
  EXPECT_EQ(second_account, service.GetSelectedEthereumDappAccount());
  EXPECT_EQ(first_sol_account, service.GetSelectedSolanaDappAccount());
  observer.WaitAndVerify();

  // Selecting currently selected account does not trigger notifications.
  EXPECT_CALL(observer, SelectedWalletAccountChanged(_)).Times(0);
  EXPECT_CALL(observer, SelectedDappAccountChanged(_, _)).Times(0);
  EXPECT_TRUE(SetSelectedAccount(&service, second_account->account_id));
  EXPECT_EQ(second_account, service.GetSelectedWalletAccount());
  EXPECT_EQ(second_account, service.GetSelectedEthereumDappAccount());
  observer.WaitAndVerify();

  // Setting account to a valid address works
  EXPECT_CALL(observer,
              SelectedWalletAccountChanged(Eq(std::ref(first_account))));
  EXPECT_CALL(observer, SelectedDappAccountChanged(
                            mojom::CoinType::ETH, Eq(std::ref(first_account))));
  EXPECT_TRUE(SetSelectedAccount(&service, first_account->account_id));
  EXPECT_EQ(first_account, service.GetSelectedWalletAccount());
  EXPECT_EQ(first_account, service.GetSelectedEthereumDappAccount());
  observer.WaitAndVerify();

  // Setting account to a non-existing account doesn't work
  EXPECT_CALL(observer, SelectedWalletAccountChanged(_)).Times(0);
  EXPECT_CALL(observer, SelectedDappAccountChanged(_, _)).Times(0);
  EXPECT_FALSE(SetSelectedAccount(
      &service, MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                              mojom::AccountKind::kDerived,
                              "0xf83C3cBfF68086F276DD4f87A82DF73B57b21559")));
  EXPECT_EQ(first_account, service.GetSelectedWalletAccount());
  EXPECT_EQ(first_account, service.GetSelectedEthereumDappAccount());
  observer.WaitAndVerify();

  // Can import only when unlocked.
  // Then check that the account can be set to an imported account.
  EXPECT_TRUE(Unlock(&service, "brave"));
  auto imported_account = ImportAccount(
      &service, "Best Evil Son",
      // 0xDc06aE500aD5ebc5972A0D8Ada4733006E905976
      "d118a12a1e3b595d7d9e5599370df4ddc58d246a3ae4a795597e50eb6a32afb5",
      mojom::CoinType::ETH);
  ASSERT_TRUE(imported_account);
  EXPECT_EQ(imported_account, service.GetSelectedWalletAccount());
  EXPECT_EQ(imported_account, service.GetSelectedEthereumDappAccount());
  EXPECT_TRUE(Lock(&service));
  observer.WaitAndVerify();

  EXPECT_CALL(observer, SelectedWalletAccountChanged(_)).Times(0);
  EXPECT_CALL(observer, SelectedDappAccountChanged(_, _)).Times(0);
  EXPECT_TRUE(SetSelectedAccount(&service, imported_account->account_id));
  observer.WaitAndVerify();
  EXPECT_EQ(imported_account, service.GetSelectedWalletAccount());
  EXPECT_EQ(imported_account, service.GetSelectedEthereumDappAccount());

  // Removing the imported account resets account selection to first eth acc.
  EXPECT_TRUE(Unlock(&service, "brave"));
  EXPECT_CALL(observer,
              SelectedWalletAccountChanged(Eq(std::ref(first_account))));
  EXPECT_CALL(observer, SelectedDappAccountChanged(
                            mojom::CoinType::ETH, Eq(std::ref(first_account))));
  EXPECT_CALL(observer, SelectedDappAccountChanged(mojom::CoinType::SOL, _))
      .Times(0);
  EXPECT_TRUE(
      RemoveAccount(&service, imported_account->account_id, kPasswordBrave));
  EXPECT_TRUE(Lock(&service));
  EXPECT_EQ(first_account, service.GetSelectedWalletAccount());
  EXPECT_EQ(first_account, service.GetSelectedEthereumDappAccount());
  observer.WaitAndVerify();

  // Can set hardware account
  EXPECT_TRUE(Unlock(&service, "brave"));
  std::vector<mojom::HardwareWalletAccountPtr> new_accounts;
  std::string hardware_account = "0x1111111111111111111111111111111111111111";
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      hardware_account, "m/44'/60'/1'/0/0", "name 1",
      mojom::HardwareVendor::kLedger, "device1", mojom::kDefaultKeyringId));
  auto hw_account =
      service.AddHardwareAccountsSync(std::move(new_accounts))[0]->Clone();
  EXPECT_EQ(hw_account, service.GetSelectedWalletAccount());
  EXPECT_EQ(hw_account, service.GetSelectedEthereumDappAccount());
  observer.WaitAndVerify();

  EXPECT_CALL(observer, SelectedWalletAccountChanged(_)).Times(0);
  EXPECT_CALL(observer, SelectedDappAccountChanged(_, _)).Times(0);
  EXPECT_TRUE(SetSelectedAccount(&service, hw_account->account_id));
  observer.WaitAndVerify();

  // Can set Filecoin account
  EXPECT_CALL(observer, SelectedWalletAccountChanged(_));
  EXPECT_CALL(observer, SelectedDappAccountChanged(_, _)).Times(0);
  auto fil_imported_account = ImportFilecoinAccount(
      &service, "Imported Filecoin account 1",
      // t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q
      "7b2254797065223a22736563703235366b31222c22507269766174654b6579223a2257"
      "6b4545645a45794235364b5168512b453338786a7663464c2b545a4842464e732b696a"
      "58533535794b383d227d",
      mojom::kFilecoinTestnet);
  ASSERT_TRUE(fil_imported_account);
  EXPECT_EQ(fil_imported_account->address,
            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q");
  EXPECT_EQ(fil_imported_account, service.GetSelectedWalletAccount());
  observer.WaitAndVerify();

  EXPECT_CALL(observer, SelectedWalletAccountChanged(_)).Times(0);
  EXPECT_CALL(observer, SelectedDappAccountChanged(_, _)).Times(0);
  EXPECT_TRUE(SetSelectedAccount(&service, fil_imported_account->account_id));
  observer.WaitAndVerify();
  EXPECT_EQ(fil_imported_account, service.GetSelectedWalletAccount());

  // Can set Solana account
  auto sol_imported_account = ImportAccount(
      &service, "Imported Account 1",
      // C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ
      "sCzwsBKmKtk5Hgb4YUJAduQ5nmJq4GTyzCXhrKonAGaexa83MgSZuTSMS6TSZTndnC"
      "YbQtaJQKLXET9jVjepWXe",
      mojom::CoinType::SOL);
  ASSERT_TRUE(sol_imported_account);
  EXPECT_EQ(sol_imported_account->address,
            "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ");
  EXPECT_EQ(sol_imported_account, service.GetSelectedWalletAccount());
  EXPECT_EQ(sol_imported_account, service.GetSelectedSolanaDappAccount());
  observer.WaitAndVerify();

  // Selecting sol account doesn't change eth dapp.
  EXPECT_CALL(observer, SelectedWalletAccountChanged(_)).Times(0);
  EXPECT_CALL(observer, SelectedDappAccountChanged(mojom::CoinType::ETH, _))
      .Times(0);
  EXPECT_CALL(observer, SelectedDappAccountChanged(mojom::CoinType::SOL, _))
      .Times(0);
  EXPECT_TRUE(SetSelectedAccount(&service, sol_imported_account->account_id));
  EXPECT_EQ(sol_imported_account, service.GetSelectedWalletAccount());
  EXPECT_EQ(sol_imported_account, service.GetSelectedSolanaDappAccount());
  EXPECT_EQ(hw_account, service.GetSelectedEthereumDappAccount());
  observer.WaitAndVerify();

  // Selecting fil account doesn't change eth and sol dapp accounts.
  EXPECT_CALL(observer,
              SelectedWalletAccountChanged(Eq(std::ref(fil_imported_account))));
  EXPECT_CALL(observer, SelectedDappAccountChanged(_, _)).Times(0);
  EXPECT_TRUE(SetSelectedAccount(&service, fil_imported_account->account_id));
  observer.WaitAndVerify();

  // Removing currently selected account switches selection to first eth
  // account.
  EXPECT_CALL(observer,
              SelectedWalletAccountChanged(Eq(std::ref(first_account))));
  EXPECT_CALL(observer, SelectedDappAccountChanged(
                            mojom::CoinType::ETH, Eq(std::ref(first_account))));
  EXPECT_CALL(observer, SelectedDappAccountChanged(mojom::CoinType::SOL, _))
      .Times(0);
  EXPECT_TRUE(RemoveAccount(&service, fil_imported_account->account_id,
                            kPasswordBrave));
  EXPECT_EQ(first_account, service.GetSelectedWalletAccount());
  EXPECT_EQ(first_account, service.GetSelectedEthereumDappAccount());
  observer.WaitAndVerify();

  // Select hw account.
  EXPECT_CALL(observer, SelectedWalletAccountChanged(Eq(std::ref(hw_account))));
  EXPECT_CALL(observer, SelectedDappAccountChanged(mojom::CoinType::ETH,
                                                   Eq(std::ref(hw_account))));
  EXPECT_CALL(observer, SelectedDappAccountChanged(mojom::CoinType::SOL, _))
      .Times(0);
  EXPECT_TRUE(SetSelectedAccount(&service, hw_account->account_id));
  EXPECT_EQ(hw_account, service.GetSelectedWalletAccount());
  EXPECT_EQ(hw_account, service.GetSelectedEthereumDappAccount());
  observer.WaitAndVerify();

  // Remove selected hw account - switch to first eth account.
  EXPECT_CALL(observer,
              SelectedWalletAccountChanged(Eq(std::ref(first_account))));
  EXPECT_CALL(observer, SelectedDappAccountChanged(
                            mojom::CoinType::ETH, Eq(std::ref(first_account))));
  EXPECT_CALL(observer, SelectedDappAccountChanged(mojom::CoinType::SOL, _))
      .Times(0);
  EXPECT_TRUE(RemoveAccount(&service, hw_account->account_id, ""));
  EXPECT_EQ(first_account, service.GetSelectedWalletAccount());
  EXPECT_EQ(first_account, service.GetSelectedEthereumDappAccount());
  observer.WaitAndVerify();

  // Removing not-selected sol account. Only sol dapp observer is called with
  // empty arg.
  EXPECT_CALL(observer, SelectedWalletAccountChanged(_)).Times(0);
  EXPECT_CALL(observer, SelectedDappAccountChanged(mojom::CoinType::ETH, _))
      .Times(0);
  EXPECT_CALL(observer, SelectedDappAccountChanged(
                            mojom::CoinType::SOL, Eq(std::ref(empty_account))));
  EXPECT_TRUE(RemoveAccount(&service, sol_imported_account->account_id,
                            kPasswordBrave));
  observer.WaitAndVerify();
}

TEST_F(KeyringServiceUnitTest, AddAccountsWithDefaultName) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(CreateWallet(&service, "brave"));
  task_environment_.RunUntilIdle();
  ASSERT_FALSE(service.IsLockedSync());

  ASSERT_TRUE(AddAccount(&service, mojom::CoinType::ETH,
                         mojom::kDefaultKeyringId, "AccountAAAAH"));

  service.AddAccountsWithDefaultName(mojom::CoinType::ETH,
                                     mojom::kDefaultKeyringId, 3);

  auto account_infos = GetAccountUtils(&service).AllEthAccounts();
  EXPECT_EQ(account_infos.size(), 5u);
  EXPECT_FALSE(account_infos[0]->address.empty());
  EXPECT_EQ(account_infos[0]->name, "Account 1");
  EXPECT_EQ(account_infos[1]->name, "AccountAAAAH");
  EXPECT_EQ(account_infos[2]->name, "Account 3");
  EXPECT_EQ(account_infos[3]->name, "Account 4");
  EXPECT_EQ(account_infos[4]->name, "Account 5");
}

TEST_F(KeyringServiceUnitTest, SignMessageByDefaultKeyring) {
  // HDKeyringUnitTest.SignMessage already tests the correctness of signature
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(RestoreWallet(&service, kMnemonicDivideCruise, "brave", false));
  ASSERT_FALSE(service.IsLockedSync());

  auto account1 = GetAccountUtils(&service).EthAccountId(0);

  const std::vector<uint8_t> message = {0xde, 0xad, 0xbe, 0xef};
  auto sig_with_err = service.SignMessageByDefaultKeyring(account1, message);
  EXPECT_NE(sig_with_err.signature, std::nullopt);
  EXPECT_FALSE(sig_with_err.signature->empty());
  EXPECT_TRUE(sig_with_err.error_message.empty());

  // message is 0x
  sig_with_err =
      service.SignMessageByDefaultKeyring(account1, std::vector<uint8_t>());
  EXPECT_NE(sig_with_err.signature, std::nullopt);
  EXPECT_FALSE(sig_with_err.signature->empty());
  EXPECT_TRUE(sig_with_err.error_message.empty());

  // not a valid account in this wallet
  auto invalid_account = GetAccountUtils(&service).EthUnkownAccountId();
  sig_with_err = service.SignMessageByDefaultKeyring(invalid_account, message);
  EXPECT_EQ(sig_with_err.signature, std::nullopt);
  EXPECT_EQ(
      sig_with_err.error_message,
      l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_SIGN_MESSAGE_INVALID_ADDRESS,
                                base::ASCIIToUTF16(invalid_account->address)));

  // Cannot sign message when locked
  service.Lock();
  sig_with_err = service.SignMessageByDefaultKeyring(account1, message);
  EXPECT_EQ(sig_with_err.signature, std::nullopt);
  EXPECT_EQ(
      sig_with_err.error_message,
      l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SIGN_MESSAGE_UNLOCK_FIRST));
}

TEST_F(KeyringServiceUnitTest, GetSetAutoLockMinutes) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

  EXPECT_EQ(10, GetAutoLockMinutes(&service));

  EXPECT_CALL(observer, AutoLockMinutesChanged());
  EXPECT_TRUE(SetAutoLockMinutes(&service, 7));
  EXPECT_EQ(7, GetAutoLockMinutes(&service));
  observer.WaitAndVerify();

  EXPECT_CALL(observer, AutoLockMinutesChanged());
  EXPECT_TRUE(SetAutoLockMinutes(&service, 3));
  EXPECT_EQ(3, GetAutoLockMinutes(&service));
  observer.WaitAndVerify();

  // Out of bound values cannot be set
  EXPECT_CALL(observer, AutoLockMinutesChanged()).Times(0);
  EXPECT_FALSE(SetAutoLockMinutes(&service, kAutoLockMinutesMin - 1));
  EXPECT_EQ(3, GetAutoLockMinutes(&service));
  observer.WaitAndVerify();

  EXPECT_CALL(observer, AutoLockMinutesChanged()).Times(0);
  EXPECT_FALSE(SetAutoLockMinutes(&service, kAutoLockMinutesMax + 1));
  EXPECT_EQ(3, GetAutoLockMinutes(&service));
  observer.WaitAndVerify();

  // Bound values can be set
  EXPECT_CALL(observer, AutoLockMinutesChanged());
  EXPECT_TRUE(SetAutoLockMinutes(&service, kAutoLockMinutesMin));
  EXPECT_EQ(kAutoLockMinutesMin, GetAutoLockMinutes(&service));
  observer.WaitAndVerify();

  EXPECT_CALL(observer, AutoLockMinutesChanged());
  EXPECT_TRUE(SetAutoLockMinutes(&service, kAutoLockMinutesMax));
  EXPECT_EQ(kAutoLockMinutesMax, GetAutoLockMinutes(&service));
  observer.WaitAndVerify();
}

TEST_F(KeyringServiceUnitTest, SetAccountName_HardwareAccounts) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  ASSERT_TRUE(CreateWallet(&service, "brave"));

  std::vector<mojom::HardwareWalletAccountPtr> new_accounts;
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0x111", "m/44'/60'/1'/0/0", "name 1", mojom::HardwareVendor::kLedger,
      "device1", mojom::kDefaultKeyringId));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0x264", "m/44'/461'/0'/0/0", "name 2", mojom::HardwareVendor::kLedger,
      "device1", mojom::kFilecoinKeyringId));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0xEA0", "m/44'/60'/2'/0/0", "name 3", mojom::HardwareVendor::kLedger,
      "device2", mojom::kDefaultKeyringId));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0xFIL", "m/44'/461'/2'/0/0", "filecoin 1",
      mojom::HardwareVendor::kLedger, "device2", mojom::kFilecoinKeyringId));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0x222", "m/44'/60'/3'/0/0", "name 4", mojom::HardwareVendor::kLedger,
      "device1", mojom::kDefaultKeyringId));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0xFILTEST", "m/44'/1'/2'/0/0", "filecoin testnet 1",
      mojom::HardwareVendor::kLedger, "device2",
      mojom::kFilecoinTestnetKeyringId));

  service.AddHardwareAccountsSync(std::move(new_accounts));

  SetAccountName(&service,
                 MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                               mojom::AccountKind::kHardware, "0x111"),
                 "name 1 changed");
  SetAccountName(&service,
                 MakeAccountId(mojom::CoinType::FIL, mojom::kFilecoinKeyringId,
                               mojom::AccountKind::kHardware, "0xFIL"),
                 "filecoin 1 changed");
  SetAccountName(
      &service,
      MakeAccountId(mojom::CoinType::FIL, mojom::kFilecoinTestnetKeyringId,
                    mojom::AccountKind::kHardware, "0xFILTEST"),
      "filecoin testnet 1 changed");

  auto account_infos = GetAccountUtils(&service).AllEthAccounts();
  EXPECT_FALSE(account_infos[1]->address.empty());
  EXPECT_EQ(account_infos[1]->name, "name 1 changed");

  account_infos = GetAccountUtils(&service).AllFilAccounts();
  EXPECT_FALSE(account_infos[1]->address.empty());
  EXPECT_EQ(account_infos[1]->name, "filecoin 1 changed");

  account_infos = GetAccountUtils(&service).AllFilTestAccounts();
  EXPECT_FALSE(account_infos[0]->address.empty());
  EXPECT_EQ(account_infos[0]->name, "filecoin testnet 1 changed");
}

TEST_F(KeyringServiceUnitTest, SetDefaultKeyringHardwareAccountName) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  ASSERT_TRUE(CreateWallet(&service, "brave"));

  const struct {
    const char* address;
    const char* derivation_path;
    const char* name;
    mojom::HardwareVendor vendor;
    const char* device_id;
    const mojom::KeyringId keyring_id;
    mojom::CoinType coin;
  } hardware_accounts[] = {
      {"0x111", "m/44'/60'/1'/0/0", "name 1", mojom::HardwareVendor::kLedger,
       "device1", mojom::kDefaultKeyringId, mojom::CoinType::ETH},
      {"0x264", "m/44'/60'/2'/0/0", "name 2", mojom::HardwareVendor::kLedger,
       "device1", mojom::kDefaultKeyringId, mojom::CoinType::ETH},
      {"0xEA0", "m/44'/60'/3'/0/0", "name 3", mojom::HardwareVendor::kLedger,
       "device2", mojom::kDefaultKeyringId, mojom::CoinType::ETH}};

  std::vector<mojom::HardwareWalletAccountPtr> new_accounts;
  for (const auto& it : hardware_accounts) {
    new_accounts.push_back(mojom::HardwareWalletAccount::New(
        it.address, it.derivation_path, it.name, it.vendor, it.device_id,
        it.keyring_id));
  }

  const std::string kUpdatedName = "Updated ledger account 2";

  // Fail when no hardware accounts.
  EXPECT_FALSE(SetAccountName(
      &service,
      MakeAccountId(hardware_accounts[1].coin, hardware_accounts[1].keyring_id,
                    mojom::AccountKind::kHardware,
                    hardware_accounts[1].address),
      kUpdatedName));

  service.AddHardwareAccountsSync(std::move(new_accounts));

  // Empty address should fail.
  EXPECT_FALSE(SetAccountName(
      &service,
      MakeAccountId(hardware_accounts[1].coin, hardware_accounts[1].keyring_id,
                    mojom::AccountKind::kHardware, ""),
      kUpdatedName));

  // Empty name should fail.
  EXPECT_FALSE(SetAccountName(
      &service,
      MakeAccountId(hardware_accounts[1].coin, hardware_accounts[1].keyring_id,
                    mojom::AccountKind::kHardware,
                    hardware_accounts[1].address),
      ""));

  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

  // Update second hardware account's name.
  EXPECT_CALL(observer, AccountsChanged());
  EXPECT_TRUE(SetAccountName(
      &service,
      MakeAccountId(hardware_accounts[1].coin, hardware_accounts[1].keyring_id,
                    mojom::AccountKind::kHardware,
                    hardware_accounts[1].address),
      kUpdatedName));
  observer.WaitAndVerify();

  // Only second hardware account's name is updated.
  auto account_infos = GetAccountUtils(&service).AllEthAccounts();

  EXPECT_EQ(account_infos.size(), 4u);
  EXPECT_FALSE(account_infos[0]->address.empty());
  EXPECT_EQ(account_infos[0]->name, "Account 1");
  EXPECT_FALSE(account_infos[0]->hardware);
  EXPECT_EQ(account_infos[1]->address, hardware_accounts[0].address);
  EXPECT_EQ(account_infos[1]->name, hardware_accounts[0].name);
  EXPECT_TRUE(account_infos[1]->hardware);
  EXPECT_EQ(account_infos[2]->address, hardware_accounts[1].address);
  EXPECT_EQ(account_infos[2]->name, kUpdatedName);
  EXPECT_TRUE(account_infos[2]->hardware);
  EXPECT_EQ(account_infos[3]->address, hardware_accounts[2].address);
  EXPECT_EQ(account_infos[3]->name, hardware_accounts[2].name);
  EXPECT_TRUE(account_infos[3]->hardware);
}

TEST_F(KeyringServiceUnitTest, IsStrongPassword) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  // Strong password that meets the length requirement passes
  EXPECT_TRUE(IsStrongPassword(&service, "LDKH66BJbLsHQPEAK@4_zak*"));
  // Character requirement can be lowercase
  EXPECT_TRUE(IsStrongPassword(&service, "663@4_*a"));
  // Character requirement can be uppercase
  EXPECT_TRUE(IsStrongPassword(&service, "663@4_*A"));
  // Character requirement can be all numbers
  EXPECT_TRUE(IsStrongPassword(&service, "663456456546546"));
  // Character requirement can be all letters
  EXPECT_TRUE(IsStrongPassword(&service, "qwertyuiop"));
  // Space is ok for non alphanumeric requirement
  EXPECT_TRUE(IsStrongPassword(&service, "LDKH66BJbLsH QPEAK4zak"));
  // Password length less than 8 characters should fail
  EXPECT_FALSE(IsStrongPassword(&service, "a7_&YF"));
  // Empty password is not accepted
  EXPECT_FALSE(IsStrongPassword(&service, ""));
}

TEST_F(KeyringServiceUnitTest, GetChecksumEthAddress) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  EXPECT_EQ(GetChecksumEthAddress(&service,
                                  "0x0D8775F648430679A709E98D2B0CB6250D2887EF"),
            "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
  EXPECT_EQ(GetChecksumEthAddress(&service,
                                  "0x0d8775f648430679a709e98d2b0cb6250d2887ef"),
            "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
  EXPECT_EQ(GetChecksumEthAddress(&service,
                                  "0x0D8775F648430679A709E98d2b0Cb6250d2887EF"),
            "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
  EXPECT_EQ(GetChecksumEthAddress(&service,
                                  "0x0000000000000000000000000000000000000000"),
            "0x0000000000000000000000000000000000000000");
  // Invalid input
  EXPECT_EQ(GetChecksumEthAddress(&service, ""), "0x");
  EXPECT_EQ(GetChecksumEthAddress(&service, "0"), "0x");
  EXPECT_EQ(GetChecksumEthAddress(&service, "0x"), "0x");
  EXPECT_EQ(GetChecksumEthAddress(&service, "hello"), "0x");
}

TEST_F(KeyringServiceUnitTest, SignTransactionByFilecoinKeyring) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  SetNetwork(mojom::kFilecoinTestnet, mojom::CoinType::FIL);

  auto transaction = FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("1", "2", "3", "4", "5",
                            "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "6"));

  auto yet_unknown_account =
      MakeAccountId(mojom::CoinType::FIL, mojom::KeyringId::kFilecoinTestnet,
                    mojom::AccountKind::kImported,
                    "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q");
  ASSERT_FALSE(
      service.SignTransactionByFilecoinKeyring(*yet_unknown_account, nullptr));
  ASSERT_FALSE(service.SignTransactionByFilecoinKeyring(*yet_unknown_account,
                                                        &transaction.value()));

  ASSERT_TRUE(CreateWallet(&service, "brave"));

  auto imported_account = ImportFilecoinAccount(
      &service, "Imported Filecoin account 1",
      // t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q
      "7b2254797065223a22736563703235366b31222c22507269766174654b6579223a2257"
      "6b4545645a45794235364b5168512b453338786a7663464c2b545a4842464e732b696a"
      "58533535794b383d227d",
      mojom::kFilecoinTestnet);
  ASSERT_TRUE(imported_account);
  EXPECT_EQ(imported_account->address,
            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q");

  auto result = service.SignTransactionByFilecoinKeyring(
      *imported_account->account_id, &transaction.value());
  ASSERT_TRUE(result);
  std::string expected_result =
      R"({
      "Message": {
        "From": "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
        "GasFeeCap": "3",
        "GasLimit": 4,
        "GasPremium": "2",
        "Method": 0,
        "Nonce": 1,
        "Params": "",
        "To": "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
        "Value": "6",
        "Version": 0
      },
      "Signature": {
        "Data": "nbzCnsLhMGfRUmjiGP4y6Y+PxpXpGgPEPEujf8filC0tbyN8ntEril1x7cCZWpWyDUFM/VhEWaaCPgHlOQkh1AA=",
        "Type": 1
      }
    })";
  EXPECT_EQ(ParseJsonDict(*result), ParseJsonDict(expected_result));
}

TEST_F(KeyringServiceUnitTest, AddFilecoinAccounts) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  {
    ASSERT_TRUE(CreateWallet(&service, "brave"));
    ASSERT_TRUE(AddAccount(&service, mojom::CoinType::FIL,
                           mojom::kFilecoinTestnetKeyringId, "FIL account1"));
    service.Reset();
  }

  ASSERT_TRUE(CreateWallet(&service, "brave"));

  ASSERT_TRUE(ImportAccount(
      &service, "Imported account1",
      "d118a12a1e3b595d7d9e5599370df4ddc58d246a3ae4a795597e50eb6a32afb5",
      mojom::CoinType::ETH));

  ASSERT_TRUE(AddAccount(&service, mojom::CoinType::FIL,
                         mojom::kFilecoinKeyringId, "FIL account1"));
  ASSERT_TRUE(AddAccount(&service, mojom::CoinType::FIL,
                         mojom::kFilecoinTestnetKeyringId,
                         "FIL testnet account 1"));
  ASSERT_TRUE(AddAccount(&service, mojom::CoinType::FIL,
                         mojom::kFilecoinTestnetKeyringId,
                         "FIL testnet account 2"));

  // Lock and unlock won't fired created event again
  service.Lock();
  EXPECT_TRUE(Unlock(&service, "brave"));

  // FIL keyring already exists
  auto last_added_account =
      AddAccount(&service, mojom::CoinType::FIL, mojom::kFilecoinKeyringId,
                 "FIL account2");
  ASSERT_TRUE(last_added_account);

  auto account_infos = GetAccountUtils(&service).AllFilAccounts();
  EXPECT_EQ(account_infos.size(), 2u);
  EXPECT_EQ(account_infos[0]->name, "FIL account1");
  EXPECT_EQ(account_infos[0]->account_id->kind, mojom::AccountKind::kDerived);
  EXPECT_EQ(account_infos[1]->name, "FIL account2");
  EXPECT_EQ(account_infos[1]->account_id->kind, mojom::AccountKind::kDerived);

  account_infos = GetAccountUtils(&service).AllFilTestAccounts();
  EXPECT_EQ(account_infos.size(), 2u);
  EXPECT_EQ(account_infos[0]->name, "FIL testnet account 1");
  EXPECT_EQ(account_infos[0]->account_id->kind, mojom::AccountKind::kDerived);
  EXPECT_EQ(account_infos[1]->name, "FIL testnet account 2");
  EXPECT_EQ(account_infos[1]->account_id->kind, mojom::AccountKind::kDerived);

  EXPECT_EQ(last_added_account, service.GetSelectedWalletAccount());
}

TEST_F(KeyringServiceUnitTest, ImportFilecoinAccounts) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  ASSERT_TRUE(CreateWallet(&service, "brave"));

  const std::vector<ImportData> imported_testnet_accounts = {
      {mojom::kFilecoinTestnet, "Imported Filecoin account 1",
       "7b2254797065223a22736563703235366b31222c2250726976"
       "6174654b6579223a22576b4"
       "545645a45794235364b5168512b453338786a7663464c2b545"
       "a4842464e732b696a585335"
       "35794b383d227d",
       "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
       "WkEEdZEyB56KQhQ+E38xjvcFL+TZHBFNs+ijXS55yK8="},
      {mojom::kFilecoinTestnet, "Imported Filecoin account 2",
       "7b2254797065223a22736563703235366b31222c2250726976"
       "6174654b6579223a22774d5"
       "267766730734d6a764657356e32515472705a5658414c596a7"
       "44d7036725156714d52535a"
       "6a482f513d227d",
       "t1par4kjqybnejlyuvpa3rodmluidq34ba6muafda",
       "wMRgvg0sMjvFW5n2QTrpZVXALYjtMp6rQVqMRSZjH/Q="},
      {mojom::kFilecoinTestnet, "Imported Filecoin account 3",
       "7b2254797065223a22736563703235366b31222c2250726976"
       "6174654b6579223a22774e5"
       "3667774514d2f466b665334423334496a475750343553546b2"
       "f737434304c724379433955"
       "6a7761773d227d",
       "t1zvggbhs5sxyeifzcrmik5oljbley7lvo57ovusy",
       "wNSfwtQM/FkfS4B34IjGWP45STk/st40LrCyC9Ujwaw="},
      {mojom::kFilecoinTestnet, "Imported Filecoin account 4",
       "7b2254797065223a22626c73222c22507269766174654b6579"
       "223a2270536e7752332f385"
       "5616b53516f777858742b345a75393257586d424d526e74716"
       "d6448696136724853453d22"
       "7d",
       "t3wwtato54ee5aod7j5uv2n75jpyn4hpwx3f2kx5cijtoxgyti"
       "ul2dczrak3ghlbt5zjnj574"
       "y3snhcb5bthva",
       "pSnwR3/8UakSQowxXt+4Zu92WXmBMRntqmdHia6rHSE="}};

  const std::vector<ImportData> imported_mainnet_accounts = {
      {mojom::kFilecoinMainnet, "Imported Filecoin account 5",
       "7b2254797065223a22736563703235366b31222c22507269766174654b6579223a226"
       "359766b546f6d473050774357774d39675844757a737a"
       "684657725332427a33576264306f5574636d38593d227d",
       "f1iqwoqxlb4m57crfxl3kbzcehfuvkq7q4ak3mlla",
       "cYvkTomG0PwCWwM9gXDuzszhFWrS2Bz3Wbd0oUtcm8Y="},
      {mojom::kFilecoinMainnet, "Imported Filecoin account 6",
       "7b2254797065223a22736563703235366b31222c22507269766174"
       "654b6579223a224c6c5a75546d4d4a46674b4e6b774756575a564a7"
       "9704d514d782f52614d7063445775426b53326c746f413d227d",
       "f1spw7nkvh5bb7th2g7n2w4p7fmh5ukje2kazf4wa",
       "LlZuTmMJFgKNkwGVWZVJypMQMx/RaMpcDWuBkS2ltoA="}};
  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

  ImportFilecoinAccounts(&service, &observer, imported_testnet_accounts,
                         mojom::kFilecoinTestnetKeyringId);
  ImportFilecoinAccounts(&service, &observer, imported_mainnet_accounts,
                         mojom::kFilecoinKeyringId);

  auto* filecoin_testnet_keyring =
      service.GetHDKeyringById(mojom::kFilecoinTestnetKeyringId);
  EXPECT_EQ(filecoin_testnet_keyring->GetImportedAccountsForTesting().size(),
            imported_testnet_accounts.size());

  // Remove testnet account
  EXPECT_CALL(observer, AccountsChanged());
  EXPECT_TRUE(RemoveAccount(
      &service,
      MakeAccountId(mojom::CoinType::FIL, mojom::kFilecoinTestnetKeyringId,
                    mojom::AccountKind::kImported,
                    imported_testnet_accounts[1].address),
      kPasswordBrave));
  observer.WaitAndVerify();

  // Remove mainnet account
  EXPECT_CALL(observer, AccountsChanged());
  EXPECT_TRUE(RemoveAccount(
      &service,
      MakeAccountId(mojom::CoinType::FIL, mojom::kFilecoinKeyringId,
                    mojom::AccountKind::kImported,
                    imported_mainnet_accounts[1].address),
      kPasswordBrave));
  observer.WaitAndVerify();

  EXPECT_EQ(filecoin_testnet_keyring->GetImportedAccountsForTesting().size(),
            imported_testnet_accounts.size() - 1);
  // remove invalid address
  EXPECT_CALL(observer, AccountsChanged()).Times(0);
  EXPECT_FALSE(RemoveAccount(
      &service,
      MakeAccountId(mojom::CoinType::FIL, mojom::kFilecoinKeyringId,
                    mojom::AccountKind::kImported, "0xxxxxxxxxx0"),
      kPasswordBrave));
  observer.WaitAndVerify();

  auto account_infos = GetAccountUtils(&service).AllFilTestAccounts();
  EXPECT_EQ(account_infos.size(), imported_testnet_accounts.size() - 1);
  EXPECT_EQ(account_infos[0]->address, imported_testnet_accounts[0].address);
  EXPECT_EQ(account_infos[0]->name, imported_testnet_accounts[0].name);
  EXPECT_EQ(account_infos[0]->account_id->kind, mojom::AccountKind::kImported);
  EXPECT_EQ(account_infos[1]->address, imported_testnet_accounts[2].address);
  EXPECT_EQ(account_infos[1]->name, imported_testnet_accounts[2].name);
  EXPECT_EQ(account_infos[1]->account_id->kind, mojom::AccountKind::kImported);
  EXPECT_EQ(account_infos[2]->address, imported_testnet_accounts[3].address);
  EXPECT_EQ(account_infos[2]->name, imported_testnet_accounts[3].name);
  EXPECT_EQ(account_infos[2]->account_id->kind, mojom::AccountKind::kImported);
  EXPECT_EQ(filecoin_testnet_keyring->GetImportedAccountsForTesting().size(),
            imported_testnet_accounts.size() - 1);
  service.Lock();
  // cannot get private key when locked
  auto private_key = EncodePrivateKeyForExport(
      &service,
      MakeAccountId(mojom::CoinType::FIL, mojom::kFilecoinTestnetKeyringId,
                    mojom::AccountKind::kImported,
                    imported_testnet_accounts[0].address));
  EXPECT_FALSE(private_key);

  EXPECT_TRUE(Unlock(&service, "brave"));

  account_infos = GetAccountUtils(&service).AllFilTestAccounts();
  // Imported accounts should be restored
  EXPECT_EQ(account_infos.size(), imported_testnet_accounts.size() - 1);
  EXPECT_EQ(account_infos[0]->address, imported_testnet_accounts[0].address);
  EXPECT_EQ(account_infos[0]->name, imported_testnet_accounts[0].name);
  EXPECT_EQ(account_infos[0]->account_id->kind, mojom::AccountKind::kImported);
  EXPECT_EQ(account_infos[1]->address, imported_testnet_accounts[2].address);
  EXPECT_EQ(account_infos[1]->name, imported_testnet_accounts[2].name);
  EXPECT_EQ(account_infos[1]->account_id->kind, mojom::AccountKind::kImported);

  account_infos = GetAccountUtils(&service).AllFilAccounts();
  EXPECT_EQ(account_infos.size(), imported_mainnet_accounts.size() - 1);
  EXPECT_EQ(account_infos[0]->address, imported_mainnet_accounts[0].address);
  EXPECT_EQ(account_infos[0]->name, imported_mainnet_accounts[0].name);
  EXPECT_EQ(account_infos[0]->account_id->kind, mojom::AccountKind::kImported);

  EXPECT_EQ(service.GetHDKeyringById(mojom::kFilecoinTestnetKeyringId)
                ->GetImportedAccountsForTesting()
                .size(),
            imported_testnet_accounts.size() - 1);
  auto payload = EncodePrivateKeyForExport(
      &service,
      MakeAccountId(mojom::CoinType::FIL, mojom::kFilecoinTestnetKeyringId,
                    mojom::AccountKind::kImported,
                    imported_testnet_accounts[0].address));
  EXPECT_TRUE(payload);
  EXPECT_EQ(imported_testnet_accounts[0].import_payload, *payload);

  auto* default_keyring = service.GetHDKeyringById(mojom::kDefaultKeyringId);
  // Imported accounts should also be restored in filecoin keyring
  EXPECT_EQ(default_keyring->GetImportedAccountsForTesting().size(), 0u);
  EXPECT_EQ(service.GetHDKeyringById(mojom::kFilecoinTestnetKeyringId)
                ->GetImportedAccountsForTesting()
                .size(),
            imported_testnet_accounts.size() - 1);
}

TEST_F(KeyringServiceUnitTest, ImportBitcoinAccount) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  ASSERT_TRUE(CreateWallet(&service, "brave"));
  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);
  EXPECT_CALL(observer, AccountsAdded(_)).Times(0);

  EXPECT_FALSE(service.ImportBitcoinAccountSync("", kBtcMainnetImportAccount0,
                                                mojom::kBitcoinMainnet));
  EXPECT_FALSE(service.ImportBitcoinAccountSync(
      "Btc import", kBtcMainnetImportAccount0, mojom::kMainnetChainId));
  EXPECT_FALSE(service.ImportBitcoinAccountSync(
      "Btc import", kBtcMainnetImportAccount0, mojom::kBitcoinTestnet));
  EXPECT_FALSE(service.ImportBitcoinAccountSync(
      "Btc import", kBtcTestnetImportAccount0, mojom::kBitcoinMainnet));
  observer.WaitAndVerify();

  EXPECT_EQ(0u, GetAccountUtils(&service).AllBtcAccounts().size());
  EXPECT_EQ(0u, GetAccountUtils(&service).AllBtcTestAccounts().size());

  EXPECT_CALL(observer, AccountsAdded(_)).Times(3);
  auto acc1 = service.ImportBitcoinAccountSync(
      "Btc import 1", kBtcMainnetImportAccount0, mojom::kBitcoinMainnet);
  auto acc2 = service.ImportBitcoinAccountSync(
      "Btc import 2", kBtcMainnetImportAccount1, mojom::kBitcoinMainnet);
  auto acc3 = service.ImportBitcoinAccountSync(
      "Btc import 3", kBtcTestnetImportAccount0, mojom::kBitcoinTestnet);
  ASSERT_TRUE(acc1);
  ASSERT_TRUE(acc2);
  ASSERT_TRUE(acc3);
  observer.WaitAndVerify();

  EXPECT_EQ(2u, GetAccountUtils(&service).AllBtcAccounts().size());
  EXPECT_EQ(1u, GetAccountUtils(&service).AllBtcTestAccounts().size());

  EXPECT_EQ(GetAccountUtils(&service).AllBtcAccounts()[0], acc1);
  EXPECT_EQ(GetAccountUtils(&service).AllBtcAccounts()[1], acc2);
  EXPECT_EQ(GetAccountUtils(&service).AllBtcTestAccounts()[0], acc3);

  EXPECT_CALL(observer, AccountsChanged());
  EXPECT_TRUE(RemoveAccount(&service, acc1->account_id, kPasswordBrave));
  observer.WaitAndVerify();

  EXPECT_CALL(observer, AccountsChanged());
  EXPECT_TRUE(RemoveAccount(&service, acc2->account_id, kPasswordBrave));
  observer.WaitAndVerify();

  EXPECT_CALL(observer, AccountsChanged());
  EXPECT_TRUE(RemoveAccount(&service, acc3->account_id, kPasswordBrave));
  observer.WaitAndVerify();

  EXPECT_EQ(0u, GetAccountUtils(&service).AllBtcAccounts().size());
  EXPECT_EQ(0u, GetAccountUtils(&service).AllBtcTestAccounts().size());
}

TEST_F(KeyringServiceUnitTest, HardwareBitcoinAccount) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  ASSERT_TRUE(CreateWallet(&service, "brave"));
  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);
  EXPECT_CALL(observer, AccountsAdded(_)).Times(0);

  auto wrong_keyring_hw_info = mojom::HardwareWalletAccount::New(
      kBtcMainnetHardwareAccount0, "derivation_path", "Btc hw account 1",
      mojom::HardwareVendor::kLedger, "device_id",
      mojom::KeyringId::kBitcoinHardwareTestnet);
  EXPECT_FALSE(
      service.AddBitcoinHardwareAccountSync(wrong_keyring_hw_info.Clone()));

  EXPECT_EQ(0u, GetAccountUtils(&service).AllBtcAccounts().size());
  EXPECT_EQ(0u, GetAccountUtils(&service).AllBtcTestAccounts().size());

  auto hw_info_1 = mojom::HardwareWalletAccount::New(
      kBtcMainnetHardwareAccount0, "derivation_path", "Btc hw account 1",
      mojom::HardwareVendor::kLedger, "device_id",
      mojom::KeyringId::kBitcoinHardware);
  auto hw_info_2 = mojom::HardwareWalletAccount::New(
      kBtcMainnetHardwareAccount1, "derivation_path", "Btc hw account 2",
      mojom::HardwareVendor::kLedger, "device_id",
      mojom::KeyringId::kBitcoinHardware);
  auto hw_info_3 = mojom::HardwareWalletAccount::New(
      kBtcTestnetHardwareAccount0, "derivation_path", "Btc hw account 3",
      mojom::HardwareVendor::kLedger, "device_id",
      mojom::KeyringId::kBitcoinHardwareTestnet);

  EXPECT_CALL(observer, AccountsAdded(_)).Times(3);
  auto acc1 = service.AddBitcoinHardwareAccountSync(hw_info_1.Clone());
  auto acc2 = service.AddBitcoinHardwareAccountSync(hw_info_2.Clone());
  auto acc3 = service.AddBitcoinHardwareAccountSync(hw_info_3.Clone());
  ASSERT_TRUE(acc1);
  ASSERT_TRUE(acc2);
  ASSERT_TRUE(acc3);
  observer.WaitAndVerify();

  EXPECT_EQ(2u, GetAccountUtils(&service).AllBtcAccounts().size());
  EXPECT_EQ(1u, GetAccountUtils(&service).AllBtcTestAccounts().size());

  EXPECT_EQ(GetAccountUtils(&service).AllBtcAccounts()[0], acc1);
  EXPECT_EQ(GetAccountUtils(&service).AllBtcAccounts()[1], acc2);
  EXPECT_EQ(GetAccountUtils(&service).AllBtcTestAccounts()[0], acc3);

  EXPECT_CALL(observer, AccountsChanged());
  EXPECT_TRUE(RemoveAccount(&service, acc1->account_id, kPasswordBrave));
  observer.WaitAndVerify();

  EXPECT_CALL(observer, AccountsChanged());
  EXPECT_TRUE(RemoveAccount(&service, acc2->account_id, kPasswordBrave));
  observer.WaitAndVerify();

  EXPECT_CALL(observer, AccountsChanged());
  SetAccountName(&service, acc3->account_id.Clone(), "name 3 changed");
  observer.WaitAndVerify();
  EXPECT_EQ(GetAccountUtils(&service).AllBtcTestAccounts()[0]->name,
            "name 3 changed");

  EXPECT_CALL(observer, AccountsChanged());
  EXPECT_TRUE(RemoveAccount(&service, acc3->account_id, kPasswordBrave));
  observer.WaitAndVerify();

  EXPECT_EQ(0u, GetAccountUtils(&service).AllBtcAccounts().size());
  EXPECT_EQ(0u, GetAccountUtils(&service).AllBtcTestAccounts().size());
}

TEST_F(KeyringServiceUnitTest, SolanaKeyring) {
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

    ASSERT_TRUE(CreateWallet(&service, "brave"));
    ASSERT_TRUE(AddAccount(&service, mojom::CoinType::SOL,
                           mojom::kSolanaKeyringId, "Account 2"));

    service.Lock();
    ASSERT_TRUE(Unlock(&service, "brave"));

    auto account_infos = GetAccountUtils(&service).AllSolAccounts();
    EXPECT_EQ(account_infos.size(), 2u);
    EXPECT_EQ(account_infos[0]->name, "Solana Account 1");
    EXPECT_EQ(account_infos[0]->account_id->kind, mojom::AccountKind::kDerived);
    EXPECT_EQ(account_infos[1]->name, "Account 2");
    EXPECT_EQ(account_infos[1]->account_id->kind, mojom::AccountKind::kDerived);

    service.Reset();
  }
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

    EXPECT_CALL(observer, WalletRestored());
    ASSERT_TRUE(RestoreWallet(&service, kMnemonicDivideCruise, "brave", false));
    observer.WaitAndVerify();

    auto account_infos = GetAccountUtils(&service).AllSolAccounts();
    EXPECT_EQ(account_infos.size(), 1u);
    EXPECT_EQ(account_infos[0]->name, "Solana Account 1");
    EXPECT_EQ(account_infos[0]->address,
              "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8");
    EXPECT_EQ(account_infos[0]->account_id->kind, mojom::AccountKind::kDerived);

    service.Reset();
  }

  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    ASSERT_TRUE(CreateWallet(&service, "brave"));

    auto imported_account = ImportAccount(
        &service, "Imported Account 1",
        "sCzwsBKmKtk5Hgb4YUJAduQ5nmJq4GTyzCXhrKonAGaexa83MgSZuTSMS6TSZTndnC"
        "YbQtaJQKLXET9jVjepWXe",
        mojom::CoinType::SOL);
    ASSERT_TRUE(imported_account);
    EXPECT_EQ(imported_account->address,
              "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ");
    auto private_key = EncodePrivateKeyForExport(
        &service,
        MakeAccountId(mojom::CoinType::SOL, mojom::kSolanaKeyringId,
                      mojom::AccountKind::kImported,
                      "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ"));
    EXPECT_TRUE(private_key);
    EXPECT_EQ(*private_key,
              "sCzwsBKmKtk5Hgb4YUJAduQ5nmJq4GTyzCXhrKonAGaexa83MgSZuTSMS6TSZTnd"
              "nCYbQtaJQKLXET9jVjepWXe");

    // wrong encoded private key (same bytes but not encoded in keypair)
    EXPECT_FALSE(ImportAccount(&service, "Imported Failed",
                               "3v1fSGD1JW5XnAd2FWrjV6HWJHM9DofVjuNt4T5b7CDL",
                               mojom::CoinType::SOL));
    imported_account = ImportAccount(
        &service, "Imported Account 2",
        "4pNHX6ATNXad3KZTb2PXTosW5ceaxqx45M9NH9pjcZCH9qoQKx6RMzUjuzm6J9Y2uwjCxJ"
        "c5JsjL1TrGr1X3nPFP",
        mojom::CoinType::SOL);
    ASSERT_TRUE(imported_account);
    ASSERT_TRUE(
        RemoveAccount(&service, imported_account->account_id, kPasswordBrave));

    // import using uint8array
    imported_account = ImportAccount(
        &service, "Imported Account 3",
        " [4,109,17,28,245,96,126,232,185,242,61,170,96,51,225,202,152,85,104,"
        "63,4,171,245,175,118,67,238,247,208,163,247,211,201,215,12,121,255,"
        "182,188,11,4,82,78,239,173,146,246,74,66,126,34,173,46,211,145,49,211,"
        "176,28,89,250,190,34,254]\t\n",
        mojom::CoinType::SOL);
    ASSERT_TRUE(imported_account);
    ASSERT_TRUE(
        RemoveAccount(&service, imported_account->account_id, kPasswordBrave));

    auto account_infos = GetAccountUtils(&service).AllSolAccounts();
    ASSERT_EQ(account_infos.size(), 2u);
    EXPECT_EQ(account_infos[1]->name, "Imported Account 1");
    EXPECT_EQ(account_infos[1]->address,
              "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ");
    EXPECT_EQ(account_infos[1]->account_id->kind,
              mojom::AccountKind::kImported);

    service.Lock();
    EXPECT_TRUE(Unlock(&service, "brave"));
    // imported accounts persist after lock & unlock
    account_infos = GetAccountUtils(&service).AllSolAccounts();
    ASSERT_EQ(account_infos.size(), 2u);
    EXPECT_EQ(account_infos[1]->name, "Imported Account 1");
    EXPECT_EQ(account_infos[1]->address,
              "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ");
    EXPECT_EQ(account_infos[1]->account_id->kind,
              mojom::AccountKind::kImported);

    service.Reset();
  }
}

TEST_F(KeyringServiceUnitTest, SignMessage) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(RestoreWallet(&service, kMnemonicDivideCruise, "brave", false));
  task_environment_.RunUntilIdle();

  auto first_sol_account = FirstSolAccount(&service);
  EXPECT_EQ(first_sol_account->address,
            "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8");

  const std::vector<uint8_t> message = {0xde, 0xad, 0xbe, 0xef};

  // invalid address for Solana keyring
  EXPECT_TRUE(
      service
          .SignMessageBySolanaKeyring(
              MakeAccountId(mojom::CoinType::SOL, mojom::KeyringId::kSolana,
                            mojom::AccountKind::kDerived,
                            "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db"),
              message)
          .empty());

  EXPECT_FALSE(
      service.SignMessageBySolanaKeyring(first_sol_account->account_id, message)
          .empty());
}

class KeyringServiceAccountDiscoveryUnitTest : public KeyringServiceUnitTest {
 public:
  using InterceptorCallback =
      base::RepeatingCallback<std::string(const std::string&)>;

  void SetUp() override {
    KeyringServiceUnitTest::SetUp();
    url_loader_factory().SetInterceptor(base::BindRepeating(
        &KeyringServiceAccountDiscoveryUnitTest::Interceptor,
        base::Unretained(this)));
  }

  void PrepareAccounts(mojom::CoinType coin_type, mojom::KeyringId keyring_id) {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    saved_mnemonic_ = CreateWallet(&service, "brave").value_or("");
    EXPECT_FALSE(saved_mnemonic_.empty());

    for (size_t i = 0; i < 100u; ++i) {
      ASSERT_TRUE(AddAccount(&service, coin_type, keyring_id,
                             "Acc" + base::NumberToString(i)));
    }
    saved_addresses_ =
        service.GetHDKeyringById(keyring_id)->GetHDAccountsForTesting();
  }

  void set_eth_transaction_count_callback(InterceptorCallback cb) {
    eth_transaction_count_callback_ = std::move(cb);
  }

  void set_sol_balance_callback(InterceptorCallback cb) {
    sol_balance_callback_ = std::move(cb);
  }

  void set_fil_balance_callback(InterceptorCallback cb) {
    fil_balance_callback_ = std::move(cb);
  }

  const std::string& saved_mnemonic() { return saved_mnemonic_; }
  const std::vector<std::string>& saved_addresses() { return saved_addresses_; }

  void Interceptor(const network::ResourceRequest& request) {
    url_loader_factory().ClearResponses();
    std::string_view request_string(request.request_body->elements()
                                        ->at(0)
                                        .As<network::DataElementBytes>()
                                        .AsStringPiece());
    base::Value::Dict dict = ParseJsonDict(request_string);
    std::string* method = dict.FindString("method");
    ASSERT_TRUE(method);
    if (*method == "eth_getTransactionCount") {
      base::Value::List* params = dict.FindList("params");
      ASSERT_TRUE(params);
      std::string* address = (*params)[0].GetIfString();
      ASSERT_TRUE(address);

      if (eth_transaction_count_callback_) {
        url_loader_factory().AddResponse(
            request.url.spec(), eth_transaction_count_callback_.Run(*address));
      }
    }

    if (*method == "Filecoin.WalletBalance") {
      base::Value::List* params = dict.FindList("params");
      ASSERT_TRUE(params);
      std::string* address = (*params)[0].GetIfString();
      ASSERT_TRUE(address);

      if (fil_balance_callback_) {
        url_loader_factory().AddResponse(request.url.spec(),
                                         fil_balance_callback_.Run(*address));
      }
    }

    if (*method == "getBalance") {
      base::Value::List* params = dict.FindList("params");
      ASSERT_TRUE(params);
      std::string* address = (*params)[0].GetIfString();
      ASSERT_TRUE(address);

      if (sol_balance_callback_) {
        url_loader_factory().AddResponse(request.url.spec(),
                                         sol_balance_callback_.Run(*address));
      }
    }
  }

 protected:
  InterceptorCallback eth_transaction_count_callback_;
  InterceptorCallback fil_balance_callback_;
  InterceptorCallback sol_balance_callback_;

  std::string saved_mnemonic_;
  std::vector<std::string> saved_addresses_;
};

TEST_F(KeyringServiceAccountDiscoveryUnitTest, AccountDiscovery) {
  PrepareAccounts(mojom::CoinType::ETH, mojom::kDefaultKeyringId);
  BraveWalletService brave_wallet_service(
      shared_url_loader_factory(), TestBraveWalletServiceDelegate::Create(),
      GetPrefs(), GetLocalState());
  BitcoinTestRpcServer bitcoin_test_rpc_server(
      brave_wallet_service.GetBitcoinWalletService());

  KeyringService& service = *brave_wallet_service.keyring_service();

  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

  std::vector<std::string> requested_addresses;
  set_eth_transaction_count_callback(base::BindLambdaForTesting(
      [this, &requested_addresses](const std::string& address) -> std::string {
        requested_addresses.push_back(address);

        // 3rd and 10th have transactions.
        if (address == saved_addresses()[3] ||
            address == saved_addresses()[10]) {
          return R"({"jsonrpc":"2.0","id":"1","result":"0x1"})";
        } else {
          return R"({"jsonrpc":"2.0","id":"1","result":"0x0"})";
        }
      }));

  EXPECT_CALL(observer, AccountsChanged()).Times(2);  // Accounts 3 and 10.
  EXPECT_TRUE(RestoreWallet(&service, saved_mnemonic(), "brave1", false));
  observer.WaitAndVerify();
  std::vector<mojom::AccountInfoPtr> account_infos =
      service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
  EXPECT_EQ(account_infos.size(), 11u);
  for (size_t i = 0; i < account_infos.size(); ++i) {
    EXPECT_EQ(account_infos[i]->address, saved_addresses()[i]);
    EXPECT_EQ(account_infos[i]->name, "Account " + base::NumberToString(i + 1));
  }
  // 20 attempts more after Account 10 is added.
  EXPECT_THAT(requested_addresses, ElementsAreArray(&saved_addresses()[1], 30));
}

TEST_F(KeyringServiceAccountDiscoveryUnitTest, SolAccountDiscovery) {
  PrepareAccounts(mojom::CoinType::SOL, mojom::kSolanaKeyringId);

  BraveWalletService brave_wallet_service(
      shared_url_loader_factory(), TestBraveWalletServiceDelegate::Create(),
      GetPrefs(), GetLocalState());
  KeyringService& service = *brave_wallet_service.keyring_service();
  BitcoinTestRpcServer bitcoin_test_rpc_server(
      brave_wallet_service.GetBitcoinWalletService());

  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

  std::vector<std::string> requested_addresses;
  set_sol_balance_callback(base::BindLambdaForTesting(
      [this, &requested_addresses](const std::string& address) -> std::string {
        requested_addresses.push_back(address);

        // 3rd and 10th have transactions.
        if (address == saved_addresses()[3] ||
            address == saved_addresses()[10]) {
          return R"({"jsonrpc":"2.0","id":"1","result": { "value": 1 }})";
        } else {
          return R"({"jsonrpc":"2.0","id":"1","result": { "value": 0 }})";
        }
      }));

  EXPECT_CALL(observer, AccountsChanged()).Times(2);  // Accounts 3 and 10.
  EXPECT_TRUE(RestoreWallet(&service, saved_mnemonic(), "brave1", false));
  observer.WaitAndVerify();
  task_environment_.RunUntilIdle();
  std::vector<mojom::AccountInfoPtr> account_infos =
      service.GetAccountInfosForKeyring(mojom::kSolanaKeyringId);
  EXPECT_EQ(account_infos.size(), 11u);
  for (size_t i = 0; i < account_infos.size(); ++i) {
    EXPECT_EQ(account_infos[i]->address, saved_addresses()[i]);
    EXPECT_EQ(account_infos[i]->name,
              "Solana Account " + base::NumberToString(i + 1));
  }
  // 20 attempts more after Account 10 is added.
  EXPECT_THAT(requested_addresses, ElementsAreArray(&saved_addresses()[1], 30));
}

TEST_F(KeyringServiceAccountDiscoveryUnitTest, FilAccountDiscovery) {
  PrepareAccounts(mojom::CoinType::FIL, mojom::kFilecoinKeyringId);

  BraveWalletService brave_wallet_service(
      shared_url_loader_factory(), TestBraveWalletServiceDelegate::Create(),
      GetPrefs(), GetLocalState());
  KeyringService& service = *brave_wallet_service.keyring_service();
  BitcoinTestRpcServer bitcoin_test_rpc_server(
      brave_wallet_service.GetBitcoinWalletService());

  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

  std::vector<std::string> requested_addresses;
  set_fil_balance_callback(base::BindLambdaForTesting(
      [this, &requested_addresses](const std::string& address) -> std::string {
        requested_addresses.push_back(address);

        // 2nd and 9 have transactions.
        if (address == saved_addresses()[2] ||
            address == saved_addresses()[9]) {
          return R"({"jsonrpc":"2.0","id":"1","result":"1"})";
        } else {
          return R"({"jsonrpc":"2.0","id":"1","result":"0"})";
        }
      }));

  EXPECT_CALL(observer, AccountsChanged()).Times(2);  // Accounts 3 and 10.
  EXPECT_TRUE(RestoreWallet(&service, saved_mnemonic(), "brave1", false));
  observer.WaitAndVerify();
  std::vector<mojom::AccountInfoPtr> account_infos =
      service.GetAccountInfosForKeyring(mojom::kFilecoinKeyringId);
  EXPECT_EQ(account_infos.size(), 10u);
  for (size_t i = 0; i < account_infos.size(); ++i) {
    EXPECT_EQ(account_infos[i]->address, saved_addresses()[i]);
    EXPECT_EQ(account_infos[i]->name,
              "Filecoin Account " + base::NumberToString(i + 1));
  }
  // 20 attempts more after Account 10 is added.
  EXPECT_THAT(requested_addresses, ElementsAreArray(&saved_addresses()[0], 30));
}

TEST_F(KeyringServiceUnitTest, BitcoinDiscovery) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletBitcoinFeature,
      {{features::kBitcoinTestnetDiscovery.name, "true"}});

  BraveWalletService brave_wallet_service(
      shared_url_loader_factory(), TestBraveWalletServiceDelegate::Create(),
      GetPrefs(), GetLocalState());
  KeyringService& service = *brave_wallet_service.keyring_service();
  BitcoinTestRpcServer bitcoin_test_rpc_server(
      brave_wallet_service.GetBitcoinWalletService());

  bitcoin_test_rpc_server.SetUpBitcoinRpc(std::nullopt, std::nullopt);
  BitcoinHDKeyring keyring_84(*MnemonicToSeed(kMnemonicAbandonAbandon), false);
  BitcoinHDKeyring keyring_84_test(*MnemonicToSeed(kMnemonicAbandonAbandon),
                                   true);

  // Account 0
  bitcoin_test_rpc_server.AddTransactedAddress(
      keyring_84.GetAddress(0, {0, 5}));

  // Account 1
  bitcoin_test_rpc_server.AddTransactedAddress(
      keyring_84.GetAddress(1, {0, 10}));
  bitcoin_test_rpc_server.AddTransactedAddress(
      keyring_84.GetAddress(1, {1, 7}));

  // Account 3 - not created as there is no Account 2 discovered.
  bitcoin_test_rpc_server.AddTransactedAddress(
      keyring_84.GetAddress(3, {0, 10}));

  // Testnet Account 0
  bitcoin_test_rpc_server.AddTransactedAddress(
      keyring_84_test.GetAddress(0, {0, 15}));

  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

  EXPECT_CALL(observer, AccountsAdded(_)).Times(5);
  EXPECT_TRUE(RestoreWallet(&service, kMnemonicAbandonAbandon,
                            kTestWalletPassword, false));
  observer.WaitAndVerify();
  auto& account_infos = service.GetAllAccountInfos();
  ASSERT_EQ(account_infos.size(), 5u);
  EXPECT_EQ(account_infos[0]->account_id->coin, mojom::CoinType::ETH);
  EXPECT_EQ(account_infos[1]->account_id->coin, mojom::CoinType::SOL);

  EXPECT_EQ(account_infos[2]->account_id->coin, mojom::CoinType::BTC);
  EXPECT_EQ(account_infos[2]->account_id->keyring_id,
            mojom::KeyringId::kBitcoin84);
  EXPECT_EQ(account_infos[2]->name, "Bitcoin Account 1");
  EXPECT_EQ(service.GetBitcoinAccountInfo(account_infos[2]->account_id)
                ->next_receive_address->key_id->index,
            6u);
  EXPECT_EQ(service.GetBitcoinAccountInfo(account_infos[2]->account_id)
                ->next_change_address->key_id->index,
            0u);

  EXPECT_EQ(account_infos[3]->account_id->coin, mojom::CoinType::BTC);
  EXPECT_EQ(account_infos[3]->account_id->keyring_id,
            mojom::KeyringId::kBitcoin84);
  EXPECT_EQ(account_infos[3]->name, "Bitcoin Account 2");
  EXPECT_EQ(service.GetBitcoinAccountInfo(account_infos[3]->account_id)
                ->next_receive_address->key_id->index,
            11u);
  EXPECT_EQ(service.GetBitcoinAccountInfo(account_infos[3]->account_id)
                ->next_change_address->key_id->index,
            8u);

  EXPECT_EQ(account_infos[4]->account_id->coin, mojom::CoinType::BTC);
  EXPECT_EQ(account_infos[4]->account_id->keyring_id,
            mojom::KeyringId::kBitcoin84Testnet);
  EXPECT_EQ(account_infos[4]->name, "Bitcoin Testnet Account 1");
  EXPECT_EQ(service.GetBitcoinAccountInfo(account_infos[4]->account_id)
                ->next_receive_address->key_id->index,
            16u);
  EXPECT_EQ(service.GetBitcoinAccountInfo(account_infos[4]->account_id)
                ->next_change_address->key_id->index,
            0u);
}

TEST_F(KeyringServiceAccountDiscoveryUnitTest, StopsOnError) {
  PrepareAccounts(mojom::CoinType::ETH, mojom::kDefaultKeyringId);

  BraveWalletService brave_wallet_service(
      shared_url_loader_factory(), TestBraveWalletServiceDelegate::Create(),
      GetPrefs(), GetLocalState());
  KeyringService& service = *brave_wallet_service.keyring_service();
  BitcoinTestRpcServer bitcoin_test_rpc_server(
      brave_wallet_service.GetBitcoinWalletService());

  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

  std::vector<std::string> requested_addresses;
  set_eth_transaction_count_callback(base::BindLambdaForTesting(
      [this, &requested_addresses](const std::string& address) -> std::string {
        requested_addresses.push_back(address);

        // 3rd account has transactions. Checking 8th account ends with network
        // error.
        if (address == saved_addresses()[3]) {
          return R"({"jsonrpc":"2.0","id":"1","result":"0x1"})";
        } else if (address == saved_addresses()[8]) {
          return "error";
        } else {
          return R"({"jsonrpc":"2.0","id":"1","result":"0x0"})";
        }
      }));

  EXPECT_CALL(observer, AccountsChanged()).Times(1);  // Account 3.
  EXPECT_TRUE(RestoreWallet(&service, saved_mnemonic(), "brave1", false));
  observer.WaitAndVerify();
  std::vector<mojom::AccountInfoPtr> account_infos =
      service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
  EXPECT_EQ(account_infos.size(), 4u);
  for (size_t i = 0; i < account_infos.size(); ++i) {
    EXPECT_EQ(account_infos[i]->address, saved_addresses()[i]);
    EXPECT_EQ(account_infos[i]->name, "Account " + base::NumberToString(i + 1));
  }
  // Stopped after 8th attempt.
  EXPECT_THAT(requested_addresses, ElementsAreArray(&saved_addresses()[1], 8));
}

TEST_F(KeyringServiceAccountDiscoveryUnitTest, ManuallyAddAccount) {
  PrepareAccounts(mojom::CoinType::ETH, mojom::kDefaultKeyringId);

  BraveWalletService brave_wallet_service(
      shared_url_loader_factory(), TestBraveWalletServiceDelegate::Create(),
      GetPrefs(), GetLocalState());
  KeyringService& service = *brave_wallet_service.keyring_service();
  BitcoinTestRpcServer bitcoin_test_rpc_server(
      brave_wallet_service.GetBitcoinWalletService());

  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

  std::vector<std::string> requested_addresses;
  set_eth_transaction_count_callback(base::BindLambdaForTesting(
      [this, &service,
       &requested_addresses](const std::string& address) -> std::string {
        requested_addresses.push_back(address);

        // Manually add account while checking 4th account. Will be added
        // instead of Account 2.
        if (address == saved_addresses()[4]) {
          EXPECT_TRUE(AddAccount(&service, mojom::CoinType::ETH,
                                 mojom::kDefaultKeyringId, "Added Account 2"));
        }

        // Manually add account while checking 6th account. Will be added
        // instead of Account 6.
        if (address == saved_addresses()[6]) {
          EXPECT_TRUE(AddAccount(&service, mojom::CoinType::ETH,
                                 mojom::kDefaultKeyringId, "Added Account 7"));
        }

        // 5th and 6th accounts have transactions.
        if (address == saved_addresses()[5] ||
            address == saved_addresses()[6]) {
          return R"({"jsonrpc":"2.0","id":"1","result":"0x1"})";
        } else {
          return R"({"jsonrpc":"2.0","id":"1","result":"0x0"})";
        }
      }));

  EXPECT_CALL(observer, AccountsChanged())
      .Times(3);  // Two accounts added manually, one by discovery.
  EXPECT_TRUE(RestoreWallet(&service, saved_mnemonic(), "brave1", false));
  task_environment_.RunUntilIdle();
  std::vector<mojom::AccountInfoPtr> account_infos =
      service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
  EXPECT_EQ(account_infos.size(), 7u);
  for (size_t i = 0; i < account_infos.size(); ++i) {
    EXPECT_EQ(account_infos[i]->address, saved_addresses()[i]);
    if (i == 1u) {
      EXPECT_EQ(account_infos[i]->name, "Added Account 2");
    } else if (i == 6u) {
      EXPECT_EQ(account_infos[i]->name, "Added Account 7");
    } else {
      EXPECT_EQ(account_infos[i]->name,
                "Account " + base::NumberToString(i + 1));
    }
  }

  // 20 attempts more after Account 6 is added.
  EXPECT_THAT(requested_addresses, ElementsAreArray(&saved_addresses()[1], 26));
}

TEST_F(KeyringServiceAccountDiscoveryUnitTest, RestoreWalletTwice) {
  PrepareAccounts(mojom::CoinType::ETH, mojom::kDefaultKeyringId);

  BraveWalletService brave_wallet_service(
      shared_url_loader_factory(), TestBraveWalletServiceDelegate::Create(),
      GetPrefs(), GetLocalState());
  KeyringService& service = *brave_wallet_service.keyring_service();
  BitcoinTestRpcServer bitcoin_test_rpc_server(
      brave_wallet_service.GetBitcoinWalletService());

  std::vector<std::string> requested_addresses;
  bool first_restore = true;
  base::RunLoop run_loop;
  set_eth_transaction_count_callback(base::BindLambdaForTesting(
      [&, this](const std::string& address) -> std::string {
        requested_addresses.push_back(address);

        // Run RestoreWallet again after processing 5th address.
        if (first_restore && address == saved_addresses()[5]) {
          run_loop.Quit();
        }

        // 3rd and 10th have transactions.
        if (address == saved_addresses()[3] ||
            address == saved_addresses()[10]) {
          return R"({"jsonrpc":"2.0","id":"1","result":"0x1"})";
        } else {
          return R"({"jsonrpc":"2.0","id":"1","result":"0x0"})";
        }
      }));

  EXPECT_TRUE(RestoreWallet(&service, saved_mnemonic(), "brave1", false));
  run_loop.Run();
  // First restore: 5 attempts.
  EXPECT_THAT(requested_addresses, ElementsAreArray(&saved_addresses()[1], 5));
  requested_addresses.clear();

  first_restore = false;
  service.Reset();

  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

  EXPECT_CALL(observer, AccountsChanged()).Times(2);  // Accounts 3 and 10.
  EXPECT_TRUE(RestoreWallet(&service, saved_mnemonic(), "brave1", false));
  observer.WaitAndVerify();

  std::vector<mojom::AccountInfoPtr> account_infos =
      service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
  EXPECT_EQ(account_infos.size(), 11u);
  for (size_t i = 0; i < account_infos.size(); ++i) {
    EXPECT_EQ(account_infos[i]->address, saved_addresses()[i]);
    EXPECT_EQ(account_infos[i]->name, "Account " + base::NumberToString(i + 1));
  }
  // Second restore: 20 attempts more after Account 10 is added.
  EXPECT_THAT(requested_addresses, ElementsAreArray(&saved_addresses()[1], 30));
}

TEST_F(KeyringServiceUnitTest, AccountsAdded) {
  // Verifies AccountsAdded event is emitted as expected in AddAccount
  // CreateWallet, RestoreWallet, AddHardwareAccounts, and
  // ImportAccountForKeyring
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);

  std::vector<mojom::AccountInfoPtr> default_eth_account;
  default_eth_account.push_back(mojom::AccountInfo::New(
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kDerived,
                    "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db"),
      "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db", "Account 1", nullptr));
  std::vector<mojom::AccountInfoPtr> default_sol_account;
  default_sol_account.push_back(mojom::AccountInfo::New(
      MakeAccountId(mojom::CoinType::SOL, mojom::kSolanaKeyringId,
                    mojom::AccountKind::kDerived,
                    "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8"),
      "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8", "Solana Account 1",
      nullptr));

  // RestoreWallet
  EXPECT_CALL(observer,
              AccountsAdded(testing::Eq(std::ref(default_eth_account))));
  EXPECT_CALL(observer,
              AccountsAdded(testing::Eq(std::ref(default_sol_account))));

  RestoreWallet(&service, kMnemonicDivideCruise, kPasswordBrave, false);
  observer.WaitAndVerify();
  task_environment_.FastForwardBy(
      base::Minutes(kAssetDiscoveryMinutesPerRequest));

  // AddAccount ETH
  std::vector<mojom::AccountInfoPtr> added_eth_account;
  added_eth_account.push_back(mojom::AccountInfo::New(
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kDerived,
                    "0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0"),
      "0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0", "Account", nullptr));
  EXPECT_CALL(observer,
              AccountsAdded(testing::Eq(std::ref(added_eth_account))));
  ASSERT_TRUE(AddAccount(&service, mojom::CoinType::ETH,
                         mojom::kDefaultKeyringId, "Account"));
  observer.WaitAndVerify();

  task_environment_.FastForwardBy(
      base::Minutes(kAssetDiscoveryMinutesPerRequest));

  // AddAccount SOL
  std::vector<mojom::AccountInfoPtr> added_sol_account;
  added_sol_account.push_back(mojom::AccountInfo::New(
      MakeAccountId(mojom::CoinType::SOL, mojom::kSolanaKeyringId,
                    mojom::AccountKind::kDerived,
                    "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV"),
      "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV", "Account", nullptr));
  EXPECT_CALL(observer,
              AccountsAdded(testing::Eq(std::ref(added_sol_account))));
  ASSERT_TRUE(AddAccount(&service, mojom::CoinType::SOL,
                         mojom::kSolanaKeyringId, "Account"));
  observer.WaitAndVerify();
  task_environment_.FastForwardBy(
      base::Minutes(kAssetDiscoveryMinutesPerRequest));

  // AddHardwareAccounts

  std::vector<mojom::HardwareWalletAccountPtr> hardware_accounts;
  hardware_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0x595a0583621FDe81A935021707e81343f75F9324", "m/44'/60'/1'/0/0",
      "name 1", mojom::HardwareVendor::kLedger, "device1",
      mojom::kDefaultKeyringId));
  std::vector<mojom::AccountInfoPtr> added_hw_account;
  added_hw_account.push_back(mojom::AccountInfo::New(
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kHardware,
                    "0x595a0583621FDe81A935021707e81343f75F9324"),
      "0x595a0583621FDe81A935021707e81343f75F9324", "name 1",
      mojom::HardwareInfo::New("m/44'/60'/1'/0/0",
                               mojom::HardwareVendor::kLedger, "device1")));
  EXPECT_CALL(observer, AccountsAdded(testing::Eq(std::ref(added_hw_account))));
  service.AddHardwareAccountsSync(std::move(hardware_accounts));
  observer.WaitAndVerify();
  task_environment_.FastForwardBy(
      base::Minutes(kAssetDiscoveryMinutesPerRequest));

  // ImportAccountForKeyring
  const std::string private_key_str =
      "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
  std::vector<uint8_t> private_key_bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(private_key_str, &private_key_bytes));
  std::vector<mojom::AccountInfoPtr> added_imported_account;
  added_imported_account.push_back(mojom::AccountInfo::New(
      MakeAccountId(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                    mojom::AccountKind::kImported,
                    "0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266"),
      "0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266", "Imported Account",
      nullptr));
  EXPECT_CALL(observer,
              AccountsAdded(testing::Eq(std::ref(added_imported_account))));
  ASSERT_TRUE(service.ImportAccountForKeyring(
      mojom::CoinType::ETH, mojom::kDefaultKeyringId, "Imported Account",
      private_key_bytes));
  observer.WaitAndVerify();
}

#if !defined(OFFICIAL_BUILD)
TEST_F(KeyringServiceUnitTest, DevWalletPassword) {
  base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();

  // Setup wallet.
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    CreateWallet(&service, "some_password");
  }

  // Locked on start by default.
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    EXPECT_TRUE(service.IsLockedSync());
  }

  // Unlocked on start with right password.
  {
    cmdline->AppendSwitchASCII(switches::kDevWalletPassword, "some_password");
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    EXPECT_FALSE(service.IsLockedSync());
    cmdline->RemoveSwitch(switches::kDevWalletPassword);
  }

  // Locked on start with wrong password.
  {
    cmdline->AppendSwitchASCII(switches::kDevWalletPassword, "wrong_password");
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    EXPECT_TRUE(service.IsLockedSync());
    cmdline->RemoveSwitch(switches::kDevWalletPassword);
  }
}
#endif  // !defined(OFFICIAL_BUILD)

TEST_F(KeyringServiceUnitTest, GetBitcoinAddresses) {
  // TODO(apaymyshev): update existing tests above to also cover Bitcoin
  // keyring.

  base::test::ScopedFeatureList feature_list{
      features::kBraveWalletBitcoinFeature};

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  // https://github.com/bitcoin/bips/blob/master/bip-0084.mediawiki#test-vectors
  ASSERT_TRUE(RestoreWallet(&service, kMnemonicAbandonAbandon, "brave", false));

  EXPECT_THAT(GetAccountUtils(&service).AllBtcAccounts(), testing::IsEmpty());

  auto added_account = AddAccount(&service, mojom::CoinType::BTC,
                                  mojom::KeyringId::kBitcoin84, "Btc Acc");

  EXPECT_THAT(GetAccountUtils(&service).AllBtcAccounts(), testing::SizeIs(1u));
  auto btc_acc = GetAccountUtils(&service).AllBtcAccounts()[0]->Clone();
  EXPECT_EQ(btc_acc, added_account);
  EXPECT_EQ(btc_acc->address, "");
  EXPECT_EQ(btc_acc->name, "Btc Acc");
  EXPECT_EQ(btc_acc->account_id->kind, mojom::AccountKind::kDerived);
  EXPECT_EQ(btc_acc->account_id->coin, mojom::CoinType::BTC);
  EXPECT_EQ(btc_acc->account_id->keyring_id, mojom::KeyringId::kBitcoin84);

  auto addresses = service.GetBitcoinAddresses(btc_acc->account_id);
  ASSERT_TRUE(addresses);
  ASSERT_EQ(addresses->size(), 2u);  // 1 receive + 1 change for fresh account.
  EXPECT_EQ(addresses->at(0), mojom::BitcoinAddress::New(
                                  "bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu",
                                  mojom::BitcoinKeyId::New(0, 0)));
  EXPECT_EQ(addresses->at(1), mojom::BitcoinAddress::New(
                                  "bc1q8c6fshw2dlwun7ekn9qwf37cu2rn755upcp6el",
                                  mojom::BitcoinKeyId::New(1, 0)));

  service.UpdateNextUnusedAddressForBitcoinAccount(btc_acc->account_id, 1,
                                                   std::nullopt);
  addresses = service.GetBitcoinAddresses(btc_acc->account_id);
  ASSERT_EQ(addresses->size(), 3u);  // +1 receive .
  EXPECT_EQ(addresses->at(0), mojom::BitcoinAddress::New(
                                  "bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu",
                                  mojom::BitcoinKeyId::New(0, 0)));
  EXPECT_EQ(addresses->at(1), mojom::BitcoinAddress::New(
                                  "bc1qnjg0jd8228aq7egyzacy8cys3knf9xvrerkf9g",
                                  mojom::BitcoinKeyId::New(0, 1)));
  EXPECT_EQ(addresses->at(2), mojom::BitcoinAddress::New(
                                  "bc1q8c6fshw2dlwun7ekn9qwf37cu2rn755upcp6el",
                                  mojom::BitcoinKeyId::New(1, 0)));

  service.UpdateNextUnusedAddressForBitcoinAccount(btc_acc->account_id,
                                                   std::nullopt, 1);
  addresses = service.GetBitcoinAddresses(btc_acc->account_id);
  ASSERT_EQ(addresses->size(), 4u);  // + 1 change.
  EXPECT_EQ(addresses->at(0), mojom::BitcoinAddress::New(
                                  "bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu",
                                  mojom::BitcoinKeyId::New(0, 0)));
  EXPECT_EQ(addresses->at(1), mojom::BitcoinAddress::New(
                                  "bc1qnjg0jd8228aq7egyzacy8cys3knf9xvrerkf9g",
                                  mojom::BitcoinKeyId::New(0, 1)));
  EXPECT_EQ(addresses->at(2), mojom::BitcoinAddress::New(
                                  "bc1q8c6fshw2dlwun7ekn9qwf37cu2rn755upcp6el",
                                  mojom::BitcoinKeyId::New(1, 0)));
  EXPECT_EQ(addresses->at(3), mojom::BitcoinAddress::New(
                                  "bc1qggnasd834t54yulsep6fta8lpjekv4zj6gv5rf",
                                  mojom::BitcoinKeyId::New(1, 1)));
  service.UpdateNextUnusedAddressForBitcoinAccount(btc_acc->account_id, 5, 5);
  addresses = service.GetBitcoinAddresses(btc_acc->account_id);
  ASSERT_EQ(addresses->size(), 12u);  // 6 receive + 6 change.
  EXPECT_EQ(addresses->at(6), mojom::BitcoinAddress::New(
                                  "bc1q8c6fshw2dlwun7ekn9qwf37cu2rn755upcp6el",
                                  mojom::BitcoinKeyId::New(1, 0)));
}

TEST_F(KeyringServiceUnitTest, UpdateNextUnusedAddressForBitcoinAccount) {
  base::test::ScopedFeatureList feature_list{
      features::kBraveWalletBitcoinFeature};

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  ASSERT_TRUE(RestoreWallet(&service, kMnemonicAbandonAbandon, "brave", false));
  auto btc_acc = GetAccountUtils(&service).EnsureBtcAccount(0);
  EXPECT_EQ(mojom::BitcoinKeyId::New(0, 0),
            service.GetBitcoinAccountInfo(btc_acc->account_id)
                ->next_receive_address->key_id);
  EXPECT_EQ(mojom::BitcoinKeyId::New(1, 0),
            service.GetBitcoinAccountInfo(btc_acc->account_id)
                ->next_change_address->key_id);

  NiceMock<TestKeyringServiceObserver> observer(service, task_environment_);
  EXPECT_CALL(observer, AccountsChanged());
  service.UpdateNextUnusedAddressForBitcoinAccount(btc_acc->account_id, 7,
                                                   std::nullopt);
  observer.WaitAndVerify();
  EXPECT_EQ(mojom::BitcoinKeyId::New(0, 7),
            service.GetBitcoinAccountInfo(btc_acc->account_id)
                ->next_receive_address->key_id);
  EXPECT_EQ(mojom::BitcoinKeyId::New(1, 0),
            service.GetBitcoinAccountInfo(btc_acc->account_id)
                ->next_change_address->key_id);
  EXPECT_CALL(observer, AccountsChanged());
  service.UpdateNextUnusedAddressForBitcoinAccount(btc_acc->account_id,
                                                   std::nullopt, 9);
  observer.WaitAndVerify();
  EXPECT_EQ(mojom::BitcoinKeyId::New(0, 7),
            service.GetBitcoinAccountInfo(btc_acc->account_id)
                ->next_receive_address->key_id);
  EXPECT_EQ(mojom::BitcoinKeyId::New(1, 9),
            service.GetBitcoinAccountInfo(btc_acc->account_id)
                ->next_change_address->key_id);
}

TEST_F(KeyringServiceUnitTest, GetBitcoinAccountInfo) {
  base::test::ScopedFeatureList feature_list{
      features::kBraveWalletBitcoinFeature};

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  ASSERT_TRUE(RestoreWallet(&service, kMnemonicAbandonAbandon, "brave", false));
  auto btc_acc = GetAccountUtils(&service).EnsureBtcAccount(0);

  service.UpdateNextUnusedAddressForBitcoinAccount(btc_acc->account_id, 7, 9);
  EXPECT_EQ(
      mojom::BitcoinAddress::New("bc1qhxgzmkmwvrlwvlfn4qe57lx2qdfg8phycnsarn",
                                 mojom::BitcoinKeyId::New(0, 7)),
      service.GetBitcoinAccountInfo(btc_acc->account_id)->next_receive_address);
  EXPECT_EQ(
      mojom::BitcoinAddress::New("bc1qwmrhe0ry500ptrhfwcvntglk8y0affaauvcp46",
                                 mojom::BitcoinKeyId::New(1, 9)),
      service.GetBitcoinAccountInfo(btc_acc->account_id)->next_change_address);
}

TEST_F(KeyringServiceUnitTest, GetBitcoinAddress) {
  base::test::ScopedFeatureList feature_list{
      features::kBraveWalletBitcoinFeature};

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  ASSERT_TRUE(RestoreWallet(&service, kMnemonicAbandonAbandon, "brave", false));
  auto btc_acc = GetAccountUtils(&service).EnsureBtcAccount(0);

  EXPECT_EQ(
      mojom::BitcoinAddress::New("bc1qhxgzmkmwvrlwvlfn4qe57lx2qdfg8phycnsarn",
                                 mojom::BitcoinKeyId::New(0, 7)),
      service.GetBitcoinAddress(btc_acc->account_id,
                                mojom::BitcoinKeyId::New(0, 7)));
  EXPECT_EQ(
      mojom::BitcoinAddress::New("bc1qwmrhe0ry500ptrhfwcvntglk8y0affaauvcp46",
                                 mojom::BitcoinKeyId::New(1, 9)),
      service.GetBitcoinAddress(btc_acc->account_id,
                                mojom::BitcoinKeyId::New(1, 9)));
}

TEST_F(KeyringServiceUnitTest, GetBitcoinAccountDiscoveryAddress) {
  base::test::ScopedFeatureList feature_list{
      features::kBraveWalletBitcoinFeature};

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  AccountUtils(&service).CreateWallet(kMnemonicAbandonAbandon,
                                      kTestWalletPassword);

  EXPECT_EQ(
      mojom::BitcoinAddress::New("bc1qhxgzmkmwvrlwvlfn4qe57lx2qdfg8phycnsarn",
                                 mojom::BitcoinKeyId::New(0, 7)),
      service.GetBitcoinAccountDiscoveryAddress(
          mojom::KeyringId::kBitcoin84, 0, mojom::BitcoinKeyId::New(0, 7)));
  EXPECT_EQ(
      mojom::BitcoinAddress::New("bc1qwmrhe0ry500ptrhfwcvntglk8y0affaauvcp46",
                                 mojom::BitcoinKeyId::New(1, 9)),
      service.GetBitcoinAccountDiscoveryAddress(
          mojom::KeyringId::kBitcoin84, 0, mojom::BitcoinKeyId::New(1, 9)));

  EXPECT_EQ(
      mojom::BitcoinAddress::New("bc1q7upazc2k8dwu5l84arl06zm7sjl0xlqgk6dey6",
                                 mojom::BitcoinKeyId::New(0, 7)),
      service.GetBitcoinAccountDiscoveryAddress(
          mojom::KeyringId::kBitcoin84, 10, mojom::BitcoinKeyId::New(0, 7)));
  EXPECT_EQ(
      mojom::BitcoinAddress::New("tb1qe90pd25ax8yjae79je9gfeuwtvje80yx0rt6ct",
                                 mojom::BitcoinKeyId::New(1, 9)),
      service.GetBitcoinAccountDiscoveryAddress(
          mojom::KeyringId::kBitcoin84Testnet, 100,
          mojom::BitcoinKeyId::New(1, 9)));
}

TEST_F(KeyringServiceUnitTest, GetBitcoinPubkey) {
  base::test::ScopedFeatureList feature_list{
      features::kBraveWalletBitcoinFeature};

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  ASSERT_TRUE(RestoreWallet(&service, kMnemonicAbandonAbandon, "brave", false));
  auto btc_acc = GetAccountUtils(&service).EnsureBtcAccount(0);

  EXPECT_EQ(
      "0275CEEC11410AC8D730ACD0F99E50A530E1C28B1CF89133EC5F798FB675BDDC8E",
      base::HexEncode(*service.GetBitcoinPubkey(
          btc_acc->account_id, mojom::BitcoinKeyId::New(0, 7))));
  EXPECT_EQ(
      "033712907E0A8F4793203935787397FDC81407B116D42626ABF142099783B964B1",
      base::HexEncode(*service.GetBitcoinPubkey(
          btc_acc->account_id, mojom::BitcoinKeyId::New(1, 9))));
}

TEST_F(KeyringServiceUnitTest, SignMessageByBitcoinKeyring) {
  base::test::ScopedFeatureList feature_list{
      features::kBraveWalletBitcoinFeature};

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  ASSERT_TRUE(RestoreWallet(&service, kMnemonicAbandonAbandon, "brave", false));
  auto btc_acc = GetAccountUtils(&service).EnsureBtcAccount(0);
  std::array<uint8_t, 32> message;
  message.fill('1');

  EXPECT_EQ(
      "304402207901E3C494F6251CAB3EAEC330B1FCF8DA8B791B3BE8DAFB3B4432636BC0AFD6"
      "02206A3506A8AF666D9E6FE00FF2DBC80176A405390D84B501B8E740CB20BC191A85",
      base::HexEncode(*service.SignMessageByBitcoinKeyring(
          btc_acc->account_id, mojom::BitcoinKeyId::New(0, 3), message)));
  EXPECT_EQ(
      "304402203794D7DAA283D56FFD09644649063D3BD1A6CED6F19BD6AC69D035B9F94629EF"
      "02202E78E2CBA60B164FF25F5FE71A2649B5788CABE6364417AC1AE7733EB7FFA566",
      base::HexEncode(*service.SignMessageByBitcoinKeyring(
          btc_acc->account_id, mojom::BitcoinKeyId::New(1, 7), message)));
}

TEST_F(KeyringServiceUnitTest, MigrateSelectedAccount) {
  auto service = std::make_unique<KeyringService>(json_rpc_service(),
                                                  GetPrefs(), GetLocalState());

  ASSERT_TRUE(
      RestoreWallet(service.get(), kMnemonicDivideCruise, "brave", false));

  auto eth_acc = AddAccount(service.get(), mojom::CoinType::ETH,
                            mojom::KeyringId::kDefault, "ETH 1");
  auto sol_acc = AddAccount(service.get(), mojom::CoinType::SOL,
                            mojom::KeyringId::kSolana, "SOL 1");
  auto fil_acc = AddAccount(service.get(), mojom::CoinType::FIL,
                            mojom::KeyringId::kFilecoin, "FIL 1");
  service.reset();

  // Setup legacy selected account prefs.
  GetPrefs()->ClearPref(kBraveWalletSelectedWalletAccount);
  GetPrefs()->ClearPref(kBraveWalletSelectedSolDappAccount);
  GetPrefs()->ClearPref(kBraveWalletSelectedEthDappAccount);
  GetPrefs()->SetInteger(kBraveWalletSelectedCoinDeprecated,
                         static_cast<int>(mojom::CoinType::FIL));
  auto keyrings = GetPrefs()->GetDict(kBraveWalletKeyrings).Clone();
  keyrings.SetByDottedPath("default.selected_account", eth_acc->address);
  keyrings.SetByDottedPath("solana.selected_account", sol_acc->address);
  keyrings.SetByDottedPath("filecoin.selected_account", fil_acc->address);
  GetPrefs()->SetDict(kBraveWalletKeyrings, std::move(keyrings));

  // Instantiate service, migration should happen.
  service = std::make_unique<KeyringService>(json_rpc_service(), GetPrefs(),
                                             GetLocalState());

  // Legacy prefs are missing.
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletSelectedCoinDeprecated));
  keyrings = GetPrefs()->GetDict(kBraveWalletKeyrings).Clone();
  EXPECT_FALSE(keyrings.FindByDottedPath("default.selected_account"));
  EXPECT_FALSE(keyrings.FindByDottedPath("solana.selected_account"));
  EXPECT_FALSE(keyrings.FindByDottedPath("filecoin.selected_account"));

  auto all_accounts = service->GetAllAccountsSync();
  EXPECT_EQ(all_accounts->eth_dapp_selected_account, eth_acc);
  EXPECT_EQ(all_accounts->sol_dapp_selected_account, sol_acc);
  EXPECT_EQ(all_accounts->selected_account, fil_acc);
}

// Generated using https://github.com/zcash/zcash-test-vectors
TEST_F(KeyringServiceUnitTest, GetOrchardRawBytes) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"}});

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  ASSERT_TRUE(RestoreWallet(&service, kMnemonicAbandonAbandon, "brave", false));

  {
    auto actual =
        service
            .GetOrchardRawBytes(
                MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                        mojom::KeyringId::kZCashMainnet,
                                        mojom::AccountKind::kDerived, 2),
                mojom::ZCashKeyId::New(2, 0 /* external */, 0))
            .value();
    auto expected = std::array<uint8_t, kOrchardRawBytesSize>(
        {0x9d, 0x61, 0x35, 0xcc, 0x32, 0xd6, 0x79, 0x4d, 0x46, 0x70, 0x7f,
         0xe4, 0x10, 0x65, 0x9a, 0x46, 0xe5, 0x39, 0x6a, 0xae, 0x2b, 0x8a,
         0xc6, 0xc6, 0x7c, 0xb0, 0x13, 0x7e, 0x37, 0xb5, 0xb7, 0x98, 0x5b,
         0xc2, 0x86, 0xe1, 0xc7, 0xb1, 0xda, 0x40, 0x4a, 0x86, 0x35});
    EXPECT_EQ(expected, actual);
  }

  {
    auto actual =
        service
            .GetOrchardRawBytes(
                MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                        mojom::KeyringId::kZCashMainnet,
                                        mojom::AccountKind::kDerived, 1),
                mojom::ZCashKeyId::New(1, 0 /* external */, 3))
            .value();
    auto expected = std::array<uint8_t, kOrchardRawBytesSize>(
        {0x0f, 0xd0, 0x19, 0x37, 0x53, 0x52, 0xc9, 0x1c, 0xd1, 0x3e, 0xfb,
         0x0a, 0x5c, 0x1e, 0x0f, 0x75, 0xd1, 0x6d, 0x31, 0x2a, 0x76, 0x74,
         0x4f, 0xcf, 0x66, 0x74, 0x23, 0x5a, 0x26, 0x33, 0x76, 0x70, 0xc4,
         0xcc, 0x15, 0x0d, 0xb2, 0x22, 0x2d, 0xaf, 0x3b, 0xc1, 0x02});
    EXPECT_EQ(expected, actual);
  }

  {
    auto actual =
        service
            .GetOrchardRawBytes(
                MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                        mojom::KeyringId::kZCashMainnet,
                                        mojom::AccountKind::kDerived, 4),
                mojom::ZCashKeyId::New(4, 0 /* external */, 0))
            .value();
    auto expected = std::array<uint8_t, kOrchardRawBytesSize>(
        {0xfa, 0x12, 0x63, 0xd3, 0x8f, 0x3f, 0x10, 0x19, 0x60, 0x5e, 0xb7,
         0xe2, 0x7c, 0xf7, 0x4c, 0x03, 0x03, 0x41, 0x14, 0xf2, 0x71, 0x9f,
         0x71, 0xdc, 0x61, 0xb6, 0x52, 0xe3, 0x04, 0x12, 0x3f, 0x34, 0x78,
         0x75, 0x02, 0x25, 0x78, 0x2a, 0x4a, 0x2b, 0x80, 0x00, 0xab});
    EXPECT_EQ(expected, actual);
  }
}

TEST_F(KeyringServiceUnitTest, GetOrchardRawBytes_ZCashDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(features::kBraveWalletZCashFeature);

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(RestoreWallet(&service, kMnemonicAbandonAbandon, "brave", false));

  EXPECT_FALSE(service.GetOrchardRawBytes(
      MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                              mojom::KeyringId::kZCashMainnet,
                              mojom::AccountKind::kDerived, 1),
      mojom::ZCashKeyId::New(1, 0 /* external */, 3)));
}

}  // namespace brave_wallet
