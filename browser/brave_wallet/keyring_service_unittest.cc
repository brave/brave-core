/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/keyring_service.h"

#include <utility>

#include "base/base64.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/ranges/algorithm.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/values_test_util.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/fil_transaction.h"
#include "brave/components/brave_wallet/browser/filecoin_keyring.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/switches.h"
#include "build/build_config.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

using ::testing::ElementsAreArray;

namespace brave_wallet {

namespace {

const char kPasswordBrave[] = "brave";
const char kPasswordBrave123[] = "brave123";
const char kPasswordEncryptorSalt[] = "password_encryptor_salt";
const char kPasswordEncryptorNonce[] = "password_encryptor_nonce";
const char kEncryptedMnemonic[] = "encrypted_mnemonic";
const char kAccountMetas[] = "account_metas";
const char kHardwareAccounts[] = "hardware";
const char kImportedAccounts[] = "imported_accounts";
const char kAccountAddress[] = "account_address";
const char kEncryptedPrivateKey[] = "encrypted_private_key";

const char kMnemonic1[] =
    "divide cruise upon flag harsh carbon filter merit once advice bright "
    "drive";
const char kMnemonic2[] =
    "misery jeans response tiny nominee civil zoo strong correct taxi chimney "
    "goat";

base::Value::Dict GetHardwareKeyringValueForTesting() {
  base::Value::Dict dict;
  dict.SetByDottedPath("hardware.A1.account_metas.0x111.account_name", "test1");
  dict.SetByDottedPath("hardware.A1.account_metas.0x111.derivation_path",
                       "path1");
  dict.SetByDottedPath("hardware.A1.account_metas.0x111.hardware_vendor",
                       "vendor1");
  dict.SetByDottedPath("hardware.B2.account_metas.0x222.account_name", "test2");
  dict.SetByDottedPath("hardware.B2.account_metas.0x222.derivation_path",
                       "path2");
  dict.SetByDottedPath("hardware.B2.account_metas.0x222.hardware_vendor",
                       "vendor2");
  return dict;
}

struct ImportData {
  const char* network;
  const char* name;
  const char* import_payload;
  const char* address;
  const char* private_key;
};

}  // namespace

class TestKeyringServiceObserver
    : public brave_wallet::mojom::KeyringServiceObserver {
 public:
  TestKeyringServiceObserver() = default;

  void AutoLockMinutesChanged() override {
    auto_lock_minutes_changed_fired_ = true;
  }

  // TODO(bbondy): We should be testing all of these observer events
  void KeyringCreated(const std::string& keyring_id) override {
    ASSERT_FALSE(keyring_id.empty());
    keyring_created_.insert(keyring_id);
  }
  void KeyringRestored(const std::string& keyring_id) override {
    ASSERT_FALSE(keyring_id.empty());
    keyring_restored_.insert(keyring_id);
  }
  void KeyringReset() override { keyring_reset_fired_ = true; }
  void Locked() override { locked_fired_ = true; }
  void Unlocked() override { unlocked_fired_ = true; }
  void BackedUp() override {}

  void SelectedAccountChanged(mojom::CoinType coin) override {
    selected_account_change_fired_.insert(coin);
  }

  void AccountsChanged() override { accounts_changed_fired_count_++; }
  void AccountsAdded(mojom::CoinType coin,
                     const std::vector<std::string>& addresses) override {
    addresses_added_.push_back(addresses);
  }
  bool AutoLockMinutesChangedFired() {
    return auto_lock_minutes_changed_fired_;
  }
  bool SelectedAccountChangedFired(mojom::CoinType coin) {
    return selected_account_change_fired_.contains(coin);
  }
  bool AccountsChangedFired() { return accounts_changed_fired_count_ > 0; }
  int AccountsChangedFiredCount() { return accounts_changed_fired_count_; }
  bool KeyringResetFired() { return keyring_reset_fired_; }
  bool LockedFired() { return locked_fired_; }
  bool UnlockedFired() { return unlocked_fired_; }
  bool IsKeyringCreated(const std::string& keyring_id) {
    return keyring_created_.contains(keyring_id);
  }
  bool IsKeyringRestored(const std::string& keyring_id) {
    return keyring_restored_.contains(keyring_id);
  }

  mojo::PendingRemote<brave_wallet::mojom::KeyringServiceObserver>
  GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }
  void ExpectAddressesAddedEq(
      const std::vector<std::vector<std::string>> expected_addresses) {
    EXPECT_EQ(expected_addresses, addresses_added_);
    addresses_added_.clear();
  }

  void Reset() {
    auto_lock_minutes_changed_fired_ = false;
    accounts_changed_fired_count_ = 0;
    keyring_reset_fired_ = false;
    locked_fired_ = false;
    unlocked_fired_ = false;
    selected_account_change_fired_.clear();
    keyring_created_.clear();
    keyring_restored_.clear();
    addresses_added_.clear();
  }

 private:
  std::vector<std::vector<std::string>> addresses_added_;
  bool auto_lock_minutes_changed_fired_ = false;
  int accounts_changed_fired_count_ = 0;
  bool keyring_reset_fired_ = false;
  bool locked_fired_ = false;
  bool unlocked_fired_ = false;
  base::flat_set<mojom::CoinType> selected_account_change_fired_;
  base::flat_set<std::string> keyring_created_;
  base::flat_set<std::string> keyring_restored_;
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      observer_receiver_{this};
};

class KeyringServiceUnitTest : public testing::Test {
 public:
  KeyringServiceUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~KeyringServiceUnitTest() override = default;

 protected:
  void SetUp() override {
    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());
    RegisterLocalState(local_state_.registry());
    builder.SetPrefService(std::move(prefs));
    profile_ = builder.Build();
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    json_rpc_service_ =
        JsonRpcServiceFactory::GetServiceForContext(browser_context());
    json_rpc_service_->SetAPIRequestHelperForTesting(
        shared_url_loader_factory_);
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

  PrefService* GetPrefs() { return profile_->GetPrefs(); }

  PrefService* GetLocalState() { return &local_state_; }

  content::BrowserContext* browser_context() { return profile_.get(); }

  JsonRpcService* json_rpc_service() { return json_rpc_service_; }

  network::TestURLLoaderFactory& url_loader_factory() {
    return url_loader_factory_;
  }

  bool HasPrefForKeyring(const std::string& key, const std::string& id) {
    return KeyringService::HasPrefForKeyring(*GetPrefs(), key, id);
  }

  std::string GetStringPrefForKeyring(const std::string& key,
                                      const std::string& id) {
    const base::Value* value =
        KeyringService::GetPrefForKeyring(*GetPrefs(), key, id);
    if (!value) {
      return std::string();
    }

    return value->GetString();
  }

  static bool IsKeyringInfoEmpty(KeyringService* service,
                                 const std::string& keyring_id) {
    base::RunLoop run_loop;
    bool result = false;
    service->GetKeyringInfo(
        keyring_id,
        base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
          ASSERT_EQ(keyring_info->id, keyring_id);
          if (!keyring_info->is_keyring_created && keyring_info->is_locked &&
              !keyring_info->is_backed_up &&
              keyring_info->account_infos.empty()) {
            result = true;
          }
          run_loop.Quit();
        }));
    run_loop.Run();
    return result;
  }

  static std::string GetMnemonicForDefaultKeyring(const std::string& password,
                                                  KeyringService* service) {
    base::RunLoop run_loop;
    std::string mnemonic;
    service->GetMnemonicForDefaultKeyring(
        password, base::BindLambdaForTesting([&](const std::string& v) {
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

  static absl::optional<std::string> GetSelectedAccount(KeyringService* service,
                                                        mojom::CoinType coin) {
    absl::optional<std::string> account;
    base::RunLoop run_loop;
    service->GetSelectedAccount(
        coin,
        base::BindLambdaForTesting([&](const absl::optional<std::string>& v) {
          account = v;
          run_loop.Quit();
        }));
    run_loop.Run();
    return account;
  }

  static absl::optional<std::string> GetFilecoinSelectedAccount(
      KeyringService* service,
      const std::string& net) {
    absl::optional<std::string> account;
    base::RunLoop run_loop;
    service->GetFilecoinSelectedAccount(
        net,
        base::BindLambdaForTesting([&](const absl::optional<std::string>& v) {
          account = v;
          run_loop.Quit();
        }));
    run_loop.Run();
    return account;
  }

  static bool SetSelectedAccount(KeyringService* service,
                                 TestKeyringServiceObserver* observer,
                                 const std::string& account,
                                 mojom::CoinType coin) {
    EXPECT_FALSE(observer->SelectedAccountChangedFired(coin));
    bool success = false;
    base::RunLoop run_loop;
    service->SetSelectedAccount(account, coin,
                                base::BindLambdaForTesting([&](bool v) {
                                  success = v;
                                  run_loop.Quit();
                                }));
    run_loop.Run();
    base::RunLoop().RunUntilIdle();
    if (success) {
      EXPECT_TRUE(observer->SelectedAccountChangedFired(coin));
      observer->Reset();
    }
    EXPECT_FALSE(observer->SelectedAccountChangedFired(coin));
    return success;
  }

  static bool SetKeyringDerivedAccountName(KeyringService* service,
                                           const std::string& keyring_id,
                                           const std::string& address,
                                           const std::string& name) {
    bool success = false;
    base::RunLoop run_loop;
    service->SetKeyringDerivedAccountName(
        keyring_id, address, name, base::BindLambdaForTesting([&](bool v) {
          success = v;
          run_loop.Quit();
        }));
    run_loop.Run();
    return success;
  }

  static bool SetKeyringImportedAccountName(KeyringService* service,
                                            const std::string& keyring_id,
                                            const std::string& address,
                                            const std::string& name) {
    bool success = false;
    base::RunLoop run_loop;
    service->SetKeyringImportedAccountName(
        keyring_id, address, name, base::BindLambdaForTesting([&](bool v) {
          success = v;
          run_loop.Quit();
        }));
    run_loop.Run();
    return success;
  }

  static bool SetHardwareAccountName(KeyringService* service,
                                     const std::string& address,
                                     const std::string& name,
                                     mojom::CoinType coin) {
    bool success = false;
    base::RunLoop run_loop;
    service->SetHardwareAccountName(address, name, coin,
                                    base::BindLambdaForTesting([&](bool v) {
                                      success = v;
                                      run_loop.Quit();
                                    }));
    run_loop.Run();
    return success;
  }

  static bool RemoveImportedAccount(KeyringService* service,
                                    const std::string& address,
                                    const std::string& password,
                                    mojom::CoinType coin) {
    bool success;
    base::RunLoop run_loop;
    service->RemoveImportedAccount(address, password, coin,
                                   base::BindLambdaForTesting([&](bool v) {
                                     success = v;
                                     run_loop.Quit();
                                   }));
    run_loop.Run();
    return success;
  }

  static bool RemoveHardwareAccount(KeyringService* service,
                                    const std::string& address,
                                    mojom::CoinType coin) {
    bool success;
    base::RunLoop run_loop;
    service->RemoveHardwareAccount(address, coin,
                                   base::BindLambdaForTesting([&](bool v) {
                                     success = v;
                                     run_loop.Quit();
                                   }));
    run_loop.Run();
    return success;
  }

  static absl::optional<std::string> EncodePrivateKeyForExport(
      KeyringService* service,
      const std::string& address,
      mojom::CoinType coin,
      const std::string& password = kPasswordBrave) {
    absl::optional<std::string> private_key;
    base::RunLoop run_loop;
    service->EncodePrivateKeyForExport(
        address, password, coin,
        base::BindLambdaForTesting([&](const std::string& key) {
          if (!key.empty()) {
            private_key = key;
          }
          run_loop.Quit();
        }));
    run_loop.Run();
    return private_key;
  }

  static absl::optional<std::string> ImportFilecoinAccount(
      KeyringService* service,
      const std::string& account_name,
      const std::string& private_key_hex,
      const std::string& network) {
    absl::optional<std::string> account;
    base::RunLoop run_loop;
    service->ImportFilecoinAccount(
        account_name, private_key_hex, network,
        base::BindLambdaForTesting(
            [&](bool success, const std::string& address) {
              if (success) {
                account = address;
              }
              run_loop.Quit();
            }));
    run_loop.Run();
    return account;
  }

  static absl::optional<std::string> ImportAccount(
      KeyringService* service,
      const std::string& name,
      const std::string& private_key,
      mojom::CoinType coin) {
    absl::optional<std::string> account;
    base::RunLoop run_loop;
    service->ImportAccount(name, private_key, coin,
                           base::BindLambdaForTesting(
                               [&](bool success, const std::string& address) {
                                 if (success) {
                                   account = address;
                                 }
                                 run_loop.Quit();
                               }));
    run_loop.Run();
    return account;
  }

  static absl::optional<std::string> ImportAccountFromJson(
      KeyringService* service,
      const std::string& name,
      const std::string& password,
      const std::string& json) {
    absl::optional<std::string> account;
    base::RunLoop run_loop;
    service->ImportAccountFromJson(
        name, password, json,
        base::BindLambdaForTesting(
            [&](bool success, const std::string& address) {
              if (success) {
                account = address;
              }
              run_loop.Quit();
            }));
    run_loop.Run();
    return account;
  }

  static absl::optional<std::string> CreateWallet(KeyringService* service,
                                                  const std::string& password) {
    std::string mnemonic;
    base::RunLoop run_loop;
    service->CreateWallet(password,
                          base::BindLambdaForTesting([&](const std::string& v) {
                            mnemonic = v;
                            run_loop.Quit();
                          }));
    run_loop.Run();
    return mnemonic;
  }

  static bool RestoreWallet(KeyringService* service,
                            const std::string& mnemonic,
                            const std::string& password,
                            bool is_legacy_brave_wallet) {
    bool success = false;
    base::RunLoop run_loop;
    service->RestoreWallet(mnemonic, password, is_legacy_brave_wallet,
                           base::BindLambdaForTesting([&](bool v) {
                             success = v;
                             run_loop.Quit();
                           }));
    run_loop.Run();
    return success;
  }

  static bool AddAccount(KeyringService* service,
                         const std::string& account_name,
                         mojom::CoinType coin) {
    bool success = false;
    base::RunLoop run_loop;
    service->AddAccount(account_name, coin,
                        base::BindLambdaForTesting([&](bool v) {
                          success = v;
                          run_loop.Quit();
                        }));
    run_loop.Run();
    return success;
  }

  static void UpdateNameForHardwareAccount(KeyringService* service,
                                           const std::string& address,
                                           const std::string& account_name,
                                           mojom::CoinType coin) {
    service->UpdateNameForHardwareAccountSync(address, account_name, coin);
  }

  static bool AddFilecoinAccount(KeyringService* service,
                                 const std::string& account_name,
                                 const std::string& network) {
    bool success = false;
    base::RunLoop run_loop;
    service->AddFilecoinAccount(account_name, network,
                                base::BindLambdaForTesting([&](bool v) {
                                  success = v;
                                  run_loop.Quit();
                                }));
    run_loop.Run();
    return success;
  }

  static void ImportFilecoinAccounts(
      KeyringService* service,
      TestKeyringServiceObserver* observer,
      const std::vector<ImportData>& imported_accounts,
      const std::string& keyring_id) {
    for (size_t i = 0; i < imported_accounts.size(); ++i) {
      absl::optional<std::string> address = ImportFilecoinAccount(
          service, imported_accounts[i].name,
          imported_accounts[i].import_payload, imported_accounts[i].network);
      ASSERT_TRUE(address.has_value());
      EXPECT_EQ(address.value(), imported_accounts[i].address);
      // ImportFilecoinAccount waits only until account is added,
      // But there are still mojo tasks we need to wait
      base::RunLoop().RunUntilIdle();
      EXPECT_TRUE(service->IsKeyringCreated(keyring_id));
      if (i == 0) {
        EXPECT_TRUE(observer->IsKeyringCreated(keyring_id));
        observer->Reset();
      } else {
        EXPECT_FALSE(observer->IsKeyringCreated(keyring_id));
      }

      auto payload = EncodePrivateKeyForExport(
          service, imported_accounts[i].address, mojom::CoinType::FIL);
      EXPECT_TRUE(payload);
      EXPECT_EQ(imported_accounts[i].import_payload, *payload);
      EXPECT_EQ(
          imported_accounts[i].address,
          GetFilecoinSelectedAccount(service, imported_accounts[i].network));
    }
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

  static bool SetAutoLockMinutes(KeyringService* service,
                                 TestKeyringServiceObserver* observer,
                                 int32_t minutes) {
    bool success = false;
    base::RunLoop run_loop;
    int32_t old_minutes = GetAutoLockMinutes(service);
    service->SetAutoLockMinutes(minutes,
                                base::BindLambdaForTesting([&](bool v) {
                                  success = v;
                                  run_loop.Quit();
                                }));
    run_loop.Run();
    // Make sure observers are received
    base::RunLoop().RunUntilIdle();
    if (old_minutes != minutes && success) {
      EXPECT_TRUE(observer->AutoLockMinutesChangedFired());
    } else {
      EXPECT_FALSE(observer->AutoLockMinutesChangedFired());
    }
    observer->Reset();
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
    bool result;
    base::RunLoop run_loop;
    json_rpc_service_->SetNetwork(chain_id, coin,
                                  base::BindLambdaForTesting([&](bool success) {
                                    result = success;
                                    run_loop.Quit();
                                  }));
    run_loop.Run();
    return result;
  }

  static bool Lock(KeyringService* service) {
    service->Lock();
    return service->IsLocked(mojom::kDefaultKeyringId);
  }

  content::BrowserTaskEnvironment task_environment_;

 private:
  std::unique_ptr<TestingProfile> profile_;
  TestingPrefServiceSimple local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};  // namespace brave_wallet

TEST_F(KeyringServiceUnitTest, HasAndGetPrefForKeyring) {
  base::Value::Dict dict;
  dict.SetByDottedPath("default.pref1", base::Value("123"));
  GetPrefs()->SetDict(kBraveWalletKeyrings, std::move(dict));
  EXPECT_TRUE(KeyringService::HasPrefForKeyring(*GetPrefs(), "pref1",
                                                mojom::kDefaultKeyringId));
  const base::Value* value = KeyringService::GetPrefForKeyring(
      *GetPrefs(), "pref1", mojom::kDefaultKeyringId);
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->GetString(), "123");

  EXPECT_FALSE(
      KeyringService::HasPrefForKeyring(*GetPrefs(), "pref1", "keyring2"));
  EXPECT_EQ(KeyringService::GetPrefForKeyring(*GetPrefs(), "pref1", "keyring2"),
            nullptr);

  EXPECT_FALSE(KeyringService::HasPrefForKeyring(*GetPrefs(), "pref2",
                                                 mojom::kDefaultKeyringId));
  EXPECT_EQ(KeyringService::GetPrefForKeyring(*GetPrefs(), "pref2",
                                              mojom::kDefaultKeyringId),
            nullptr);
}

TEST_F(KeyringServiceUnitTest, SetPrefForKeyring) {
  KeyringService::SetPrefForKeyring(GetPrefs(), "pref1", base::Value("123"),
                                    mojom::kDefaultKeyringId);
  const auto& keyrings_pref = GetPrefs()->GetDict(kBraveWalletKeyrings);
  const std::string* value =
      keyrings_pref.FindStringByDottedPath("default.pref1");
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(*value, "123");

  EXPECT_EQ(keyrings_pref.FindByDottedPath("default.pref2"), nullptr);
  EXPECT_EQ(keyrings_pref.FindByDottedPath("keyring2.pref1"), nullptr);
}

TEST_F(KeyringServiceUnitTest, GetPrefInBytesForKeyring) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  KeyringService::SetPrefForKeyring(GetPrefs(), kEncryptedMnemonic,
                                    base::Value("3q2+7w=="),
                                    mojom::kDefaultKeyringId);

  auto verify_bytes = [](const std::vector<uint8_t>& bytes) {
    ASSERT_EQ(bytes.size(), 4u);
    EXPECT_EQ(bytes[0], 0xde);
    EXPECT_EQ(bytes[1], 0xad);
    EXPECT_EQ(bytes[2], 0xbe);
    EXPECT_EQ(bytes[3], 0xef);
  };

  auto mnemonic = KeyringService::GetPrefInBytesForKeyring(
      *GetPrefs(), kEncryptedMnemonic, mojom::kDefaultKeyringId);
  ASSERT_TRUE(mnemonic);
  verify_bytes(*mnemonic);

  // invalid base64 encoded
  KeyringService::SetPrefForKeyring(GetPrefs(), kEncryptedMnemonic,
                                    base::Value("3q2+7w^^"),
                                    mojom::kDefaultKeyringId);
  EXPECT_FALSE(KeyringService::GetPrefInBytesForKeyring(
      *GetPrefs(), kEncryptedMnemonic, mojom::kDefaultKeyringId));

  // default pref value (empty)
  GetPrefs()->ClearPref(kBraveWalletKeyrings);
  EXPECT_FALSE(KeyringService::GetPrefInBytesForKeyring(
      *GetPrefs(), kEncryptedMnemonic, mojom::kDefaultKeyringId));

  // non-existing pref
  EXPECT_FALSE(KeyringService::GetPrefInBytesForKeyring(
      *GetPrefs(), "brave.nothinghere", mojom::kDefaultKeyringId));

  // non-string pref
  KeyringService::SetPrefForKeyring(GetPrefs(), "test_num", base::Value(123),
                                    mojom::kDefaultKeyringId);
  EXPECT_FALSE(KeyringService::GetPrefInBytesForKeyring(
      *GetPrefs(), "test_num", mojom::kDefaultKeyringId));
}

TEST_F(KeyringServiceUnitTest, SetPrefInBytesForKeyring) {
  const uint8_t bytes_array[] = {0xde, 0xad, 0xbe, 0xef};
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  KeyringService::SetPrefInBytesForKeyring(
      GetPrefs(), kEncryptedMnemonic, bytes_array, mojom::kDefaultKeyringId);
  EXPECT_EQ(
      GetStringPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId),
      "3q2+7w==");

  GetPrefs()->ClearPref(kBraveWalletKeyrings);
  const std::vector<uint8_t> bytes_vector = {0xde, 0xad, 0xbe, 0xef};
  KeyringService::SetPrefInBytesForKeyring(
      GetPrefs(), kEncryptedMnemonic, bytes_vector, mojom::kDefaultKeyringId);
  EXPECT_EQ(
      GetStringPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId),
      "3q2+7w==");
}

TEST_F(KeyringServiceUnitTest, GetOrCreateNonceForKeyring) {
  std::string encoded_nonce;
  std::string encoded_nonce2;
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    const std::vector<uint8_t> nonce =
        service.GetOrCreateNonceForKeyring(mojom::kDefaultKeyringId);
    encoded_nonce = base::Base64Encode(nonce);
    const std::vector<uint8_t> nonce2 =
        service.GetOrCreateNonceForKeyring("keyring2");
    encoded_nonce2 = base::Base64Encode(nonce2);
    EXPECT_EQ(encoded_nonce, GetStringPrefForKeyring(kPasswordEncryptorNonce,
                                                     mojom::kDefaultKeyringId));
    EXPECT_EQ(encoded_nonce2,
              GetStringPrefForKeyring(kPasswordEncryptorNonce, "keyring2"));
  }
  {  // It should be the same nonce as long as the pref exists
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    const std::vector<uint8_t> nonce =
        service.GetOrCreateNonceForKeyring(mojom::kDefaultKeyringId);
    EXPECT_EQ(base::Base64Encode(nonce), encoded_nonce);
    const std::vector<uint8_t> nonce2 =
        service.GetOrCreateNonceForKeyring("keyring2");
    EXPECT_EQ(base::Base64Encode(nonce2), encoded_nonce2);
    EXPECT_EQ(encoded_nonce, GetStringPrefForKeyring(kPasswordEncryptorNonce,
                                                     mojom::kDefaultKeyringId));
    EXPECT_EQ(encoded_nonce2,
              GetStringPrefForKeyring(kPasswordEncryptorNonce, "keyring2"));
  }
  GetPrefs()->ClearPref(kBraveWalletKeyrings);
  {  // nonce should be different now
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    const std::vector<uint8_t> nonce =
        service.GetOrCreateNonceForKeyring(mojom::kDefaultKeyringId);
    EXPECT_NE(base::Base64Encode(nonce), encoded_nonce);
    const std::vector<uint8_t> nonce2 =
        service.GetOrCreateNonceForKeyring("keyring2");
    EXPECT_NE(base::Base64Encode(nonce2), encoded_nonce2);
  }
  {  // nonce should change after calling with force_reset
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    const std::vector<uint8_t> nonce =
        service.GetOrCreateNonceForKeyring(mojom::kDefaultKeyringId);
    const std::vector<uint8_t> nonce2 =
        service.GetOrCreateNonceForKeyring("keyring2");

    const std::vector<uint8_t> nonce_new =
        service.GetOrCreateNonceForKeyring(mojom::kDefaultKeyringId, true);
    const std::vector<uint8_t> nonce2_new =
        service.GetOrCreateNonceForKeyring("keyring2", true);
    EXPECT_NE(nonce, nonce_new);
    EXPECT_NE(nonce2, nonce2_new);

    EXPECT_EQ(nonce_new,
              service.GetOrCreateNonceForKeyring(mojom::kDefaultKeyringId));
    EXPECT_EQ(nonce2_new, service.GetOrCreateNonceForKeyring("keyring2"));
  }
}

TEST_F(KeyringServiceUnitTest, GetOrCreateSaltForKeyring) {
  std::string encoded_salt;
  std::string encoded_salt2;
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    const std::vector<uint8_t> salt =
        service.GetOrCreateSaltForKeyring(mojom::kDefaultKeyringId);
    encoded_salt = base::Base64Encode(salt);
    const std::vector<uint8_t> salt2 =
        service.GetOrCreateSaltForKeyring("keyring2");
    encoded_salt2 = base::Base64Encode(salt2);
    EXPECT_EQ(encoded_salt, GetStringPrefForKeyring(kPasswordEncryptorSalt,
                                                    mojom::kDefaultKeyringId));
    EXPECT_EQ(encoded_salt2,
              GetStringPrefForKeyring(kPasswordEncryptorSalt, "keyring2"));
  }
  {  // It should be the same salt as long as the pref exists
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    const std::vector<uint8_t> salt =
        service.GetOrCreateSaltForKeyring(mojom::kDefaultKeyringId);
    EXPECT_EQ(base::Base64Encode(salt), encoded_salt);
    const std::vector<uint8_t> salt2 =
        service.GetOrCreateSaltForKeyring("keyring2");
    EXPECT_EQ(base::Base64Encode(salt2), encoded_salt2);
    EXPECT_EQ(encoded_salt, GetStringPrefForKeyring(kPasswordEncryptorSalt,
                                                    mojom::kDefaultKeyringId));
    EXPECT_EQ(encoded_salt2,
              GetStringPrefForKeyring(kPasswordEncryptorSalt, "keyring2"));
  }
  GetPrefs()->ClearPref(kBraveWalletKeyrings);
  {  // salt should be different now
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    const std::vector<uint8_t> salt =
        service.GetOrCreateSaltForKeyring(mojom::kDefaultKeyringId);
    EXPECT_NE(base::Base64Encode(salt), encoded_salt);
    const std::vector<uint8_t> salt2 =
        service.GetOrCreateSaltForKeyring("keyring2");
    EXPECT_NE(base::Base64Encode(salt2), encoded_salt2);
  }
  {  // salt should change after calling with force_reset
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    const std::vector<uint8_t> salt =
        service.GetOrCreateSaltForKeyring(mojom::kDefaultKeyringId);
    const std::vector<uint8_t> salt2 =
        service.GetOrCreateSaltForKeyring("keyring2");

    const std::vector<uint8_t> salt_new =
        service.GetOrCreateSaltForKeyring(mojom::kDefaultKeyringId, true);
    const std::vector<uint8_t> salt2_new =
        service.GetOrCreateSaltForKeyring("keyring2", true);
    EXPECT_NE(salt, salt_new);
    EXPECT_NE(salt2, salt2_new);

    EXPECT_EQ(salt_new,
              service.GetOrCreateSaltForKeyring(mojom::kDefaultKeyringId));
    EXPECT_EQ(salt2_new, service.GetOrCreateSaltForKeyring("keyring2"));
  }
}

TEST_F(KeyringServiceUnitTest, CreateEncryptorForKeyring) {
  std::string encoded_salt;
  std::string encoded_salt2;
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    EXPECT_TRUE(
        service.CreateEncryptorForKeyring("123", mojom::kDefaultKeyringId));
    EXPECT_NE(service.encryptors_.at(mojom::kDefaultKeyringId), nullptr);
    EXPECT_TRUE(service.CreateEncryptorForKeyring("456", "keyring2"));
    EXPECT_NE(service.encryptors_.at("keyring2"), nullptr);
    EXPECT_NE(service.encryptors_.at("keyring2"),
              service.encryptors_.at(mojom::kDefaultKeyringId));
    encoded_salt = GetStringPrefForKeyring(kPasswordEncryptorSalt,
                                           mojom::kDefaultKeyringId);
    EXPECT_FALSE(encoded_salt.empty());
    encoded_salt2 = GetStringPrefForKeyring(kPasswordEncryptorSalt, "keyring2");
    EXPECT_FALSE(encoded_salt2.empty());
  }
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    EXPECT_TRUE(
        service.CreateEncryptorForKeyring("123", mojom::kDefaultKeyringId));
    EXPECT_TRUE(service.CreateEncryptorForKeyring("456", "keyring2"));
    EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorSalt,
                                      mojom::kDefaultKeyringId),
              encoded_salt);
    EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorSalt, "keyring2"),
              encoded_salt2);
  }
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    EXPECT_FALSE(
        service.CreateEncryptorForKeyring("", mojom::kDefaultKeyringId));
    ASSERT_TRUE(service.encryptors_.empty());
    EXPECT_FALSE(service.CreateEncryptorForKeyring("", "keyring2"));
    ASSERT_TRUE(service.encryptors_.empty());
  }
}

TEST_F(KeyringServiceUnitTest, CreateDefaultKeyringInternal) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  // encryptor is nullptr
  ASSERT_FALSE(service.CreateKeyringInternal(mojom::kDefaultKeyringId,
                                             kMnemonic1, false));

  EXPECT_TRUE(
      service.CreateEncryptorForKeyring("brave", mojom::kDefaultKeyringId));
  ASSERT_TRUE(service.CreateKeyringInternal(mojom::kDefaultKeyringId,
                                            kMnemonic1, false));
  base::RunLoop().RunUntilIdle();
  auto* default_keyring =
      service.GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId);
  default_keyring->AddAccounts(1);
  EXPECT_EQ(default_keyring->GetAddress(0),
            "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db");
  const std::string encrypted_mnemonic1 =
      GetStringPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId);
  // The pref is encrypted
  EXPECT_NE(
      base::Base64Encode(std::vector<uint8_t>(
          reinterpret_cast<const uint8_t*>(kMnemonic1),
          reinterpret_cast<const uint8_t*>(kMnemonic1) + strlen(kMnemonic1))),
      encrypted_mnemonic1);

  // default keyring will be overwritten
  ASSERT_TRUE(service.CreateKeyringInternal(mojom::kDefaultKeyringId,
                                            kMnemonic2, false));
  auto* default_keyring2 =
      service.GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId);
  default_keyring2->AddAccounts(1);
  EXPECT_EQ(default_keyring2->GetAddress(0),
            "0xf83C3cBfF68086F276DD4f87A82DF73B57b28820");
  EXPECT_NE(
      GetStringPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId),
      encrypted_mnemonic1);
}

TEST_F(KeyringServiceUnitTest, CreateDefaultKeyring) {
  std::string salt;
  std::string nonce;
  std::string mnemonic;
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    EXPECT_EQ(service.CreateKeyring(mojom::kDefaultKeyringId,
                                    GenerateMnemonic(16), ""),
              nullptr);
    EXPECT_FALSE(
        HasPrefForKeyring(kPasswordEncryptorSalt, mojom::kDefaultKeyringId));
    EXPECT_FALSE(
        HasPrefForKeyring(kPasswordEncryptorNonce, mojom::kDefaultKeyringId));
    EXPECT_FALSE(
        HasPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId));

    HDKeyring* keyring = service.CreateKeyring(mojom::kDefaultKeyringId,
                                               GenerateMnemonic(16), "brave1");
    keyring->AddAccounts(1);
    const std::string address1 = keyring->GetAddress(0);
    EXPECT_FALSE(address1.empty());
    EXPECT_TRUE(
        HasPrefForKeyring(kPasswordEncryptorSalt, mojom::kDefaultKeyringId));
    EXPECT_TRUE(
        HasPrefForKeyring(kPasswordEncryptorNonce, mojom::kDefaultKeyringId));
    EXPECT_TRUE(
        HasPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId));

    // default keyring will be overwritten
    keyring = service.CreateKeyring(mojom::kDefaultKeyringId,
                                    GenerateMnemonic(16), "brave2");
    keyring->AddAccounts(1);
    const std::string address2 = keyring->GetAddress(0);
    EXPECT_FALSE(address2.empty());
    EXPECT_NE(address1, address2);

    salt = GetStringPrefForKeyring(kPasswordEncryptorSalt,
                                   mojom::kDefaultKeyringId);
    nonce = GetStringPrefForKeyring(kPasswordEncryptorNonce,
                                    mojom::kDefaultKeyringId);
    mnemonic =
        GetStringPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId);
  }

  // mnemonic, salt and account number don't get clear unless Reset() is called
  EXPECT_EQ(
      GetStringPrefForKeyring(kPasswordEncryptorSalt, mojom::kDefaultKeyringId),
      salt);
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorNonce,
                                    mojom::kDefaultKeyringId),
            nonce);
  EXPECT_EQ(
      GetStringPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId),
      mnemonic);
}

TEST_F(KeyringServiceUnitTest, RestoreDefaultKeyring) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  absl::optional<std::string> mnemonic = CreateWallet(&service, "brave");
  ASSERT_TRUE(mnemonic.has_value());

  std::string salt =
      GetStringPrefForKeyring(kPasswordEncryptorSalt, mojom::kDefaultKeyringId);
  std::string encrypted_mnemonic =
      GetStringPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId);
  std::string nonce = GetStringPrefForKeyring(kPasswordEncryptorNonce,
                                              mojom::kDefaultKeyringId);

  // Restore with same mnemonic and same password
  EXPECT_NE(service.RestoreKeyring(mojom::kDefaultKeyringId, *mnemonic, "brave",
                                   false),
            nullptr);
  EXPECT_EQ(
      GetStringPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId),
      encrypted_mnemonic);
  EXPECT_EQ(
      GetStringPrefForKeyring(kPasswordEncryptorSalt, mojom::kDefaultKeyringId),
      salt);
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorNonce,
                                    mojom::kDefaultKeyringId),
            nonce);
  EXPECT_EQ(
      service.GetAccountInfosForKeyring(brave_wallet::mojom::kDefaultKeyringId)
          .size(),
      1u);

  // Restore with same mnemonic but different password
  EXPECT_NE(service.RestoreKeyring(mojom::kDefaultKeyringId, *mnemonic,
                                   "brave377", false),
            nullptr);
  EXPECT_NE(
      GetStringPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId),
      encrypted_mnemonic);
  EXPECT_NE(
      GetStringPrefForKeyring(kPasswordEncryptorSalt, mojom::kDefaultKeyringId),
      salt);
  EXPECT_NE(GetStringPrefForKeyring(kPasswordEncryptorNonce,
                                    mojom::kDefaultKeyringId),
            nonce);
  EXPECT_EQ(service.GetMnemonicForKeyringImpl(mojom::kDefaultKeyringId),
            *mnemonic);
  EXPECT_EQ(
      service.GetAccountInfosForKeyring(brave_wallet::mojom::kDefaultKeyringId)
          .size(),
      0u);

  // Update salt for next test case
  encrypted_mnemonic =
      GetStringPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId);
  salt =
      GetStringPrefForKeyring(kPasswordEncryptorSalt, mojom::kDefaultKeyringId);
  nonce = GetStringPrefForKeyring(kPasswordEncryptorNonce,
                                  mojom::kDefaultKeyringId);

  // Restore with invalid mnemonic but same password
  EXPECT_EQ(
      service.RestoreKeyring(mojom::kDefaultKeyringId, "", "brave", false),
      nullptr);
  // Keyring prefs won't be cleared
  EXPECT_EQ(
      GetStringPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId),
      encrypted_mnemonic);
  EXPECT_EQ(
      GetStringPrefForKeyring(kPasswordEncryptorSalt, mojom::kDefaultKeyringId),
      salt);
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorNonce,
                                    mojom::kDefaultKeyringId),
            nonce);
  EXPECT_EQ(
      service.GetAccountInfosForKeyring(brave_wallet::mojom::kDefaultKeyringId)
          .size(),
      0u);

  // Restore with same mnemonic but empty password
  EXPECT_EQ(
      service.RestoreKeyring(mojom::kDefaultKeyringId, *mnemonic, "", false),
      nullptr);
  // Keyring prefs won't be cleared
  EXPECT_EQ(
      GetStringPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId),
      encrypted_mnemonic);
  EXPECT_EQ(
      GetStringPrefForKeyring(kPasswordEncryptorSalt, mojom::kDefaultKeyringId),
      salt);
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorNonce,
                                    mojom::kDefaultKeyringId),
            nonce);
  EXPECT_EQ(
      service.GetAccountInfosForKeyring(brave_wallet::mojom::kDefaultKeyringId)
          .size(),
      0u);

  // default keyring will be overwritten by new seed which will be encrypted by
  // new key even though the passphrase is same.
  EXPECT_NE(service.RestoreKeyring(mojom::kDefaultKeyringId, kMnemonic1,
                                   "brave", false),
            nullptr);
  EXPECT_NE(
      GetStringPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId),
      encrypted_mnemonic);
  // salt is regenerated and account num is cleared
  EXPECT_NE(
      GetStringPrefForKeyring(kPasswordEncryptorSalt, mojom::kDefaultKeyringId),
      salt);
  EXPECT_NE(GetStringPrefForKeyring(kPasswordEncryptorNonce,
                                    mojom::kDefaultKeyringId),
            nonce);
  ASSERT_TRUE(AddAccount(&service, "Account 1", mojom::CoinType::ETH));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(
      service.GetAccountInfosForKeyring(brave_wallet::mojom::kDefaultKeyringId)
          .size(),
      1u);
  EXPECT_EQ(service.GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId)
                ->GetAddress(0),
            "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db");
}

TEST_F(KeyringServiceUnitTest, UnlockResumesDefaultKeyring) {
  std::string salt;
  std::string mnemonic;
  std::string nonce;
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    ASSERT_TRUE(CreateWallet(&service, "brave"));
    ASSERT_TRUE(AddAccount(&service, "Account2", mojom::CoinType::ETH));

    salt = GetStringPrefForKeyring(kPasswordEncryptorSalt,
                                   mojom::kDefaultKeyringId);
    nonce = GetStringPrefForKeyring(kPasswordEncryptorNonce,
                                    mojom::kDefaultKeyringId);
    mnemonic =
        GetStringPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId);
  }
  {
    // KeyringService is now destructed, simlulating relaunch
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    EXPECT_TRUE(Unlock(&service, "brave"));
    ASSERT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));

    EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorSalt,
                                      mojom::kDefaultKeyringId),
              salt);
    EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorNonce,
                                      mojom::kDefaultKeyringId),
              nonce);
    EXPECT_EQ(
        GetStringPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId),
        mnemonic);
    EXPECT_EQ(
        2u, service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId).size());
  }
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    // wrong password
    EXPECT_FALSE(Unlock(&service, "brave123"));
    ASSERT_TRUE(service.IsLocked(mojom::kDefaultKeyringId));
    // empty password
    EXPECT_FALSE(Unlock(&service, ""));
    ASSERT_TRUE(service.IsLocked(mojom::kDefaultKeyringId));
  }
}

TEST_F(KeyringServiceUnitTest, GetMnemonicForDefaultKeyring) {
  // Needed to skip unnecessary migration in CreateEncryptorForKeyring.
  GetPrefs()->SetBoolean(kBraveWalletKeyringEncryptionKeysMigrated, true);
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(service.CreateEncryptorForKeyring(kPasswordBrave,
                                                mojom::kDefaultKeyringId));

  // no pref exists yet
  EXPECT_TRUE(GetMnemonicForDefaultKeyring(kPasswordBrave, &service).empty());

  ASSERT_TRUE(service.CreateKeyringInternal(mojom::kDefaultKeyringId,
                                            kMnemonic1, false));
  EXPECT_EQ(GetMnemonicForDefaultKeyring(kPasswordBrave, &service), kMnemonic1);

  // Lock service
  service.Lock();
  ASSERT_TRUE(service.IsLocked(mojom::kDefaultKeyringId));
  EXPECT_TRUE(GetMnemonicForDefaultKeyring(kPasswordBrave, &service).empty());

  // unlock with wrong password
  ASSERT_FALSE(Unlock(&service, kPasswordBrave123));
  ASSERT_TRUE(service.IsLocked(mojom::kDefaultKeyringId));

  EXPECT_TRUE(GetMnemonicForDefaultKeyring(kPasswordBrave, &service).empty());

  ASSERT_TRUE(Unlock(&service, kPasswordBrave));
  ASSERT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));

  // Can only get mnemonic when password is correct.
  EXPECT_TRUE(
      GetMnemonicForDefaultKeyring(kPasswordBrave123, &service).empty());
  EXPECT_EQ(GetMnemonicForDefaultKeyring(kPasswordBrave, &service), kMnemonic1);
}

TEST_F(KeyringServiceUnitTest, ValidatePassword) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(CreateWallet(&service, "brave"));

  EXPECT_TRUE(ValidatePassword(&service, "brave"));
  EXPECT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
  EXPECT_FALSE(ValidatePassword(&service, "brave123"));
  EXPECT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));

  service.Lock();
  EXPECT_TRUE(ValidatePassword(&service, "brave"));
}

TEST_F(KeyringServiceUnitTest, GetKeyringInfo) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  EXPECT_TRUE(IsKeyringInfoEmpty(&service, mojom::kDefaultKeyringId));

  ASSERT_TRUE(CreateWallet(&service, "brave"));

  bool callback_called = false;
  service.GetKeyringInfo(
      mojom::kDefaultKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_EQ(keyring_info->id, mojom::kDefaultKeyringId);
        EXPECT_TRUE(keyring_info->is_keyring_created);
        EXPECT_FALSE(keyring_info->is_locked);
        EXPECT_FALSE(keyring_info->is_backed_up);
        EXPECT_EQ(keyring_info->account_infos.size(), 1u);
        EXPECT_FALSE(keyring_info->account_infos[0]->address.empty());
        EXPECT_EQ(keyring_info->account_infos[0]->name, "Account 1");
        EXPECT_FALSE(keyring_info->account_infos[0]->is_imported);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  service.NotifyWalletBackupComplete();
  ASSERT_TRUE(AddAccount(&service, "Account5566", mojom::CoinType::ETH));

  callback_called = false;
  service.GetKeyringInfo(
      brave_wallet::mojom::kDefaultKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_EQ(keyring_info->id, mojom::kDefaultKeyringId);
        EXPECT_TRUE(keyring_info->is_keyring_created);
        EXPECT_FALSE(keyring_info->is_locked);
        EXPECT_TRUE(keyring_info->is_backed_up);
        EXPECT_EQ(keyring_info->account_infos.size(), 2u);
        EXPECT_FALSE(keyring_info->account_infos[0]->address.empty());
        EXPECT_EQ(keyring_info->account_infos[0]->name, "Account 1");
        EXPECT_FALSE(keyring_info->account_infos[1]->address.empty());
        EXPECT_EQ(keyring_info->account_infos[1]->name, "Account5566");
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // invalid id or keyring is not yet created
  EXPECT_TRUE(IsKeyringInfoEmpty(&service, "invalid_id"));
}

TEST_F(KeyringServiceUnitTest, LockAndUnlock) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeatures(
      {brave_wallet::features::kBraveWalletFilecoinFeature,
       brave_wallet::features::kBraveWalletSolanaFeature},
      {});

  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    // No encryptor but there is no keyring created so they should be unlocked.
    // And Lock() has no effect here.
    service.Lock();
    EXPECT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
    EXPECT_FALSE(service.IsLocked(mojom::kFilecoinKeyringId));
    EXPECT_FALSE(service.IsLocked(mojom::kSolanaKeyringId));
    EXPECT_FALSE(service.IsLockedSync());
  }
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    TestKeyringServiceObserver observer;
    service.AddObserver(observer.GetReceiver());
    ASSERT_NE(service.CreateKeyring(brave_wallet::mojom::kDefaultKeyringId,
                                    GenerateMnemonic(16), "brave"),
              nullptr);
    ASSERT_TRUE(AddAccount(&service, "ETH Account 1", mojom::CoinType::ETH));
    EXPECT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
    EXPECT_FALSE(service.IsLocked(mojom::kFilecoinKeyringId));
    EXPECT_FALSE(service.IsLocked(mojom::kSolanaKeyringId));
    ASSERT_NE(service.CreateKeyring(brave_wallet::mojom::kFilecoinKeyringId,
                                    GenerateMnemonic(16), "brave"),
              nullptr);
    ASSERT_TRUE(
        AddFilecoinAccount(&service, "FIL Account 1", mojom::kFilecoinMainnet));
    ASSERT_NE(service.CreateKeyring(brave_wallet::mojom::kSolanaKeyringId,
                                    GenerateMnemonic(16), "brave"),
              nullptr);
    ASSERT_TRUE(AddAccount(&service, "SOL Account 1", mojom::CoinType::SOL));
    EXPECT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
    EXPECT_FALSE(service.IsLocked(mojom::kFilecoinKeyringId));
    EXPECT_FALSE(service.IsLocked(mojom::kSolanaKeyringId));

    service.Lock();
    // wait for observer
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(observer.LockedFired());
    observer.Reset();
    EXPECT_TRUE(service.IsLocked(mojom::kDefaultKeyringId));
    EXPECT_TRUE(service.IsLocked(mojom::kFilecoinKeyringId));
    EXPECT_TRUE(service.IsLocked(mojom::kSolanaKeyringId));
    EXPECT_TRUE(service.IsLockedSync());
    EXPECT_FALSE(
        service.GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId));
    EXPECT_FALSE(
        service.GetHDKeyringById(brave_wallet::mojom::kFilecoinKeyringId));
    EXPECT_FALSE(
        service.GetHDKeyringById(brave_wallet::mojom::kSolanaKeyringId));

    EXPECT_FALSE(Unlock(&service, "abc"));
    // wait for observer
    base::RunLoop().RunUntilIdle();
    EXPECT_FALSE(observer.UnlockedFired());
    observer.Reset();
    EXPECT_TRUE(service.IsLocked(mojom::kDefaultKeyringId));
    EXPECT_TRUE(service.IsLocked(mojom::kFilecoinKeyringId));
    EXPECT_TRUE(service.IsLocked(mojom::kSolanaKeyringId));
    EXPECT_TRUE(service.IsLockedSync());

    EXPECT_TRUE(Unlock(&service, "brave"));
    // wait for observer
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(observer.UnlockedFired());
    observer.Reset();
    EXPECT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
    EXPECT_FALSE(service.IsLocked(mojom::kFilecoinKeyringId));
    EXPECT_FALSE(service.IsLocked(mojom::kSolanaKeyringId));
    EXPECT_FALSE(service.IsLockedSync());
  }
}

TEST_F(KeyringServiceUnitTest, Reset) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(CreateWallet(&service, "brave"));
  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  ASSERT_TRUE(AddAccount(&service, "Account 1", mojom::CoinType::ETH));
  // Trigger account number saving
  service.Lock();

  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletKeyrings));
  EXPECT_TRUE(
      HasPrefForKeyring(kPasswordEncryptorSalt, mojom::kDefaultKeyringId));
  EXPECT_TRUE(
      HasPrefForKeyring(kPasswordEncryptorNonce, mojom::kDefaultKeyringId));
  EXPECT_TRUE(HasPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId));
  EXPECT_TRUE(service.IsKeyringCreated(brave_wallet::mojom::kDefaultKeyringId));
  service.Reset();
  EXPECT_FALSE(
      HasPrefForKeyring(kPasswordEncryptorSalt, mojom::kDefaultKeyringId));
  EXPECT_FALSE(
      HasPrefForKeyring(kPasswordEncryptorNonce, mojom::kDefaultKeyringId));
  EXPECT_FALSE(HasPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId));
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletKeyrings));
  auto* default_keyring =
      service.GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId);
  EXPECT_EQ(default_keyring, nullptr);
  ASSERT_TRUE(service.encryptors_.empty());
  EXPECT_FALSE(
      service.IsKeyringCreated(brave_wallet::mojom::kDefaultKeyringId));
  // Keyring observer fire
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.KeyringResetFired());
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
  base::test::ScopedFeatureList feature_list{
      features::kBraveWalletFilecoinFeature};

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  ASSERT_TRUE(RestoreWallet(&service, kMnemonic1, "brave", false));
  EXPECT_TRUE(AddAccount(&service, "AccountETH", mojom::CoinType::ETH));
  EXPECT_TRUE(AddAccount(&service, "AccountSOL", mojom::CoinType::SOL));
  EXPECT_TRUE(
      AddFilecoinAccount(&service, "AccountFIL", mojom::kFilecoinMainnet));
  EXPECT_TRUE(
      AddFilecoinAccount(&service, "AccountFILTest", mojom::kFilecoinTestnet));

  EXPECT_EQ(*KeyringService::GetPrefForKeyring(*GetPrefs(), kAccountMetas,
                                               mojom::kDefaultKeyringId),
            base::test::ParseJson(R"(
  {
    "m/44'/60'/0'/0/0": {
        "account_address": "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db",
        "account_name": "Account 1"
    },
    "m/44'/60'/0'/0/1": {
        "account_address": "0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0",
        "account_name": "AccountETH"
    }
  })"));

  EXPECT_EQ(*KeyringService::GetPrefForKeyring(*GetPrefs(), kAccountMetas,
                                               mojom::kSolanaKeyringId),
            base::test::ParseJson(R"(
  {
    "m/44'/501'/0'/0'": {
        "account_address": "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
        "account_name": "Solana Account 1"
    },
    "m/44'/501'/1'/0'": {
        "account_address": "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV",
        "account_name": "AccountSOL"
    }
  })"));

  EXPECT_EQ(*KeyringService::GetPrefForKeyring(*GetPrefs(), kAccountMetas,
                                               mojom::kFilecoinKeyringId),
            base::test::ParseJson(R"(
  {
    "m/44'/461'/0'/0/0": {
        "account_address": "f1qjidlytseoouzfhsgzczf3ettbhuaezorczeava",
        "account_name": "AccountFIL"
    }
  })"));

  EXPECT_EQ(*KeyringService::GetPrefForKeyring(
                *GetPrefs(), kAccountMetas, mojom::kFilecoinTestnetKeyringId),
            base::test::ParseJson(R"(
  {
    "m/44'/1'/0'/0/0": {
      "account_address": "t1dca7adhz5lbvin5n3qlw67munu6xhn5fpb77nly",
      "account_name": "AccountFILTest"
    }
  })"));
}

TEST_F(KeyringServiceUnitTest, CreateAndRestoreWallet) {
  base::test::ScopedFeatureList feature_list;
  base::FieldTrialParams parameters;
  parameters[features::kCreateDefaultSolanaAccount.name] = "false";

  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(
      brave_wallet::features::kBraveWalletSolanaFeature, parameters);

  feature_list.InitWithFeaturesAndParameters(enabled_features, {});

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  absl::optional<std::string> mnemonic_to_be_restored =
      CreateWallet(&service, "brave");
  ASSERT_TRUE(mnemonic_to_be_restored.has_value());

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.IsKeyringCreated(mojom::kDefaultKeyringId));
  EXPECT_FALSE(observer.IsKeyringRestored(mojom::kDefaultKeyringId));
  EXPECT_FALSE(observer.IsKeyringCreated(mojom::kFilecoinKeyringId));
  EXPECT_FALSE(observer.IsKeyringRestored(mojom::kFilecoinKeyringId));
  EXPECT_FALSE(observer.IsKeyringCreated(mojom::kSolanaKeyringId));
  EXPECT_FALSE(observer.IsKeyringRestored(mojom::kSolanaKeyringId));
  observer.Reset();

  std::vector<mojom::AccountInfoPtr> account_infos =
      service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
  EXPECT_EQ(account_infos.size(), 1u);
  EXPECT_EQ(account_infos[0]->address,
            service.GetSelectedAccount(mojom::CoinType::ETH).value());
  EXPECT_FALSE(service.GetSelectedAccount(mojom::CoinType::SOL));
  EXPECT_FALSE(service.GetFilecoinSelectedAccount(mojom::kFilecoinMainnet));
  EXPECT_FALSE(service.GetFilecoinSelectedAccount(mojom::kFilecoinTestnet));

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
          EXPECT_EQ(account_infos[0]->address,
                    service.GetSelectedAccount(mojom::CoinType::ETH).value());
          EXPECT_EQ(account_infos[0]->address, address0);
          EXPECT_EQ(account_infos[0]->name, "Account 1");
        }

        {
          std::vector<mojom::AccountInfoPtr> account_infos =
              service.GetAccountInfosForKeyring(
                  mojom::kFilecoinTestnetKeyringId);
          EXPECT_EQ(account_infos.size(), 0u);
          EXPECT_FALSE(
              service.GetFilecoinSelectedAccount(mojom::kFilecoinTestnet));
        }

        {
          std::vector<mojom::AccountInfoPtr> account_infos =
              service.GetAccountInfosForKeyring(mojom::kFilecoinKeyringId);
          EXPECT_EQ(account_infos.size(), 0u);
          EXPECT_FALSE(
              service.GetFilecoinSelectedAccount(mojom::kFilecoinMainnet));
        }

        {
          std::vector<mojom::AccountInfoPtr> account_infos =
              service.GetAccountInfosForKeyring(mojom::kSolanaKeyringId);
          EXPECT_EQ(account_infos.size(), 0u);
          EXPECT_FALSE(service.GetSelectedAccount(mojom::CoinType::SOL));
        }
      });
  verify_restore_wallet.Run();

  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.IsKeyringCreated(mojom::kDefaultKeyringId));
  EXPECT_TRUE(observer.IsKeyringRestored(mojom::kDefaultKeyringId));
  EXPECT_FALSE(observer.IsKeyringCreated(mojom::kFilecoinKeyringId));
  EXPECT_FALSE(observer.IsKeyringRestored(mojom::kFilecoinKeyringId));
  EXPECT_FALSE(observer.IsKeyringCreated(mojom::kSolanaKeyringId));
  EXPECT_FALSE(observer.IsKeyringRestored(mojom::kSolanaKeyringId));
  observer.Reset();
  // Restore twice consecutively should succeed and have only one account
  verify_restore_wallet.Run();

  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.IsKeyringCreated(mojom::kDefaultKeyringId));
  EXPECT_FALSE(observer.IsKeyringRestored(mojom::kDefaultKeyringId));
  EXPECT_FALSE(observer.IsKeyringCreated(mojom::kFilecoinKeyringId));
  EXPECT_FALSE(observer.IsKeyringRestored(mojom::kFilecoinKeyringId));
  EXPECT_FALSE(observer.IsKeyringCreated(mojom::kSolanaKeyringId));
  EXPECT_FALSE(observer.IsKeyringRestored(mojom::kSolanaKeyringId));
}

TEST_F(KeyringServiceUnitTest, DefaultSolanaAccountCreated) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_NE(CreateWallet(&service, "brave"), absl::nullopt);

  std::vector<mojom::AccountInfoPtr> account_infos =
      service.GetAccountInfosForKeyring(mojom::kSolanaKeyringId);
  EXPECT_EQ(account_infos.size(), 1u);
  EXPECT_FALSE(account_infos[0]->address.empty());
  EXPECT_EQ(account_infos[0]->name, "Solana Account 1");
}

TEST_F(KeyringServiceUnitTest, DefaultSolanaAccountRestored) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(RestoreWallet(&service, kMnemonic1, "brave", false));

  std::vector<mojom::AccountInfoPtr> account_infos =
      service.GetAccountInfosForKeyring(mojom::kSolanaKeyringId);
  EXPECT_EQ(account_infos.size(), 1u);
  EXPECT_FALSE(account_infos[0]->address.empty());
  EXPECT_EQ(account_infos[0]->name, "Solana Account 1");
}

TEST_F(KeyringServiceUnitTest, AddAccount) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_NE(CreateWallet(&service, "brave"), absl::nullopt);
  EXPECT_TRUE(AddAccount(&service, "Account5566", mojom::CoinType::ETH));

  std::vector<mojom::AccountInfoPtr> account_infos =
      service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
  EXPECT_EQ(account_infos.size(), 2u);
  EXPECT_FALSE(account_infos[0]->address.empty());
  EXPECT_EQ(account_infos[0]->name, "Account 1");
  EXPECT_FALSE(account_infos[1]->address.empty());
  EXPECT_EQ(account_infos[1]->name, "Account5566");
}

TEST_F(KeyringServiceUnitTest, MigrationPrefs) {
  GetPrefs()->SetDict(kBraveWalletKeyrings,
                      GetHardwareKeyringValueForTesting());
  EXPECT_EQ(*GetPrefs()
                 ->GetDict(kBraveWalletKeyrings)
                 .FindStringByDottedPath(
                     "hardware.A1.account_metas.0x111.account_name"),
            "test1");

  KeyringService::MigrateObsoleteProfilePrefs(GetPrefs());

  const base::Value::Dict& hardware_accounts =
      KeyringService::GetPrefForKeyring(*GetPrefs(), kHardwareAccounts,
                                        mojom::kDefaultKeyringId)
          ->GetDict();
  EXPECT_EQ(hardware_accounts.size(), 2u);
  EXPECT_EQ(*hardware_accounts.FindStringByDottedPath(
                "A1.account_metas.0x111.account_name"),
            "test1");
  EXPECT_EQ(*hardware_accounts.FindStringByDottedPath(
                "A1.account_metas.0x111.derivation_path"),
            "path1");
  EXPECT_EQ(*hardware_accounts.FindStringByDottedPath(
                "A1.account_metas.0x111.hardware_vendor"),
            "vendor1");

  EXPECT_EQ(*hardware_accounts.FindStringByDottedPath(
                "B2.account_metas.0x222.account_name"),
            "test2");
  EXPECT_EQ(*hardware_accounts.FindStringByDottedPath(
                "B2.account_metas.0x222.derivation_path"),
            "path2");
  EXPECT_EQ(*hardware_accounts.FindStringByDottedPath(
                "B2.account_metas.0x222.hardware_vendor"),
            "vendor2");
  ASSERT_FALSE(GetPrefs()
                   ->GetDict(kBraveWalletKeyrings)
                   .FindStringByDottedPath(
                       "hardware.A1.account_metas.0x111.account_name"));
}

TEST_F(KeyringServiceUnitTest, ImportedAccounts) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

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
    absl::optional<std::string> imported_account = ImportAccount(
        &service, account.name, account.private_key, mojom::CoinType::ETH);
    ASSERT_TRUE(imported_account.has_value());
    EXPECT_EQ(account.address, *imported_account);

    auto private_key = EncodePrivateKeyForExport(&service, account.address,
                                                 mojom::CoinType::ETH);
    EXPECT_TRUE(private_key);
    EXPECT_EQ(account.encoded_private_key, private_key);
  }
  base::RunLoop().RunUntilIdle();

  observer.Reset();
  EXPECT_FALSE(RemoveImportedAccount(&service, "", kPasswordBrave,
                                     mojom::CoinType::ETH));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.AccountsChangedFired());

  observer.Reset();
  EXPECT_FALSE(RemoveImportedAccount(&service, imported_accounts[1].address,
                                     kPasswordBrave123, mojom::CoinType::ETH));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.AccountsChangedFired());

  observer.Reset();
  EXPECT_TRUE(RemoveImportedAccount(&service, imported_accounts[1].address,
                                    kPasswordBrave, mojom::CoinType::ETH));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());

  observer.Reset();
  // remove invalid address
  EXPECT_FALSE(RemoveImportedAccount(&service, "0xxxxxxxxxx0", kPasswordBrave,
                                     mojom::CoinType::ETH));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.AccountsChangedFired());

  bool callback_called = false;
  service.GetKeyringInfo(
      brave_wallet::mojom::kDefaultKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_EQ(keyring_info->id, mojom::kDefaultKeyringId);
        EXPECT_TRUE(keyring_info->is_keyring_created);
        EXPECT_FALSE(keyring_info->is_locked);
        EXPECT_FALSE(keyring_info->is_backed_up);
        EXPECT_EQ(keyring_info->account_infos.size(), 3u);
        EXPECT_FALSE(keyring_info->account_infos[0]->address.empty());
        EXPECT_EQ(keyring_info->account_infos[0]->name, "Account 1");
        EXPECT_FALSE(keyring_info->account_infos[0]->is_imported);
        // import accounts
        EXPECT_EQ(keyring_info->account_infos[1]->address,
                  imported_accounts[0].address);
        EXPECT_EQ(keyring_info->account_infos[1]->name,
                  imported_accounts[0].name);
        EXPECT_TRUE(keyring_info->account_infos[1]->is_imported);

        EXPECT_EQ(keyring_info->account_infos[2]->address,
                  imported_accounts[2].address);
        EXPECT_EQ(keyring_info->account_infos[2]->name,
                  imported_accounts[2].name);
        EXPECT_TRUE(keyring_info->account_infos[2]->is_imported);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  service.Lock();
  // cannot get private key when locked
  auto private_key = EncodePrivateKeyForExport(
      &service, imported_accounts[0].address, mojom::CoinType::ETH);
  EXPECT_FALSE(private_key);

  EXPECT_TRUE(Unlock(&service, "brave"));

  callback_called = false;
  // Imported accounts should be restored
  service.GetKeyringInfo(
      brave_wallet::mojom::kDefaultKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_EQ(keyring_info->account_infos.size(), 3u);
        EXPECT_EQ(keyring_info->account_infos[1]->address,
                  imported_accounts[0].address);
        EXPECT_EQ(keyring_info->account_infos[1]->name,
                  imported_accounts[0].name);
        EXPECT_TRUE(keyring_info->account_infos[1]->is_imported);
        EXPECT_EQ(keyring_info->account_infos[2]->address,
                  imported_accounts[2].address);
        EXPECT_EQ(keyring_info->account_infos[2]->name,
                  imported_accounts[2].name);
        EXPECT_TRUE(keyring_info->account_infos[2]->is_imported);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Unlocked but with wrong password won't get private key.
  EXPECT_FALSE(EncodePrivateKeyForExport(&service, imported_accounts[0].address,
                                         mojom::CoinType::ETH,
                                         kPasswordBrave123));

  // private key should also be available now
  private_key = EncodePrivateKeyForExport(
      &service, imported_accounts[0].address, mojom::CoinType::ETH);
  EXPECT_TRUE(private_key);
  EXPECT_EQ(imported_accounts[0].private_key, *private_key);

  // Imported accounts should also be restored in default keyring
  auto* default_keyring =
      service.GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId);
  EXPECT_EQ(default_keyring->GetImportedAccountsNumber(), 2u);

  const base::Value* imported_accounts_value =
      KeyringService::GetPrefForKeyring(*GetPrefs(), kImportedAccounts,
                                        mojom::kDefaultKeyringId);
  ASSERT_TRUE(imported_accounts_value);
  EXPECT_EQ(*imported_accounts_value->GetList()[0].GetDict().FindString(
                kAccountAddress),
            imported_accounts[0].address);
  // private key is encrypted
  const std::string encrypted_private_key =
      *imported_accounts_value->GetList()[0].GetDict().FindString(
          kEncryptedPrivateKey);
  EXPECT_FALSE(encrypted_private_key.empty());

  std::vector<uint8_t> private_key0;
  ASSERT_TRUE(
      base::HexStringToBytes(imported_accounts[0].private_key, &private_key0));
  EXPECT_NE(encrypted_private_key, base::Base64Encode(private_key0));
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

  EXPECT_EQ(
      ImportAccountFromJson(&service, "Imported 1", "wrong password", json),
      absl::nullopt);

  EXPECT_EQ(ImportAccountFromJson(&service, "Imported 1", "testtest",
                                  "{crypto: 123}"),
            absl::nullopt);

  absl::optional<std::string> address =
      ImportAccountFromJson(&service, "Imported 1", "testtest", json);
  ASSERT_TRUE(address.has_value());
  EXPECT_EQ(*address, expected_address);

  service.Lock();
  EXPECT_TRUE(Unlock(&service, "brave"));

  // check restore by getting private key
  auto private_key = EncodePrivateKeyForExport(&service, expected_address,
                                               mojom::CoinType::ETH);
  EXPECT_TRUE(private_key);
  EXPECT_EQ(expected_private_key, *private_key);

  // private key is encrypted
  const base::Value* imported_accounts_value =
      KeyringService::GetPrefForKeyring(*GetPrefs(), kImportedAccounts,
                                        mojom::kDefaultKeyringId);
  ASSERT_TRUE(imported_accounts_value);
  const std::string encrypted_private_key =
      *imported_accounts_value->GetList()[0].GetDict().FindString(
          kEncryptedPrivateKey);
  EXPECT_FALSE(encrypted_private_key.empty());

  std::vector<uint8_t> private_key_bytes;
  ASSERT_TRUE(base::HexStringToBytes(expected_private_key, &private_key_bytes));
  EXPECT_NE(encrypted_private_key, base::Base64Encode(private_key_bytes));
}

TEST_F(KeyringServiceUnitTest, EncodePrivateKeyForExport) {
  base::test::ScopedFeatureList feature_list;
  base::FieldTrialParams parameters;
  parameters[features::kCreateDefaultSolanaAccount.name] = "false";

  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(
      brave_wallet::features::kBraveWalletSolanaFeature, parameters);

  feature_list.InitWithFeaturesAndParameters(enabled_features, {});

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(RestoreWallet(&service, kMnemonic1, "brave", false));

  // Can't get private key with wrong password.
  EXPECT_FALSE(EncodePrivateKeyForExport(
      &service, "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db",
      mojom::CoinType::ETH, kPasswordBrave123));

  absl::optional<std::string> private_key = EncodePrivateKeyForExport(
      &service, "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db",
      mojom::CoinType::ETH);
  ASSERT_TRUE(private_key.has_value());
  EXPECT_EQ(*private_key,
            "919af8081ce2a02d9650bf3e10ffb6b7cbadbb1dca749122d7d982cdb6cbcc50");

  // account not added yet
  EXPECT_FALSE(EncodePrivateKeyForExport(
      &service, "0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0",
      mojom::CoinType::ETH));
  ASSERT_TRUE(AddAccount(&service, "Account 2", mojom::CoinType::ETH));

  private_key = EncodePrivateKeyForExport(
      &service, "0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0",
      mojom::CoinType::ETH);
  ASSERT_TRUE(private_key.has_value());
  EXPECT_EQ(*private_key,
            "17c31fdade7d84f22462f398df300405a76fc11b1fe5a9e286dc8c3b0913e31c");

  EXPECT_FALSE(EncodePrivateKeyForExport(&service, "", mojom::CoinType::ETH));
  EXPECT_FALSE(
      EncodePrivateKeyForExport(&service, "0x123", mojom::CoinType::ETH));

  // Other keyrings
  // account not added yet
  EXPECT_FALSE(EncodePrivateKeyForExport(
      &service, "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
      mojom::CoinType::SOL));
  ASSERT_TRUE(AddAccount(&service, "Account 1", mojom::CoinType::SOL));
  // Wrong password.
  EXPECT_FALSE(EncodePrivateKeyForExport(
      &service, "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
      mojom::CoinType::SOL, kPasswordBrave123));
  private_key = EncodePrivateKeyForExport(
      &service, "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
      mojom::CoinType::SOL);
  ASSERT_TRUE(private_key.has_value());
  EXPECT_EQ(*private_key,
            "LNWjgQq8NhxWTUhz9jAD7koZfsKDwdJuLmVHyMxfjaFAamqXbtyUd3TcYQV2vPeRoM"
            "58gw7Ez8qsvKSZee6KdUQ");
}

TEST_F(KeyringServiceUnitTest, GetMainKeyringIdForCoin) {
  EXPECT_FALSE(KeyringService::GetKeyringIdForCoinNonFIL(mojom::CoinType::FIL));
  EXPECT_EQ(*KeyringService::GetKeyringIdForCoinNonFIL(mojom::CoinType::SOL),
            mojom::kSolanaKeyringId);
  EXPECT_EQ(*KeyringService::GetKeyringIdForCoinNonFIL(mojom::CoinType::ETH),
            mojom::kDefaultKeyringId);
}

TEST_F(KeyringServiceUnitTest, SetDefaultKeyringDerivedAccountMeta) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  const std::string updated_name = "Updated";
  ASSERT_FALSE(observer.AccountsChangedFired());

  // no keyring yet
  EXPECT_FALSE(SetKeyringDerivedAccountName(
      &service, mojom::kDefaultKeyringId,
      "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db", updated_name));

  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.AccountsChangedFired());
  observer.Reset();

  ASSERT_TRUE(RestoreWallet(&service, kMnemonic1, "brave", false));
  AddAccount(&service, "New Account", mojom::CoinType::ETH);

  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(observer.AccountsChangedFired());
  observer.Reset();

  auto account_infos =
      service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
  EXPECT_EQ(2u, account_infos.size());
  EXPECT_EQ("Account 1", account_infos[0]->name);
  EXPECT_EQ("New Account", account_infos[1]->name);
  const std::string address2 = account_infos[1]->address;

  // empty address
  EXPECT_FALSE(SetKeyringDerivedAccountName(&service, mojom::kDefaultKeyringId,
                                            "", updated_name));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.AccountsChangedFired());
  observer.Reset();

  ASSERT_FALSE(observer.AccountsChangedFired());
  // empty name
  EXPECT_FALSE(SetKeyringDerivedAccountName(&service, mojom::kDefaultKeyringId,
                                            address2, ""));

  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.AccountsChangedFired());
  observer.Reset();

  ASSERT_FALSE(observer.AccountsChangedFired());
  EXPECT_TRUE(SetKeyringDerivedAccountName(&service, mojom::kDefaultKeyringId,
                                           address2, updated_name));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();

  account_infos = service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
  EXPECT_EQ(2u, account_infos.size());
  EXPECT_EQ("Account 1", account_infos[0]->name);
  EXPECT_EQ(updated_name, account_infos[1]->name);
}

TEST_F(KeyringServiceUnitTest, SetDefaultKeyringImportedAccountName) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

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
  EXPECT_FALSE(SetKeyringImportedAccountName(&service, mojom::kDefaultKeyringId,
                                             imported_accounts[1].address,
                                             kUpdatedName));

  // Add import accounts.
  for (const auto& account : imported_accounts) {
    ASSERT_FALSE(observer.AccountsChangedFired());

    absl::optional<std::string> imported_account = ImportAccount(
        &service, account.name, account.private_key, mojom::CoinType::ETH);
    ASSERT_TRUE(imported_account.has_value());
    EXPECT_EQ(account.address, *imported_account);

    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(observer.AccountsChangedFired());
    observer.Reset();
  }

  // Empty address should fail.
  EXPECT_FALSE(SetKeyringImportedAccountName(&service, mojom::kDefaultKeyringId,
                                             "", kUpdatedName));

  // Empty name should fail.
  EXPECT_FALSE(SetKeyringImportedAccountName(&service, mojom::kDefaultKeyringId,
                                             imported_accounts[1].address, ""));

  // Update second imported account's name.
  EXPECT_TRUE(SetKeyringImportedAccountName(&service, mojom::kDefaultKeyringId,
                                            imported_accounts[1].address,
                                            kUpdatedName));

  // Private key of imported accounts should not be changed.
  for (const auto& imported_account : imported_accounts) {
    auto private_key = EncodePrivateKeyForExport(
        &service, imported_account.address, mojom::CoinType::ETH);
    EXPECT_TRUE(private_key);
    EXPECT_EQ(imported_account.private_key, *private_key);
  }

  // Only second imported account's name is updated.
  base::RunLoop run_loop;
  service.GetKeyringInfo(
      brave_wallet::mojom::kDefaultKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_EQ(keyring_info->id, mojom::kDefaultKeyringId);
        EXPECT_TRUE(keyring_info->is_keyring_created);
        EXPECT_FALSE(keyring_info->is_locked);
        EXPECT_FALSE(keyring_info->is_backed_up);
        EXPECT_EQ(keyring_info->account_infos.size(), 4u);
        EXPECT_FALSE(keyring_info->account_infos[0]->address.empty());
        EXPECT_EQ(keyring_info->account_infos[0]->name, "Account 1");
        EXPECT_FALSE(keyring_info->account_infos[0]->is_imported);
        EXPECT_EQ(keyring_info->account_infos[1]->address,
                  imported_accounts[0].address);
        EXPECT_EQ(keyring_info->account_infos[1]->name,
                  imported_accounts[0].name);
        EXPECT_TRUE(keyring_info->account_infos[1]->is_imported);
        EXPECT_EQ(keyring_info->account_infos[2]->address,
                  imported_accounts[1].address);
        EXPECT_EQ(keyring_info->account_infos[2]->name, kUpdatedName);
        EXPECT_TRUE(keyring_info->account_infos[2]->is_imported);
        EXPECT_EQ(keyring_info->account_infos[3]->address,
                  imported_accounts[2].address);
        EXPECT_EQ(keyring_info->account_infos[3]->name,
                  imported_accounts[2].name);
        EXPECT_TRUE(keyring_info->account_infos[3]->is_imported);
        run_loop.Quit();
      }));
  run_loop.Run();
}

TEST_F(KeyringServiceUnitTest, RestoreLegacyBraveWallet) {
  const char* mnemonic24 =
      "cushion pitch impact album daring marine much annual budget social "
      "clarify balance rose almost area busy among bring hidden bind later "
      "capable pulp laundry";
  const char* mnemonic12 =
      "drip caution abandon festival order clown oven regular absorb evidence "
      "crew where";
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  auto verify_restore_wallet = base::BindLambdaForTesting(
      [&service](const char* mnemonic, const char* address, bool is_legacy,
                 bool expect_result) {
        if (expect_result) {
          EXPECT_TRUE(RestoreWallet(&service, mnemonic, "brave1", is_legacy));
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
  // brave legacy menmonic can only be 24 words
  verify_restore_wallet.Run(mnemonic12, "", true, false);
  verify_restore_wallet.Run(
      mnemonic12, "0x084DCb94038af1715963F149079cE011C4B22961", false, true);
}

TEST_F(KeyringServiceUnitTest, HardwareAccounts) {
  base::test::ScopedFeatureList feature_list{
      features::kBraveWalletFilecoinFeature};

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  SetNetwork(mojom::kFilecoinMainnet, mojom::CoinType::FIL);

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  EXPECT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
  EXPECT_FALSE(service.IsLocked(mojom::kFilecoinKeyringId));
  EXPECT_FALSE(service.IsLocked(mojom::kFilecoinTestnetKeyringId));
  EXPECT_FALSE(service.IsLocked(mojom::kSolanaKeyringId));
  // Wallet is unlocked when there is no accounts of any types.
  EXPECT_FALSE(service.IsLockedSync());

  // We don't need to create wallet to use hardware accounts
  std::vector<mojom::HardwareWalletAccountPtr> new_accounts;
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0x111", "m/44'/60'/1'/0/0", "name 1", "Ledger", "device1",
      mojom::CoinType::ETH, absl::nullopt));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0x264", "m/44'/461'/0'/0/0", "name 2", "Ledger", "device1",
      mojom::CoinType::FIL, mojom::kFilecoinMainnet));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0xEA0", "m/44'/60'/2'/0/0", "name 3", "Ledger", "device2",
      mojom::CoinType::ETH, absl::nullopt));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0xFIL", "m/44'/461'/2'/0/0", "filecoin 1", "Ledger", "device2",
      mojom::CoinType::FIL, mojom::kFilecoinMainnet));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0x222", "m/44'/60'/3'/0/0", "name 4", "Ledger", "device1",
      mojom::CoinType::ETH, absl::nullopt));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0xFILTEST", "m/44'/1'/2'/0/0", "filecoin testnet 1", "Ledger", "device2",
      mojom::CoinType::FIL, mojom::kFilecoinTestnet));

  std::vector<mojom::HardwareWalletAccountPtr> accounts;
  base::ranges::transform(new_accounts, std::back_inserter(accounts),
                          [](const auto& account) { return account.Clone(); });

  EXPECT_FALSE(observer.AccountsChangedFired());
  service.AddHardwareAccounts(std::move(new_accounts));

  // ETH and FIL have hardware accounts
  EXPECT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
  EXPECT_EQ(*service.GetSelectedAccount(mojom::CoinType::ETH), "0x111");
  EXPECT_FALSE(service.IsLocked(mojom::kFilecoinKeyringId));
  EXPECT_EQ(*service.GetFilecoinSelectedAccount(mojom::kFilecoinMainnet),
            "0x264");
  EXPECT_FALSE(service.IsLocked(mojom::kFilecoinTestnetKeyringId));
  EXPECT_EQ(*service.GetFilecoinSelectedAccount(mojom::kFilecoinTestnet),
            "0xFILTEST");
  // SOL doesn't have any type of accounts
  EXPECT_FALSE(service.IsLocked(mojom::kSolanaKeyringId));
  EXPECT_FALSE(service.GetSelectedAccount(mojom::CoinType::SOL));

  EXPECT_TRUE(service.IsHardwareAccount(mojom::kDefaultKeyringId, "0x111"));
  EXPECT_FALSE(service.IsHardwareAccount(mojom::kSolanaKeyringId, "0x111"));
  EXPECT_TRUE(service.IsHardwareAccount(mojom::kFilecoinKeyringId, "0x264"));
  EXPECT_FALSE(service.IsHardwareAccount(mojom::kDefaultKeyringId, "0x264"));
  // Wallet is unlocked when the user has only hardware accounts
  EXPECT_FALSE(service.IsLockedSync());
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();
  for (const auto& account : accounts) {
    auto keyring_id =
        account->coin == mojom::CoinType::FIL
            ? brave_wallet::GetFilecoinKeyringId(*account->network)
            : *KeyringService::GetKeyringIdForCoinNonFIL(account->coin);
    auto path = keyring_id + ".hardware." + account->device_id +
                ".account_metas." + account->address;
    ASSERT_TRUE(
        GetPrefs()->GetDict(kBraveWalletKeyrings).FindByDottedPath(path));
  }
  {
    // Checking Default keyring accounts
    base::RunLoop run_loop;
    service.GetKeyringInfo(
        brave_wallet::mojom::kDefaultKeyringId,
        base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
          const auto& accounts = keyring_info->account_infos;
          EXPECT_EQ(accounts.size(), 3u);

          EXPECT_EQ(accounts[0]->address, "0x111");
          EXPECT_EQ(accounts[0]->name, "name 1");
          EXPECT_EQ(accounts[0]->is_imported, false);
          ASSERT_TRUE(accounts[0]->hardware);
          EXPECT_EQ(accounts[0]->hardware->device_id, "device1");
          EXPECT_EQ(accounts[0]->coin, mojom::CoinType::ETH);

          EXPECT_EQ(accounts[1]->address, "0x222");
          EXPECT_EQ(accounts[1]->name, "name 4");
          EXPECT_EQ(accounts[1]->is_imported, false);
          ASSERT_TRUE(accounts[1]->hardware);
          EXPECT_EQ(accounts[1]->hardware->device_id, "device1");
          EXPECT_EQ(accounts[1]->coin, mojom::CoinType::ETH);

          EXPECT_EQ(accounts[2]->address, "0xEA0");
          EXPECT_EQ(accounts[2]->name, "name 3");
          EXPECT_EQ(accounts[2]->is_imported, false);
          ASSERT_TRUE(accounts[2]->hardware);
          EXPECT_EQ(accounts[2]->hardware->device_id, "device2");
          EXPECT_EQ(accounts[2]->coin, mojom::CoinType::ETH);

          run_loop.Quit();
        }));
    run_loop.Run();
  }
  {
    // Checking Filecoin keyring accounts
    base::RunLoop run_loop;
    service.GetKeyringInfo(
        brave_wallet::mojom::kFilecoinKeyringId,
        base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
          const auto& accounts = keyring_info->account_infos;
          EXPECT_EQ(accounts.size(), 2u);

          EXPECT_EQ(accounts[0]->address, "0x264");
          EXPECT_EQ(accounts[0]->name, "name 2");
          EXPECT_EQ(accounts[0]->is_imported, false);
          ASSERT_TRUE(accounts[0]->hardware);
          EXPECT_EQ(accounts[0]->hardware->device_id, "device1");
          EXPECT_EQ(accounts[0]->coin, mojom::CoinType::FIL);

          EXPECT_EQ(accounts[1]->address, "0xFIL");
          EXPECT_EQ(accounts[1]->name, "filecoin 1");
          EXPECT_EQ(accounts[1]->is_imported, false);
          ASSERT_TRUE(accounts[1]->hardware);
          EXPECT_EQ(accounts[1]->hardware->device_id, "device2");
          EXPECT_EQ(accounts[1]->coin, mojom::CoinType::FIL);

          run_loop.Quit();
        }));
    run_loop.Run();
  }
  {
    // Checking Filecoin keyring testnet accounts
    base::RunLoop run_loop;
    service.GetKeyringInfo(
        brave_wallet::mojom::kFilecoinTestnetKeyringId,
        base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
          const auto& accounts = keyring_info->account_infos;
          EXPECT_EQ(accounts.size(), 1u);

          EXPECT_EQ(accounts[0]->address, "0xFILTEST");
          EXPECT_EQ(accounts[0]->name, "filecoin testnet 1");
          EXPECT_EQ(accounts[0]->is_imported, false);
          ASSERT_TRUE(accounts[0]->hardware);
          EXPECT_EQ(accounts[0]->hardware->device_id, "device2");
          EXPECT_EQ(accounts[0]->coin, mojom::CoinType::FIL);

          run_loop.Quit();
        }));
    run_loop.Run();
  }
  ASSERT_FALSE(observer.AccountsChangedFired());

  EXPECT_FALSE(RemoveHardwareAccount(&service, "", mojom::CoinType::ETH));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.AccountsChangedFired());
  observer.Reset();

  EXPECT_TRUE(RemoveHardwareAccount(&service, "0x111", mojom::CoinType::ETH));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();

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

  ASSERT_FALSE(observer.AccountsChangedFired());
  EXPECT_TRUE(RemoveHardwareAccount(&service, "0xEA0", mojom::CoinType::ETH));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();

  bool callback_called = false;
  service.GetKeyringInfo(
      brave_wallet::mojom::kDefaultKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        const auto& accounts = keyring_info->account_infos;
        EXPECT_EQ(accounts.size(), size_t(1));

        EXPECT_EQ(accounts[0]->address, "0x222");
        EXPECT_EQ(accounts[0]->name, "name 4");
        EXPECT_EQ(accounts[0]->is_imported, false);
        ASSERT_TRUE(accounts[0]->hardware);
        EXPECT_EQ(accounts[0]->hardware->device_id, "device1");
        EXPECT_EQ(accounts[0]->coin, mojom::CoinType::ETH);

        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  ASSERT_FALSE(observer.AccountsChangedFired());

  EXPECT_TRUE(RemoveHardwareAccount(&service, "0x222", mojom::CoinType::ETH));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();

  ASSERT_FALSE(
      GetPrefs()
          ->GetDict(kBraveWalletKeyrings)
          .FindByDottedPath("default.hardware.device2.account_metas.0xEA0"));

  ASSERT_FALSE(GetPrefs()
                   ->GetDict(kBraveWalletKeyrings)
                   .FindByDottedPath("default.hardware.device2"));

  EXPECT_TRUE(RemoveHardwareAccount(&service, "0xFIL", mojom::CoinType::FIL));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();
  ASSERT_FALSE(
      GetPrefs()
          ->GetDict(kBraveWalletKeyrings)
          .FindByDottedPath("filecoin.hardware.device2.account_metas.0xFIL"));

  EXPECT_TRUE(
      RemoveHardwareAccount(&service, "0xFILTEST", mojom::CoinType::FIL));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();
  ASSERT_FALSE(
      GetPrefs()
          ->GetDict(kBraveWalletKeyrings)
          .FindByDottedPath(
              "filecoin_testnet.hardware.device2.account_metas.0xFILTEST"));

  ASSERT_TRUE(CreateWallet(&service, "brave"));
  auto* default_keyring = service.GetHDKeyringById(mojom::kDefaultKeyringId);
  std::string first_account = default_keyring->GetAddress(0);
  EXPECT_FALSE(
      service.IsHardwareAccount(mojom::kDefaultKeyringId, first_account));

  EXPECT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
  // Selected account changed when creating wallet
  EXPECT_EQ(*service.GetSelectedAccount(mojom::CoinType::ETH), first_account);

  service.Lock();
  EXPECT_TRUE(service.IsLocked(mojom::kDefaultKeyringId));
  EXPECT_FALSE(service.IsLocked(mojom::kFilecoinKeyringId));
  EXPECT_FALSE(service.IsLocked(mojom::kFilecoinTestnetKeyringId));
  EXPECT_TRUE(service.IsLocked(mojom::kSolanaKeyringId));
  // Wallet is locked when the user has both software and hardware accounts
  EXPECT_TRUE(service.IsLockedSync());
}

TEST_F(KeyringServiceUnitTest, AutoLock) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  absl::optional<std::string> mnemonic = CreateWallet(&service, "brave");
  ASSERT_TRUE(mnemonic.has_value());
  ASSERT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));

  // Should not be locked yet after 4 minutes
  task_environment_.FastForwardBy(base::Minutes(4));
  ASSERT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));

  // After the 5th minute, it should be locked
  task_environment_.FastForwardBy(base::Minutes(1));
  ASSERT_TRUE(service.IsLocked(mojom::kDefaultKeyringId));
  // Locking after it is auto locked won't cause a crash
  service.Lock();
  ASSERT_TRUE(service.IsLocked(mojom::kDefaultKeyringId));

  // Unlocking will reset the timer
  EXPECT_TRUE(Unlock(&service, "brave"));
  ASSERT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
  task_environment_.FastForwardBy(base::Minutes(5));
  ASSERT_TRUE(service.IsLocked(mojom::kDefaultKeyringId));

  // Locking before the timer fires won't cause any problems after the
  // timer fires.
  EXPECT_TRUE(Unlock(&service, "brave"));
  ASSERT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
  task_environment_.FastForwardBy(base::Minutes(1));
  service.Lock();
  ASSERT_TRUE(service.IsLocked(mojom::kDefaultKeyringId));
  task_environment_.FastForwardBy(base::Minutes(4));
  ASSERT_TRUE(service.IsLocked(mojom::kDefaultKeyringId));

  // Restoring keyring will auto lock too
  service.Reset();
  ASSERT_TRUE(RestoreWallet(&service, *mnemonic, "brave", false));
  ASSERT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
  task_environment_.FastForwardBy(base::Minutes(6));
  ASSERT_TRUE(service.IsLocked(mojom::kDefaultKeyringId));

  // Changing the auto lock pref should reset the timer
  EXPECT_TRUE(Unlock(&service, "brave"));
  ASSERT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
  task_environment_.FastForwardBy(base::Minutes(4));
  GetPrefs()->SetInteger(kBraveWalletAutoLockMinutes, 3);
  task_environment_.FastForwardBy(base::Minutes(2));
  EXPECT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
  task_environment_.FastForwardBy(base::Minutes(1));
  EXPECT_TRUE(service.IsLocked(mojom::kDefaultKeyringId));

  // Changing the auto lock pref should reset the timer even if higher
  // for simplicity of logic
  EXPECT_TRUE(Unlock(&service, "brave"));
  ASSERT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
  task_environment_.FastForwardBy(base::Minutes(2));
  EXPECT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
  GetPrefs()->SetInteger(kBraveWalletAutoLockMinutes, 10);
  task_environment_.FastForwardBy(base::Minutes(9));
  EXPECT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
  task_environment_.FastForwardBy(base::Minutes(1));
  EXPECT_TRUE(service.IsLocked(mojom::kDefaultKeyringId));
}

TEST_F(KeyringServiceUnitTest, NotifyUserInteraction) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(CreateWallet(&service, "brave"));
  ASSERT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));

  // Notifying of user interaction should keep the wallet unlocked
  task_environment_.FastForwardBy(base::Minutes(4));
  service.NotifyUserInteraction();
  task_environment_.FastForwardBy(base::Minutes(1));
  service.NotifyUserInteraction();
  task_environment_.FastForwardBy(base::Minutes(4));
  ASSERT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
  task_environment_.FastForwardBy(base::Minutes(1));
  ASSERT_TRUE(service.IsLocked(mojom::kDefaultKeyringId));
}

TEST_F(KeyringServiceUnitTest, SelectAddedAccount) {
  base::test::ScopedFeatureList feature_list;
  base::FieldTrialParams parameters;
  parameters[features::kCreateDefaultSolanaAccount.name] = "false";

  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(
      brave_wallet::features::kBraveWalletSolanaFeature, parameters);

  feature_list.InitWithFeaturesAndParameters(enabled_features, {});

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(CreateWallet(&service, "brave"));

  AddAccount(&service, "eth acc 1", mojom::CoinType::ETH);
  AddAccount(&service, "eth acc 2", mojom::CoinType::ETH);
  AddAccount(&service, "eth acc 3", mojom::CoinType::ETH);

  AddAccount(&service, "sol acc 1", mojom::CoinType::SOL);
  AddAccount(&service, "sol acc 2", mojom::CoinType::SOL);
  AddAccount(&service, "sol acc 3", mojom::CoinType::SOL);

  service.GetKeyringInfo(
      mojom::kDefaultKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        ASSERT_EQ(GetSelectedAccount(&service, mojom::CoinType::ETH),
                  keyring_info->account_infos[3]->address);
      }));

  service.GetKeyringInfo(
      mojom::kSolanaKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        ASSERT_EQ(GetSelectedAccount(&service, mojom::CoinType::SOL),
                  keyring_info->account_infos[2]->address);
      }));
}

TEST_F(KeyringServiceUnitTest, SelectAddedFilecoinAccount) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(CreateWallet(&service, "brave"));

#if !BUILDFLAG(IS_ANDROID)
  AddFilecoinAccount(&service, "fil acc 1", mojom::kFilecoinMainnet);
  AddFilecoinAccount(&service, "fil acc 2", mojom::kFilecoinMainnet);
  AddFilecoinAccount(&service, "fil acc 3", mojom::kFilecoinMainnet);

  AddFilecoinAccount(&service, "fil acc 1", mojom::kFilecoinTestnet);
  AddFilecoinAccount(&service, "fil acc 2", mojom::kFilecoinTestnet);
  AddFilecoinAccount(&service, "fil acc 3", mojom::kFilecoinTestnet);

  service.GetKeyringInfo(
      mojom::kFilecoinKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        ASSERT_EQ(GetFilecoinSelectedAccount(&service, mojom::kFilecoinMainnet),
                  keyring_info->account_infos[2]->address);
      }));

  service.GetKeyringInfo(
      mojom::kFilecoinTestnetKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        ASSERT_EQ(GetFilecoinSelectedAccount(&service, mojom::kFilecoinTestnet),
                  keyring_info->account_infos[2]->address);
      }));
#else
  ASSERT_FALSE(
      AddFilecoinAccount(&service, "fil acc 1", mojom::kFilecoinMainnet));
  ASSERT_FALSE(
      AddFilecoinAccount(&service, "fil acc 3", mojom::kFilecoinTestnet));
#endif
}

TEST_F(KeyringServiceUnitTest, SelectImportedFilecoinAccount) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(CreateWallet(&service, "brave"));
  ASSERT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));

#if !BUILDFLAG(IS_ANDROID)
  ImportFilecoinAccount(&service, "fil m acc 1",
                        "7b2254797065223a22736563703235366b31222c22507269766174"
                        "654b6579223a224169776f6a344469323155316844776835735348"
                        "434d7a37342b346c45303472376e5349454d706d6258493d227d",
                        mojom::kFilecoinMainnet);

  ImportFilecoinAccount(&service, "fil m acc 1",
                        "7b2254797065223a22736563703235366b31222c22507269766174"
                        "654b6579223a226376414367502f53344f3274796c4f42466a6348"
                        "33583154373677696661456c6646435057612b6a474a453d227d",
                        mojom::kFilecoinMainnet);

  service.GetKeyringInfo(
      mojom::kFilecoinKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        ASSERT_EQ(GetFilecoinSelectedAccount(&service, mojom::kFilecoinMainnet),
                  keyring_info->account_infos[1]->address);
      }));

  ImportFilecoinAccount(&service, "fil t acc 2",
                        "7b2254797065223a22736563703235366b31222c22507269766174"
                        "654b6579223a226376414367502f53344f3274796c4f42466a6348"
                        "33583154373677696661456c6646435057612b6a474a453d227d",
                        mojom::kFilecoinTestnet);

  ImportFilecoinAccount(&service, "fil t acc 2",
                        "7b2254797065223a22736563703235366b31222c22507269766174"
                        "654b6579223a224169776f6a344469323155316844776835735348"
                        "434d7a37342b346c45303472376e5349454d706d6258493d227d",
                        mojom::kFilecoinTestnet);
  service.GetKeyringInfo(
      mojom::kFilecoinTestnetKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        ASSERT_EQ(GetFilecoinSelectedAccount(&service, mojom::kFilecoinTestnet),
                  keyring_info->account_infos[1]->address);
      }));
#else
  ASSERT_EQ(absl::nullopt,
            ImportFilecoinAccount(
                &service, "fil m acc 1",
                "7b2254797065223a22736563703235366b31222c22507269766174"
                "654b6579223a224169776f6a344469323155316844776835735348"
                "434d7a37342b346c45303472376e5349454d706d6258493d227d",
                mojom::kFilecoinMainnet));
  ASSERT_EQ(absl::nullopt,
            ImportFilecoinAccount(
                &service, "fil t acc 2",
                "7b2254797065223a22736563703235366b31222c22507269766174"
                "654b6579223a224169776f6a344469323155316844776835735348"
                "434d7a37342b346c45303472376e5349454d706d6258493d227d",
                mojom::kFilecoinTestnet));
#endif  // !BUILDFLAG(IS_ANDROID)
}

TEST_F(KeyringServiceUnitTest, SelectImportedAccount) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(CreateWallet(&service, "brave"));

  ImportAccount(
      &service, "Best Evil Son",
      "d118a12a1e3b595d7d9e5599370df4ddc58d246a3ae4a795597e50eb6a32afb5",
      mojom::CoinType::ETH);

  ImportAccount(
      &service, "Best Evil Son 2",
      "5b48615b7e43d015c3de46cbe9bc01bff9e106277a91bd44a55f9c4b1a268314",
      mojom::CoinType::ETH);

  service.GetKeyringInfo(
      mojom::kDefaultKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        ASSERT_EQ(GetSelectedAccount(&service, mojom::CoinType::ETH),
                  keyring_info->account_infos[2]->address);
      }));
}

TEST_F(KeyringServiceUnitTest, SelectHardwareAccount) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(CreateWallet(&service, "brave"));

  std::vector<mojom::HardwareWalletAccountPtr> new_accounts;
  std::string hardware_account1 = "0x1111111111111111111111111111111111111111";
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      hardware_account1, "m/44'/60'/1'/0/0", "name 1", "Ledger", "device1",
      mojom::CoinType::ETH, absl::nullopt));
  std::string hardware_account2 = "0x2222222222222222222222222222222222222222";
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      hardware_account2, "m/44'/60'/1'/0/0", "name 2", "Ledger", "device1",
      mojom::CoinType::ETH, absl::nullopt));

  service.AddHardwareAccounts(std::move(new_accounts));

  service.GetKeyringInfo(
      mojom::kDefaultKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        ASSERT_EQ(GetSelectedAccount(&service, mojom::CoinType::ETH),
                  keyring_info->account_infos[1]->address);
      }));
}

TEST_F(KeyringServiceUnitTest, SetSelectedAccount) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeatures(
      {brave_wallet::features::kBraveWalletFilecoinFeature,
       brave_wallet::features::kBraveWalletSolanaFeature},
      {});

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  SetNetwork(brave_wallet::mojom::kFilecoinTestnet,
             brave_wallet::mojom::CoinType::FIL);

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  ASSERT_TRUE(CreateWallet(&service, "brave"));
  auto* default_keyring =
      service.GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId);
  std::string first_account = default_keyring->GetAddress(0);
  ASSERT_TRUE(
      AddAccount(&service, "Who does number 2 work for", mojom::CoinType::ETH));
  std::string second_account = default_keyring->GetAddress(1);

  // This does not depend on being locked
  EXPECT_TRUE(Lock(&service));

  // No account set as the default
  EXPECT_EQ(second_account, GetSelectedAccount(&service, mojom::CoinType::ETH));

  // Setting account to a valid address works
  EXPECT_TRUE(SetSelectedAccount(&service, &observer, second_account,
                                 mojom::CoinType::ETH));
  EXPECT_EQ(second_account, GetSelectedAccount(&service, mojom::CoinType::ETH));

  // Setting account to a non-existing account doesn't work
  EXPECT_FALSE(SetSelectedAccount(&service, &observer,
                                  "0xf83C3cBfF68086F276DD4f87A82DF73B57b21559",
                                  mojom::CoinType::ETH));
  EXPECT_EQ(second_account, GetSelectedAccount(&service, mojom::CoinType::ETH));

  // Can import only when unlocked.
  // Then check that the account can be set to an imported account.
  EXPECT_TRUE(Unlock(&service, "brave"));
  absl::optional<std::string> imported_account = ImportAccount(
      &service, "Best Evil Son",
      // 0xDc06aE500aD5ebc5972A0D8Ada4733006E905976
      "d118a12a1e3b595d7d9e5599370df4ddc58d246a3ae4a795597e50eb6a32afb5",
      mojom::CoinType::ETH);
  ASSERT_TRUE(imported_account.has_value());
  EXPECT_TRUE(Lock(&service));
  EXPECT_TRUE(SetSelectedAccount(&service, &observer, *imported_account,
                                 mojom::CoinType::ETH));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(*imported_account,
            GetSelectedAccount(&service, mojom::CoinType::ETH));
  // Removing the imported account resets to no selected account
  observer.Reset();
  EXPECT_TRUE(Unlock(&service, "brave"));
  EXPECT_TRUE(RemoveImportedAccount(
      &service, "0xDc06aE500aD5ebc5972A0D8Ada4733006E905976", kPasswordBrave,
      mojom::CoinType::ETH));
  EXPECT_TRUE(Lock(&service));
  EXPECT_EQ(absl::nullopt, GetSelectedAccount(&service, mojom::CoinType::ETH));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.SelectedAccountChangedFired(mojom::CoinType::ETH));
  observer.Reset();

  // Can set hardware account
  std::vector<mojom::HardwareWalletAccountPtr> new_accounts;
  std::string hardware_account = "0x1111111111111111111111111111111111111111";
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      hardware_account, "m/44'/60'/1'/0/0", "name 1", "Ledger", "device1",
      mojom::CoinType::ETH, absl::nullopt));
  service.AddHardwareAccounts(std::move(new_accounts));
  EXPECT_TRUE(SetSelectedAccount(&service, &observer, hardware_account,
                                 mojom::CoinType::ETH));
  observer.Reset();

  EXPECT_TRUE(Unlock(&service, "brave"));
  // Can set Filecoin account
  {
    absl::optional<std::string> fil_imported_account = ImportFilecoinAccount(
        &service, "Imported Filecoin account 1",
        // t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q
        "7b2254797065223a22736563703235366b31222c22507269766174654b6579223a2257"
        "6b4545645a45794235364b5168512b453338786a7663464c2b545a4842464e732b696a"
        "58533535794b383d227d",
        mojom::kFilecoinTestnet);
    ASSERT_TRUE(fil_imported_account.has_value());
    EXPECT_EQ(*fil_imported_account,
              "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q");
    EXPECT_TRUE(SetSelectedAccount(&service, &observer,
                                   fil_imported_account.value(),
                                   mojom::CoinType::FIL));
    EXPECT_EQ(fil_imported_account.value(),
              GetFilecoinSelectedAccount(&service, mojom::kFilecoinTestnet));
  }
  // Can set Solana account
  {
    // lazily create keyring when importing SOL account
    absl::optional<std::string> sol_imported_account = ImportAccount(
        &service, "Imported Account 1",
        // C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ
        "sCzwsBKmKtk5Hgb4YUJAduQ5nmJq4GTyzCXhrKonAGaexa83MgSZuTSMS6TSZTndnC"
        "YbQtaJQKLXET9jVjepWXe",
        mojom::CoinType::SOL);
    ASSERT_TRUE(sol_imported_account.has_value());
    EXPECT_EQ(*sol_imported_account,
              "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ");
    EXPECT_TRUE(SetSelectedAccount(&service, &observer,
                                   sol_imported_account.value(),
                                   mojom::CoinType::SOL));
    EXPECT_EQ(sol_imported_account.value(),
              GetSelectedAccount(&service, mojom::CoinType::SOL));
  }
  EXPECT_EQ(hardware_account,
            GetSelectedAccount(&service, mojom::CoinType::ETH));
  EXPECT_EQ("t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
            GetFilecoinSelectedAccount(&service, mojom::kFilecoinTestnet));
  EXPECT_EQ("C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ",
            GetSelectedAccount(&service, mojom::CoinType::SOL));

  EXPECT_TRUE(RemoveImportedAccount(&service,
                                    "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                                    kPasswordBrave, mojom::CoinType::FIL));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.SelectedAccountChangedFired(mojom::CoinType::FIL));
  observer.Reset();

  EXPECT_TRUE(
      RemoveHardwareAccount(&service, hardware_account, mojom::CoinType::ETH));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.SelectedAccountChangedFired(mojom::CoinType::ETH));
  observer.Reset();
  EXPECT_TRUE(RemoveImportedAccount(
      &service, "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ", kPasswordBrave,
      mojom::CoinType::SOL));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.SelectedAccountChangedFired(mojom::CoinType::SOL));
  observer.Reset();
}

TEST_F(KeyringServiceUnitTest, AddAccountsWithDefaultName) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(CreateWallet(&service, "brave"));
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));

  ASSERT_TRUE(AddAccount(&service, "AccountAAAAH", mojom::CoinType::ETH));

  service.AddAccountsWithDefaultName(3);

  base::RunLoop run_loop;
  service.GetKeyringInfo(
      mojom::kDefaultKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_EQ(keyring_info->id, mojom::kDefaultKeyringId);
        EXPECT_TRUE(keyring_info->is_keyring_created);
        EXPECT_EQ(keyring_info->account_infos.size(), 5u);
        EXPECT_FALSE(keyring_info->account_infos[0]->address.empty());
        EXPECT_EQ(keyring_info->account_infos[0]->name, "Account 1");
        EXPECT_EQ(keyring_info->account_infos[1]->name, "AccountAAAAH");
        EXPECT_EQ(keyring_info->account_infos[2]->name, "Account 3");
        EXPECT_EQ(keyring_info->account_infos[3]->name, "Account 4");
        EXPECT_EQ(keyring_info->account_infos[4]->name, "Account 5");
        run_loop.Quit();
      }));
  run_loop.Run();
}

TEST_F(KeyringServiceUnitTest, SignMessageByDefaultKeyring) {
  // HDKeyringUnitTest.SignMessage already tests the correctness of signature
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(RestoreWallet(&service, kMnemonic1, "brave", false));
  ASSERT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));

  std::string account1;
  {
    base::RunLoop run_loop;
    service.GetKeyringInfo(
        mojom::kDefaultKeyringId,
        base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
          ASSERT_EQ(keyring_info->account_infos.size(), 1u);
          account1 = keyring_info->account_infos[0]->address;
          run_loop.Quit();
        }));
    run_loop.Run();
  }
  const std::vector<uint8_t> message = {0xde, 0xad, 0xbe, 0xef};
  auto sig_with_err = service.SignMessageByDefaultKeyring(account1, message);
  EXPECT_NE(sig_with_err.signature, absl::nullopt);
  EXPECT_FALSE(sig_with_err.signature->empty());
  EXPECT_TRUE(sig_with_err.error_message.empty());

  // message is 0x
  sig_with_err =
      service.SignMessageByDefaultKeyring(account1, std::vector<uint8_t>());
  EXPECT_NE(sig_with_err.signature, absl::nullopt);
  EXPECT_FALSE(sig_with_err.signature->empty());
  EXPECT_TRUE(sig_with_err.error_message.empty());

  // not a valid account in this wallet
  const std::vector<std::string> invalid_accounts(
      {"0xea3C17c81E3baC3472d163b2c8b12ddDAa027874", "", "0x1234"});
  for (const auto& invalid_account : invalid_accounts) {
    sig_with_err =
        service.SignMessageByDefaultKeyring(invalid_account, message);
    EXPECT_EQ(sig_with_err.signature, absl::nullopt);
    EXPECT_EQ(
        sig_with_err.error_message,
        l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_SIGN_MESSAGE_INVALID_ADDRESS,
                                  base::ASCIIToUTF16(invalid_account)));
  }

  // Cannot sign message when locked
  service.Lock();
  sig_with_err = service.SignMessageByDefaultKeyring(account1, message);
  EXPECT_EQ(sig_with_err.signature, absl::nullopt);
  EXPECT_EQ(
      sig_with_err.error_message,
      l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SIGN_MESSAGE_UNLOCK_FIRST));
}

TEST_F(KeyringServiceUnitTest, GetSetAutoLockMinutes) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  EXPECT_EQ(5, GetAutoLockMinutes(&service));
  EXPECT_TRUE(SetAutoLockMinutes(&service, &observer, 7));
  EXPECT_EQ(7, GetAutoLockMinutes(&service));
  EXPECT_TRUE(SetAutoLockMinutes(&service, &observer, 3));
  EXPECT_EQ(3, GetAutoLockMinutes(&service));

  // Out of bound values cannot be set
  EXPECT_FALSE(
      SetAutoLockMinutes(&service, &observer, kAutoLockMinutesMin - 1));
  EXPECT_EQ(3, GetAutoLockMinutes(&service));
  EXPECT_FALSE(
      SetAutoLockMinutes(&service, &observer, kAutoLockMinutesMax + 1));
  EXPECT_EQ(3, GetAutoLockMinutes(&service));

  // Bound values can be set
  EXPECT_TRUE(SetAutoLockMinutes(&service, &observer, kAutoLockMinutesMin));
  EXPECT_EQ(kAutoLockMinutesMin, GetAutoLockMinutes(&service));
  EXPECT_TRUE(SetAutoLockMinutes(&service, &observer, kAutoLockMinutesMax));
  EXPECT_EQ(kAutoLockMinutesMax, GetAutoLockMinutes(&service));
}

TEST_F(KeyringServiceUnitTest, UpdateNameForHardwareAccountSync) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  ASSERT_TRUE(CreateWallet(&service, "brave"));

  std::vector<mojom::HardwareWalletAccountPtr> new_accounts;
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0x111", "m/44'/60'/1'/0/0", "name 1", "Ledger", "device1",
      mojom::CoinType::ETH, absl::nullopt));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0x264", "m/44'/461'/0'/0/0", "name 2", "Ledger", "device1",
      mojom::CoinType::FIL, mojom::kFilecoinMainnet));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0xEA0", "m/44'/60'/2'/0/0", "name 3", "Ledger", "device2",
      mojom::CoinType::ETH, absl::nullopt));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0xFIL", "m/44'/461'/2'/0/0", "filecoin 1", "Ledger", "device2",
      mojom::CoinType::FIL, mojom::kFilecoinMainnet));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0x222", "m/44'/60'/3'/0/0", "name 4", "Ledger", "device1",
      mojom::CoinType::ETH, absl::nullopt));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0xFILTEST", "m/44'/1'/2'/0/0", "filecoin testnet 1", "Ledger", "device2",
      mojom::CoinType::FIL, mojom::kFilecoinTestnet));

  service.AddHardwareAccounts(std::move(new_accounts));

  UpdateNameForHardwareAccount(&service, "0x111", "name 1 changed",
                               mojom::CoinType::ETH);
  UpdateNameForHardwareAccount(&service, "0xFIL", "filecoin 1 changed",
                               mojom::CoinType::FIL);
  UpdateNameForHardwareAccount(&service, "0xFILTEST",
                               "filecoin testnet 1 changed",
                               mojom::CoinType::FIL);

  {
    base::RunLoop run_loop;
    service.GetKeyringInfo(
        mojom::kDefaultKeyringId,
        base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
          EXPECT_FALSE(keyring_info->account_infos[1]->address.empty());
          EXPECT_EQ(keyring_info->account_infos[1]->name, "name 1 changed");
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  {
    base::RunLoop run_loop;
    service.GetKeyringInfo(
        mojom::kFilecoinKeyringId,
        base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
          EXPECT_FALSE(keyring_info->account_infos[1]->address.empty());
          EXPECT_EQ(keyring_info->account_infos[1]->name, "filecoin 1 changed");
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  {
    base::RunLoop run_loop;
    service.GetKeyringInfo(
        mojom::kFilecoinTestnetKeyringId,
        base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
          EXPECT_FALSE(keyring_info->account_infos[0]->address.empty());
          EXPECT_EQ(keyring_info->account_infos[0]->name,
                    "filecoin testnet 1 changed");
          run_loop.Quit();
        }));
    run_loop.Run();
  }
}

TEST_F(KeyringServiceUnitTest, SetDefaultKeyringHardwareAccountName) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  ASSERT_TRUE(CreateWallet(&service, "brave"));

  const struct {
    const char* address;
    const char* derivation_path;
    const char* name;
    const char* vendor;
    const char* device_id;
    mojom::CoinType coin;
  } hardware_accounts[] = {{"0x111", "m/44'/60'/1'/0/0", "name 1", "Ledger",
                            "device1", mojom::CoinType::ETH},
                           {"0x264", "m/44'/60'/2'/0/0", "name 2", "Ledger",
                            "device1", mojom::CoinType::ETH},
                           {"0xEA0", "m/44'/60'/3'/0/0", "name 3", "Ledger",
                            "device2", mojom::CoinType::ETH}};

  std::vector<mojom::HardwareWalletAccountPtr> new_accounts;
  for (const auto& it : hardware_accounts) {
    new_accounts.push_back(mojom::HardwareWalletAccount::New(
        it.address, it.derivation_path, it.name, it.vendor, it.device_id,
        it.coin, absl::nullopt));
  }

  const std::string kUpdatedName = "Updated ledger accoount 2";

  // Fail when no hardware accounts.
  EXPECT_FALSE(SetHardwareAccountName(&service, hardware_accounts[1].address,
                                      kUpdatedName, hardware_accounts[1].coin));

  service.AddHardwareAccounts(std::move(new_accounts));

  // Empty address should fail.
  EXPECT_FALSE(SetHardwareAccountName(&service, "", kUpdatedName,
                                      hardware_accounts[1].coin));

  // Empty name should fail.
  EXPECT_FALSE(SetHardwareAccountName(&service, hardware_accounts[1].address,
                                      "", hardware_accounts[1].coin));

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  // Update second hardware account's name.
  EXPECT_TRUE(SetHardwareAccountName(&service, hardware_accounts[1].address,
                                     kUpdatedName, hardware_accounts[1].coin));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();

  // Only second hardware account's name is updated.
  base::RunLoop run_loop;
  service.GetKeyringInfo(
      mojom::kDefaultKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_EQ(keyring_info->id, mojom::kDefaultKeyringId);
        EXPECT_TRUE(keyring_info->is_keyring_created);
        EXPECT_FALSE(keyring_info->is_locked);
        EXPECT_FALSE(keyring_info->is_backed_up);
        EXPECT_EQ(keyring_info->account_infos.size(), 4u);
        EXPECT_FALSE(keyring_info->account_infos[0]->address.empty());
        EXPECT_EQ(keyring_info->account_infos[0]->name, "Account 1");
        EXPECT_FALSE(keyring_info->account_infos[0]->hardware);
        EXPECT_EQ(keyring_info->account_infos[1]->address,
                  hardware_accounts[0].address);
        EXPECT_EQ(keyring_info->account_infos[1]->name,
                  hardware_accounts[0].name);
        EXPECT_TRUE(keyring_info->account_infos[1]->hardware);
        EXPECT_EQ(keyring_info->account_infos[2]->address,
                  hardware_accounts[1].address);
        EXPECT_EQ(keyring_info->account_infos[2]->name, kUpdatedName);
        EXPECT_TRUE(keyring_info->account_infos[2]->hardware);
        EXPECT_EQ(keyring_info->account_infos[3]->address,
                  hardware_accounts[2].address);
        EXPECT_EQ(keyring_info->account_infos[3]->name,
                  hardware_accounts[2].name);
        EXPECT_TRUE(keyring_info->account_infos[3]->hardware);
        run_loop.Quit();
      }));
  run_loop.Run();
}

TEST_F(KeyringServiceUnitTest, IsStrongPassword) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  // Strong password that meets all requirements passes
  EXPECT_TRUE(IsStrongPassword(&service, "LDKH66BJbLsHQPEAK@4_zak*"));
  // Must have at least one number
  EXPECT_FALSE(IsStrongPassword(&service, "LDKHBJbLsHQPEAK@_zak*"));
  // Number requirement is satisfied
  EXPECT_TRUE(IsStrongPassword(&service, "LDKHBJbLsH0QPEAK@_zak*"));
  // Must have at least one alpha character
  EXPECT_FALSE(IsStrongPassword(&service, "663@4_*"));
  // Character requirement can be lowercase
  EXPECT_TRUE(IsStrongPassword(&service, "663@4_*a"));
  // Character requirement can be uppercase
  EXPECT_TRUE(IsStrongPassword(&service, "663@4_*A"));
  // Must have at least one non-alphanumeric character
  EXPECT_FALSE(IsStrongPassword(&service, "LDKH66BJbLsHQPEAK4zak"));
  // space is ok for non alphanumeric requirement
  EXPECT_TRUE(IsStrongPassword(&service, "LDKH66BJbLsH QPEAK4zak"));
  // All requirements met except for length should still fail
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
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      brave_wallet::features::kBraveWalletFilecoinFeature);
  ASSERT_FALSE(service.SignTransactionByFilecoinKeyring(nullptr));
  auto transaction = FilTransaction::FromTxData(mojom::FilTxData::New(
      "1", "2", "3", "4", "5", "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
      "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6"));
  ASSERT_FALSE(service.SignTransactionByFilecoinKeyring(&transaction.value()));
  ASSERT_TRUE(CreateWallet(&service, "brave"));

  absl::optional<std::string> imported_account = ImportFilecoinAccount(
      &service, "Imported Filecoin account 1",
      // t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q
      "7b2254797065223a22736563703235366b31222c22507269766174654b6579223a2257"
      "6b4545645a45794235364b5168512b453338786a7663464c2b545a4842464e732b696a"
      "58533535794b383d227d",
      mojom::kFilecoinTestnet);
  ASSERT_TRUE(imported_account);
  EXPECT_EQ(*imported_account, "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q");

  auto result = service.SignTransactionByFilecoinKeyring(&transaction.value());
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
  EXPECT_EQ(base::JSONReader::Read(*result),
            base::JSONReader::Read(expected_result));
}

TEST_F(KeyringServiceUnitTest, AddFilecoinAccounts) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  {
    ASSERT_TRUE(CreateWallet(&service, "brave"));
#if BUILDFLAG(IS_ANDROID)
    ASSERT_FALSE(
        AddFilecoinAccount(&service, "FIL account1", mojom::kFilecoinTestnet));
#else
    ASSERT_TRUE(
        AddFilecoinAccount(&service, "FIL account1", mojom::kFilecoinTestnet));
#endif
    service.Reset();
  }

  base::test::ScopedFeatureList feature_list{
      features::kBraveWalletFilecoinFeature};

  ASSERT_TRUE(CreateWallet(&service, "brave"));

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  // import ETH account won't create other keyrings lazily
  ASSERT_TRUE(ImportAccount(
      &service, "Imported account1",
      "d118a12a1e3b595d7d9e5599370df4ddc58d246a3ae4a795597e50eb6a32afb5",
      mojom::CoinType::ETH));
  EXPECT_FALSE(service.IsKeyringCreated(mojom::kFilecoinKeyringId));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.IsKeyringCreated(mojom::kFilecoinKeyringId));

  // Add FIL account will lazily create corresponding keyring
  ASSERT_TRUE(
      AddFilecoinAccount(&service, "FIL account1", mojom::kFilecoinMainnet));
  EXPECT_TRUE(service.IsKeyringCreated(mojom::kFilecoinKeyringId));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.IsKeyringCreated(mojom::kFilecoinKeyringId));
  EXPECT_FALSE(observer.IsKeyringRestored(mojom::kFilecoinKeyringId));
  observer.Reset();

  // Add FIL with testnet network
  EXPECT_FALSE(service.IsKeyringCreated(mojom::kFilecoinTestnetKeyringId));
  ASSERT_TRUE(AddFilecoinAccount(&service, "FIL testnet account 1",
                                 mojom::kFilecoinTestnet));
  EXPECT_TRUE(service.IsKeyringCreated(mojom::kFilecoinTestnetKeyringId));
  ASSERT_TRUE(AddFilecoinAccount(&service, "FIL testnet account 2",
                                 mojom::kFilecoinTestnet));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.IsKeyringCreated(mojom::kFilecoinTestnetKeyringId));
  EXPECT_FALSE(observer.IsKeyringRestored(mojom::kFilecoinTestnetKeyringId));
  observer.Reset();

  // Lock and unlock won't fired created event again
  service.Lock();
  EXPECT_TRUE(Unlock(&service, "brave"));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(service.IsKeyringCreated(mojom::kFilecoinKeyringId));
  EXPECT_FALSE(observer.IsKeyringCreated(mojom::kFilecoinKeyringId));
  observer.Reset();

  // FIL keyring already exists
  ASSERT_TRUE(
      AddFilecoinAccount(&service, "FIL account2", mojom::kFilecoinMainnet));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.IsKeyringCreated(mojom::kFilecoinKeyringId));
  EXPECT_FALSE(observer.IsKeyringRestored(mojom::kFilecoinKeyringId));

  base::RunLoop run_loop;
  service.GetKeyringInfo(
      brave_wallet::mojom::kFilecoinKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_EQ(keyring_info->account_infos.size(), 2u);
        EXPECT_EQ(keyring_info->account_infos[0]->name, "FIL account1");
        EXPECT_FALSE(keyring_info->account_infos[0]->is_imported);
        EXPECT_EQ(keyring_info->account_infos[1]->name, "FIL account2");
        EXPECT_FALSE(keyring_info->account_infos[1]->is_imported);
        EXPECT_EQ(keyring_info->account_infos[1]->address,
                  GetFilecoinSelectedAccount(
                      &service, brave_wallet::mojom::kFilecoinMainnet));
        run_loop.Quit();
      }));

  service.GetKeyringInfo(
      brave_wallet::mojom::kFilecoinTestnetKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_EQ(keyring_info->account_infos.size(), 2u);
        EXPECT_EQ(keyring_info->account_infos[0]->name,
                  "FIL testnet account 1");
        EXPECT_FALSE(keyring_info->account_infos[0]->is_imported);
        EXPECT_EQ(keyring_info->account_infos[1]->name,
                  "FIL testnet account 2");
        EXPECT_FALSE(keyring_info->account_infos[1]->is_imported);
        EXPECT_EQ(keyring_info->account_infos[1]->address,
                  GetFilecoinSelectedAccount(
                      &service, brave_wallet::mojom::kFilecoinTestnet));
        run_loop.Quit();
      }));
  run_loop.Run();
}

TEST_F(KeyringServiceUnitTest, ImportFilecoinAccounts) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      brave_wallet::features::kBraveWalletFilecoinFeature);

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  ASSERT_TRUE(CreateWallet(&service, "brave"));

  ASSERT_FALSE(observer.IsKeyringCreated(mojom::kFilecoinKeyringId));
  EXPECT_FALSE(service.IsKeyringCreated(mojom::kFilecoinKeyringId));

  ASSERT_FALSE(observer.IsKeyringCreated(mojom::kFilecoinTestnetKeyringId));
  EXPECT_FALSE(service.IsKeyringCreated(mojom::kFilecoinTestnetKeyringId));

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
  ImportFilecoinAccounts(&service, &observer, imported_testnet_accounts,
                         mojom::kFilecoinTestnetKeyringId);
  ImportFilecoinAccounts(&service, &observer, imported_mainnet_accounts,
                         mojom::kFilecoinKeyringId);

  // filecoin keyring will be lazily created in first FIL import
  auto* filecoin_testnet_keyring =
      service.GetHDKeyringById(brave_wallet::mojom::kFilecoinTestnetKeyringId);
  EXPECT_EQ(filecoin_testnet_keyring->GetImportedAccountsNumber(),
            imported_testnet_accounts.size());

  // Remove testnet account
  EXPECT_TRUE(RemoveImportedAccount(&service,
                                    imported_testnet_accounts[1].address,
                                    kPasswordBrave, mojom::CoinType::FIL));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());

  // Remove mainnet account
  EXPECT_TRUE(RemoveImportedAccount(&service,
                                    imported_mainnet_accounts[1].address,
                                    kPasswordBrave, mojom::CoinType::FIL));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());

  observer.Reset();
  EXPECT_EQ(filecoin_testnet_keyring->GetImportedAccountsNumber(),
            imported_testnet_accounts.size() - 1);
  // remove invalid address
  EXPECT_FALSE(RemoveImportedAccount(&service, "0xxxxxxxxxx0", kPasswordBrave,
                                     mojom::CoinType::FIL));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.AccountsChangedFired());

  bool callback_called = false;
  service.GetKeyringInfo(
      brave_wallet::mojom::kFilecoinTestnetKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_EQ(keyring_info->id, mojom::kFilecoinTestnetKeyringId);
        EXPECT_TRUE(keyring_info->is_keyring_created);
        EXPECT_FALSE(keyring_info->is_locked);
        EXPECT_FALSE(keyring_info->is_backed_up);
        EXPECT_EQ(keyring_info->account_infos.size(),
                  imported_testnet_accounts.size() - 1);
        EXPECT_EQ(keyring_info->account_infos[0]->address,
                  imported_testnet_accounts[0].address);
        EXPECT_EQ(keyring_info->account_infos[0]->name,
                  imported_testnet_accounts[0].name);
        EXPECT_TRUE(keyring_info->account_infos[0]->is_imported);
        EXPECT_EQ(keyring_info->account_infos[1]->address,
                  imported_testnet_accounts[2].address);
        EXPECT_EQ(keyring_info->account_infos[1]->name,
                  imported_testnet_accounts[2].name);
        EXPECT_TRUE(keyring_info->account_infos[1]->is_imported);
        EXPECT_EQ(keyring_info->account_infos[2]->address,
                  imported_testnet_accounts[3].address);
        EXPECT_EQ(keyring_info->account_infos[2]->name,
                  imported_testnet_accounts[3].name);
        EXPECT_TRUE(keyring_info->account_infos[2]->is_imported);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(filecoin_testnet_keyring->GetImportedAccountsNumber(),
            imported_testnet_accounts.size() - 1);
  service.Lock();
  // cannot get private key when locked
  auto private_key = EncodePrivateKeyForExport(
      &service, imported_testnet_accounts[0].address, mojom::CoinType::FIL);
  EXPECT_FALSE(private_key);

  EXPECT_TRUE(Unlock(&service, "brave"));

  callback_called = false;
  // Imported accounts should be restored
  service.GetKeyringInfo(
      brave_wallet::mojom::kFilecoinTestnetKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_EQ(keyring_info->account_infos.size(),
                  imported_testnet_accounts.size() - 1);
        EXPECT_EQ(keyring_info->account_infos[0]->address,
                  imported_testnet_accounts[0].address);
        EXPECT_EQ(keyring_info->account_infos[0]->name,
                  imported_testnet_accounts[0].name);
        EXPECT_TRUE(keyring_info->account_infos[0]->is_imported);
        EXPECT_EQ(keyring_info->account_infos[1]->address,
                  imported_testnet_accounts[2].address);
        EXPECT_EQ(keyring_info->account_infos[1]->name,
                  imported_testnet_accounts[2].name);
        EXPECT_TRUE(keyring_info->account_infos[1]->is_imported);
        callback_called = true;
      }));

  EXPECT_TRUE(callback_called);
  callback_called = false;

  base::RunLoop().RunUntilIdle();

  service.GetKeyringInfo(
      brave_wallet::mojom::kFilecoinKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_EQ(keyring_info->account_infos.size(),
                  imported_mainnet_accounts.size() - 1);
        EXPECT_EQ(keyring_info->account_infos[0]->address,
                  imported_mainnet_accounts[0].address);
        EXPECT_EQ(keyring_info->account_infos[0]->name,
                  imported_mainnet_accounts[0].name);
        EXPECT_TRUE(keyring_info->account_infos[0]->is_imported);
        callback_called = true;
      }));

  EXPECT_TRUE(callback_called);

  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(
      service.GetHDKeyringById(brave_wallet::mojom::kFilecoinTestnetKeyringId)
          ->GetImportedAccountsNumber(),
      imported_testnet_accounts.size() - 1);
  auto payload = EncodePrivateKeyForExport(
      &service, imported_testnet_accounts[0].address, mojom::CoinType::FIL);
  EXPECT_TRUE(payload);
  EXPECT_EQ(imported_testnet_accounts[0].import_payload, *payload);

  auto* default_keyring =
      service.GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId);
  // Imported accounts should also be restored in filecoin keyring
  EXPECT_EQ(default_keyring->GetImportedAccountsNumber(), 0u);
  EXPECT_EQ(
      service.GetHDKeyringById(brave_wallet::mojom::kFilecoinTestnetKeyringId)
          ->GetImportedAccountsNumber(),
      imported_testnet_accounts.size() - 1);
}

TEST_F(KeyringServiceUnitTest, PreCreateEncryptors) {
  base::test::ScopedFeatureList feature_list;
  base::FieldTrialParams parameters;
  parameters[features::kCreateDefaultSolanaAccount.name] = "false";

  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(
      brave_wallet::features::kBraveWalletSolanaFeature, parameters);

  feature_list.InitWithFeaturesAndParameters(enabled_features, {});

  {
    // Create default wallet with disabled filecoin feature.
    // Solana feature is enabled on desktop and disabled on Android.
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    ASSERT_TRUE(CreateWallet(&service, "brave"));
    EXPECT_NE(service.encryptors_.at(mojom::kDefaultKeyringId), nullptr);
    EXPECT_NE(service.encryptors_.at(mojom::kSolanaKeyringId), nullptr);
#if BUILDFLAG(IS_ANDROID)
    EXPECT_FALSE(service.encryptors_.contains(mojom::kFilecoinKeyringId));
#else
    EXPECT_NE(service.encryptors_.at(mojom::kFilecoinKeyringId), nullptr);
#endif
  }
  {
    // Create wallet with enabled filecoin & solana
    base::test::ScopedFeatureList local_feature_list;
    local_feature_list.InitWithFeatures(
        {brave_wallet::features::kBraveWalletFilecoinFeature,
         brave_wallet::features::kBraveWalletSolanaFeature},
        {});

    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    ASSERT_TRUE(CreateWallet(&service, "brave"));
    EXPECT_NE(service.encryptors_.at(mojom::kDefaultKeyringId), nullptr);
    EXPECT_NE(service.encryptors_.at(mojom::kFilecoinKeyringId), nullptr);
    EXPECT_NE(service.encryptors_.at(mojom::kSolanaKeyringId), nullptr);
  }
  {
    // Create wallet and enable filecoin & solana before unlock
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    ASSERT_TRUE(CreateWallet(&service, "brave"));
    EXPECT_NE(service.encryptors_.at(mojom::kDefaultKeyringId), nullptr);
    EXPECT_NE(service.encryptors_.at(mojom::kSolanaKeyringId), nullptr);
#if BUILDFLAG(IS_ANDROID)
    EXPECT_FALSE(service.encryptors_.contains(mojom::kFilecoinKeyringId));
#else
    EXPECT_NE(service.encryptors_.at(mojom::kFilecoinKeyringId), nullptr);
#endif
    service.Lock();
    base::test::ScopedFeatureList local_feature_list;
    local_feature_list.InitWithFeatures(
        {brave_wallet::features::kBraveWalletFilecoinFeature,
         brave_wallet::features::kBraveWalletSolanaFeature},
        {});

    ASSERT_TRUE(Unlock(&service, "brave"));
    EXPECT_NE(service.encryptors_.at(mojom::kDefaultKeyringId), nullptr);
    EXPECT_NE(service.encryptors_.at(mojom::kFilecoinKeyringId), nullptr);
    EXPECT_NE(service.encryptors_.at(mojom::kSolanaKeyringId), nullptr);
  }
  {
    // Create default wallet and enable filecoin solana before restore
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    TestKeyringServiceObserver observer;
    service.AddObserver(observer.GetReceiver());
    absl::optional<std::string> mnemonic_to_be_restored =
        CreateWallet(&service, "brave");
    ASSERT_TRUE(mnemonic_to_be_restored.has_value());

    service.Reset();
    ASSERT_TRUE(
        RestoreWallet(&service, *mnemonic_to_be_restored, "brave", false));
    EXPECT_NE(service.encryptors_.at(mojom::kDefaultKeyringId), nullptr);
    EXPECT_NE(service.encryptors_.at(mojom::kSolanaKeyringId), nullptr);
#if BUILDFLAG(IS_ANDROID)
    EXPECT_FALSE(service.encryptors_.contains(mojom::kFilecoinKeyringId));
#else
    EXPECT_NE(service.encryptors_.at(mojom::kFilecoinKeyringId), nullptr);
#endif

    base::test::ScopedFeatureList local_feature_list;
    base::FieldTrialParams local_parameters;
    local_parameters[features::kCreateDefaultSolanaAccount.name] = "false";

    std::vector<base::test::FeatureRefAndParams> local_enabled_features;
    local_enabled_features.emplace_back(
        brave_wallet::features::kBraveWalletSolanaFeature, local_parameters);
    local_enabled_features.emplace_back(
        brave_wallet::features::kBraveWalletFilecoinFeature,
        base::FieldTrialParams());

    local_feature_list.InitWithFeaturesAndParameters(local_enabled_features,
                                                     {});

    service.Reset();
    ASSERT_TRUE(
        RestoreWallet(&service, *mnemonic_to_be_restored, "brave", false));
    EXPECT_NE(service.encryptors_.at(mojom::kDefaultKeyringId), nullptr);
    EXPECT_NE(service.encryptors_.at(mojom::kFilecoinKeyringId), nullptr);
    EXPECT_NE(service.encryptors_.at(mojom::kSolanaKeyringId), nullptr);
    // non default keyrings won't be created
    EXPECT_FALSE(
        service.IsKeyringCreated(brave_wallet::mojom::kFilecoinKeyringId));
    EXPECT_FALSE(
        service.IsKeyringCreated(brave_wallet::mojom::kSolanaKeyringId));
    EXPECT_FALSE(observer.IsKeyringCreated(mojom::kFilecoinKeyringId));
    EXPECT_FALSE(observer.IsKeyringCreated(mojom::kSolanaKeyringId));
    EXPECT_FALSE(observer.IsKeyringRestored(mojom::kFilecoinKeyringId));
    EXPECT_FALSE(observer.IsKeyringRestored(mojom::kSolanaKeyringId));
  }
}

TEST_F(KeyringServiceUnitTest, SolanaKeyring) {
  base::test::ScopedFeatureList feature_list;
  base::FieldTrialParams parameters;
  parameters[features::kCreateDefaultSolanaAccount.name] = "false";

  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(
      brave_wallet::features::kBraveWalletSolanaFeature, parameters);

  feature_list.InitWithFeaturesAndParameters(enabled_features, {});

  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    TestKeyringServiceObserver observer;
    service.AddObserver(observer.GetReceiver());
    ASSERT_TRUE(CreateWallet(&service, "brave"));
    EXPECT_TRUE(
        service.IsKeyringCreated(brave_wallet::mojom::kDefaultKeyringId));
    EXPECT_FALSE(
        service.IsKeyringCreated(brave_wallet::mojom::kSolanaKeyringId));
    EXPECT_FALSE(observer.IsKeyringCreated(mojom::kSolanaKeyringId));

    // lazily create solana keyring when adding SOL account
    ASSERT_TRUE(AddAccount(&service, "Account 1", mojom::CoinType::SOL));
    EXPECT_TRUE(
        service.IsKeyringCreated(brave_wallet::mojom::kDefaultKeyringId));
    EXPECT_TRUE(
        service.IsKeyringCreated(brave_wallet::mojom::kSolanaKeyringId));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(observer.IsKeyringCreated(mojom::kSolanaKeyringId));
    observer.Reset();

    // solana keyring already exists
    ASSERT_TRUE(AddAccount(&service, "Account 2", mojom::CoinType::SOL));
    base::RunLoop().RunUntilIdle();
    EXPECT_FALSE(observer.IsKeyringCreated(mojom::kSolanaKeyringId));

    service.Lock();
    ASSERT_TRUE(Unlock(&service, "brave"));

    base::RunLoop run_loop;
    service.GetKeyringInfo(
        brave_wallet::mojom::kSolanaKeyringId,
        base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
          EXPECT_EQ(keyring_info->id, mojom::kSolanaKeyringId);
          EXPECT_TRUE(keyring_info->is_keyring_created);
          EXPECT_EQ(keyring_info->account_infos.size(), 2u);
          EXPECT_EQ(keyring_info->account_infos[0]->name, "Account 1");
          EXPECT_FALSE(keyring_info->account_infos[0]->is_imported);
          EXPECT_EQ(keyring_info->account_infos[1]->name, "Account 2");
          EXPECT_FALSE(keyring_info->account_infos[1]->is_imported);
          run_loop.Quit();
        }));
    run_loop.Run();
    service.Reset();
  }
  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    TestKeyringServiceObserver observer;
    service.AddObserver(observer.GetReceiver());
    ASSERT_TRUE(RestoreWallet(&service, kMnemonic1, "brave", false));
    EXPECT_TRUE(
        service.IsKeyringCreated(brave_wallet::mojom::kDefaultKeyringId));
    EXPECT_FALSE(
        service.IsKeyringCreated(brave_wallet::mojom::kSolanaKeyringId));
    base::RunLoop().RunUntilIdle();
    EXPECT_FALSE(observer.IsKeyringRestored(mojom::kSolanaKeyringId));

    // lazily create solana keyring when adding SOL account
    ASSERT_TRUE(AddAccount(&service, "Account 1", mojom::CoinType::SOL));
    EXPECT_TRUE(
        service.IsKeyringCreated(brave_wallet::mojom::kDefaultKeyringId));
    EXPECT_TRUE(
        service.IsKeyringCreated(brave_wallet::mojom::kSolanaKeyringId));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(observer.IsKeyringCreated(mojom::kSolanaKeyringId));

    base::RunLoop run_loop;
    service.GetKeyringInfo(
        brave_wallet::mojom::kSolanaKeyringId,
        base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
          EXPECT_EQ(keyring_info->id, mojom::kSolanaKeyringId);
          EXPECT_TRUE(keyring_info->is_keyring_created);
          EXPECT_EQ(keyring_info->account_infos.size(), 1u);
          EXPECT_EQ(keyring_info->account_infos[0]->name, "Account 1");
          EXPECT_EQ(keyring_info->account_infos[0]->address,
                    "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8");
          EXPECT_FALSE(keyring_info->account_infos[0]->is_imported);
          run_loop.Quit();
        }));
    run_loop.Run();
    service.Reset();
  }

  {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    TestKeyringServiceObserver observer;
    service.AddObserver(observer.GetReceiver());
    ASSERT_TRUE(CreateWallet(&service, "brave"));

    // lazily create keyring when importing SOL account
    absl::optional<std::string> imported_account = ImportAccount(
        &service, "Imported Account 1",
        "sCzwsBKmKtk5Hgb4YUJAduQ5nmJq4GTyzCXhrKonAGaexa83MgSZuTSMS6TSZTndnC"
        "YbQtaJQKLXET9jVjepWXe",
        mojom::CoinType::SOL);
    ASSERT_TRUE(imported_account.has_value());
    EXPECT_EQ(*imported_account,
              "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ");
    EXPECT_TRUE(
        service.IsKeyringCreated(brave_wallet::mojom::kDefaultKeyringId));
    EXPECT_TRUE(
        service.IsKeyringCreated(brave_wallet::mojom::kSolanaKeyringId));
    // wait for observer
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(observer.IsKeyringCreated(mojom::kSolanaKeyringId));
    auto private_key = EncodePrivateKeyForExport(
        &service, "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ",
        mojom::CoinType::SOL);
    EXPECT_TRUE(private_key);
    EXPECT_EQ(*private_key,
              "sCzwsBKmKtk5Hgb4YUJAduQ5nmJq4GTyzCXhrKonAGaexa83MgSZuTSMS6TSZTnd"
              "nCYbQtaJQKLXET9jVjepWXe");

    // wrong encoded private key (same bytes but not encoded in keypair)
    EXPECT_EQ(ImportAccount(&service, "Imported Failed",
                            "3v1fSGD1JW5XnAd2FWrjV6HWJHM9DofVjuNt4T5b7CDL",
                            mojom::CoinType::SOL),
              absl::nullopt);
    imported_account = ImportAccount(
        &service, "Imported Account 2",
        "4pNHX6ATNXad3KZTb2PXTosW5ceaxqx45M9NH9pjcZCH9qoQKx6RMzUjuzm6J9Y2uwjCxJ"
        "c5JsjL1TrGr1X3nPFP",
        mojom::CoinType::SOL);
    ASSERT_TRUE(imported_account.has_value());
    ASSERT_TRUE(RemoveImportedAccount(&service, *imported_account,
                                      kPasswordBrave, mojom::CoinType::SOL));

    // import using uint8array
    imported_account = ImportAccount(
        &service, "Imported Account 3",
        " [4,109,17,28,245,96,126,232,185,242,61,170,96,51,225,202,152,85,104,"
        "63,4,171,245,175,118,67,238,247,208,163,247,211,201,215,12,121,255,"
        "182,188,11,4,82,78,239,173,146,246,74,66,126,34,173,46,211,145,49,211,"
        "176,28,89,250,190,34,254]\t\n",
        mojom::CoinType::SOL);
    ASSERT_TRUE(imported_account.has_value());
    ASSERT_TRUE(RemoveImportedAccount(&service, *imported_account,
                                      kPasswordBrave, mojom::CoinType::SOL));

    base::RunLoop run_loop;
    service.GetKeyringInfo(
        brave_wallet::mojom::kSolanaKeyringId,
        base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
          EXPECT_EQ(keyring_info->id, mojom::kSolanaKeyringId);
          EXPECT_TRUE(keyring_info->is_keyring_created);
          ASSERT_EQ(keyring_info->account_infos.size(), 1u);
          EXPECT_EQ(keyring_info->account_infos[0]->name, "Imported Account 1");
          EXPECT_EQ(keyring_info->account_infos[0]->address,
                    "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ");
          EXPECT_TRUE(keyring_info->account_infos[0]->is_imported);
          run_loop.Quit();
        }));
    run_loop.Run();

    service.Lock();
    EXPECT_TRUE(Unlock(&service, "brave"));

    base::RunLoop run_loop2;
    // imported accounts persist after lock & unlock
    service.GetKeyringInfo(
        brave_wallet::mojom::kSolanaKeyringId,
        base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
          EXPECT_EQ(keyring_info->id, mojom::kSolanaKeyringId);
          EXPECT_TRUE(keyring_info->is_keyring_created);
          ASSERT_EQ(keyring_info->account_infos.size(), 1u);
          EXPECT_EQ(keyring_info->account_infos[0]->name, "Imported Account 1");
          EXPECT_EQ(keyring_info->account_infos[0]->address,
                    "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ");
          EXPECT_TRUE(keyring_info->account_infos[0]->is_imported);
          run_loop2.Quit();
        }));
    run_loop2.Run();

    service.Reset();
  }
}

TEST_F(KeyringServiceUnitTest, SignMessage) {
  base::test::ScopedFeatureList feature_list;
  base::FieldTrialParams parameters;
  parameters[features::kCreateDefaultSolanaAccount.name] = "false";

  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(
      brave_wallet::features::kBraveWalletSolanaFeature, parameters);

  feature_list.InitWithFeaturesAndParameters(enabled_features, {});

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  ASSERT_TRUE(RestoreWallet(&service, kMnemonic1, "brave", false));
  base::RunLoop().RunUntilIdle();

  const std::vector<uint8_t> message = {0xde, 0xad, 0xbe, 0xef};

  // solana keyring doesn't exist yet
  EXPECT_TRUE(service
                  .SignMessage(mojom::kSolanaKeyringId,
                               "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
                               message)
                  .empty());

  // create solana keyring
  ASSERT_TRUE(AddAccount(&service, "Account 1", mojom::CoinType::SOL));
  ASSERT_TRUE(service.IsKeyringCreated(brave_wallet::mojom::kSolanaKeyringId));

  // not suppprt default keyring
  EXPECT_TRUE(service
                  .SignMessage(mojom::kDefaultKeyringId,
                               "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db",
                               message)
                  .empty());

  // invalid address for Solana keyring
  EXPECT_TRUE(service
                  .SignMessage(mojom::kSolanaKeyringId,
                               "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db",
                               message)
                  .empty());

  EXPECT_FALSE(service
                   .SignMessage(mojom::kSolanaKeyringId,
                                "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
                                message)
                   .empty());
}

class KeyringServiceAccountDiscoveryUnitTest : public KeyringServiceUnitTest {
 public:
  using TransactionCountCallback =
      base::RepeatingCallback<std::string(const std::string&)>;

  void SetUp() override {
    KeyringServiceUnitTest::SetUp();
    url_loader_factory().SetInterceptor(base::BindRepeating(
        &KeyringServiceAccountDiscoveryUnitTest::Interceptor,
        base::Unretained(this)));

    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    saved_mnemonic_ = CreateWallet(&service, "brave").value_or("");
    EXPECT_FALSE(saved_mnemonic_.empty());

    auto* default_keyring = service.GetHDKeyringById(mojom::kDefaultKeyringId);
    for (size_t i = 0; i < 100u; ++i) {
      EXPECT_TRUE(AddAccount(&service, "Acc" + std::to_string(i),
                             mojom::CoinType::ETH));
      saved_addresses_.push_back(default_keyring->GetAddress(i));
    }
    base::RunLoop().RunUntilIdle();
  }

  void set_transaction_count_callback(TransactionCountCallback cb) {
    transaction_count_callback_ = std::move(cb);
  }

  const std::string& saved_mnemonic() { return saved_mnemonic_; }
  const std::vector<std::string>& saved_addresses() { return saved_addresses_; }

  void Interceptor(const network::ResourceRequest& request) {
    url_loader_factory().ClearResponses();
    base::StringPiece request_string(request.request_body->elements()
                                         ->at(0)
                                         .As<network::DataElementBytes>()
                                         .AsStringPiece());
    absl::optional<base::Value> request_value =
        base::JSONReader::Read(request_string);
    if (*request_value->GetDict().FindString("method") ==
        "eth_getTransactionCount") {
      base::Value* params = request_value->FindListKey("params");
      EXPECT_TRUE(params);
      std::string* address = params->GetList()[0].GetIfString();
      EXPECT_TRUE(address);

      if (transaction_count_callback_) {
        url_loader_factory().AddResponse(
            request.url.spec(), transaction_count_callback_.Run(*address));
      }
    }
  }

 protected:
  TransactionCountCallback transaction_count_callback_;
  std::string saved_mnemonic_;
  std::vector<std::string> saved_addresses_;
};

TEST_F(KeyringServiceAccountDiscoveryUnitTest, AccountDiscovery) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  std::vector<std::string> requested_addresses;
  set_transaction_count_callback(base::BindLambdaForTesting(
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

  EXPECT_TRUE(RestoreWallet(&service, saved_mnemonic(), "brave1", false));
  base::RunLoop().RunUntilIdle();
  std::vector<mojom::AccountInfoPtr> account_infos =
      service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
  EXPECT_EQ(account_infos.size(), 11u);
  for (size_t i = 0; i < account_infos.size(); ++i) {
    EXPECT_EQ(account_infos[i]->address, saved_addresses()[i]);
    EXPECT_EQ(account_infos[i]->name, "Account " + std::to_string(i + 1));
  }
  // Accounts 3 and 10.
  EXPECT_EQ(2, observer.AccountsChangedFiredCount());
  // 20 attempts more after Account 10 is added.
  EXPECT_THAT(requested_addresses, ElementsAreArray(&saved_addresses()[1], 30));
}

TEST_F(KeyringServiceAccountDiscoveryUnitTest, StopsOnError) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  std::vector<std::string> requested_addresses;
  set_transaction_count_callback(base::BindLambdaForTesting(
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

  EXPECT_TRUE(RestoreWallet(&service, saved_mnemonic(), "brave1", false));
  base::RunLoop().RunUntilIdle();
  std::vector<mojom::AccountInfoPtr> account_infos =
      service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
  EXPECT_EQ(account_infos.size(), 4u);
  for (size_t i = 0; i < account_infos.size(); ++i) {
    EXPECT_EQ(account_infos[i]->address, saved_addresses()[i]);
    EXPECT_EQ(account_infos[i]->name, "Account " + std::to_string(i + 1));
  }
  // Account 3.
  EXPECT_EQ(1, observer.AccountsChangedFiredCount());
  // Stopped after 8th attempt.
  EXPECT_THAT(requested_addresses, ElementsAreArray(&saved_addresses()[1], 8));
}

TEST_F(KeyringServiceAccountDiscoveryUnitTest, ManuallyAddAccount) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  std::vector<std::string> requested_addresses;
  set_transaction_count_callback(base::BindLambdaForTesting(
      [this, &service,
       &requested_addresses](const std::string& address) -> std::string {
        requested_addresses.push_back(address);

        // Manually add account while checking 4th account. Will be added
        // instead of Account 2.
        if (address == saved_addresses()[4]) {
          EXPECT_TRUE(
              AddAccount(&service, "Added Account 2", mojom::CoinType::ETH));
        }

        // Manually add account while checking 6th account. Will be added
        // instead of Account 6.
        if (address == saved_addresses()[6]) {
          EXPECT_TRUE(
              AddAccount(&service, "Added Account 7", mojom::CoinType::ETH));
        }

        // 5th and 6th accounts have transactions.
        if (address == saved_addresses()[5] ||
            address == saved_addresses()[6]) {
          return R"({"jsonrpc":"2.0","id":"1","result":"0x1"})";
        } else {
          return R"({"jsonrpc":"2.0","id":"1","result":"0x0"})";
        }
      }));

  EXPECT_TRUE(RestoreWallet(&service, saved_mnemonic(), "brave1", false));
  base::RunLoop().RunUntilIdle();
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
      EXPECT_EQ(account_infos[i]->name, "Account " + std::to_string(i + 1));
    }
  }
  // Two accounts added manually, one by discovery.
  EXPECT_EQ(3, observer.AccountsChangedFiredCount());
  // 20 attempts more after Account 6 is added.
  EXPECT_THAT(requested_addresses, ElementsAreArray(&saved_addresses()[1], 26));
}

TEST_F(KeyringServiceAccountDiscoveryUnitTest, RestoreWalletTwice) {
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  std::vector<std::string> requested_addresses;
  bool first_restore = true;
  base::RunLoop run_loop;
  set_transaction_count_callback(base::BindLambdaForTesting(
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
  observer.Reset();
  EXPECT_TRUE(RestoreWallet(&service, saved_mnemonic(), "brave1", false));
  base::RunLoop().RunUntilIdle();

  std::vector<mojom::AccountInfoPtr> account_infos =
      service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
  EXPECT_EQ(account_infos.size(), 11u);
  for (size_t i = 0; i < account_infos.size(); ++i) {
    EXPECT_EQ(account_infos[i]->address, saved_addresses()[i]);
    EXPECT_EQ(account_infos[i]->name, "Account " + std::to_string(i + 1));
  }
  // Accounts 3 and 10.
  EXPECT_EQ(2, observer.AccountsChangedFiredCount());
  // Second restore: 20 attempts more after Account 10 is added.
  EXPECT_THAT(requested_addresses, ElementsAreArray(&saved_addresses()[1], 30));
}

class KeyringServiceEncryptionKeysMigrationUnitTest
    : public KeyringServiceUnitTest {
 public:
  KeyringServiceEncryptionKeysMigrationUnitTest() {
    feature_list_.InitWithFeatures(
        {brave_wallet::features::kBraveWalletFilecoinFeature,
         brave_wallet::features::kBraveWalletSolanaFeature},
        {});
  }

  void SetupKeyring() {
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    saved_mnemonic_ = *CreateWallet(&service, "brave");

    absl::optional<std::string> imported_eth_account = ImportAccount(
        &service, "Imported account1",
        "d118a12a1e3b595d7d9e5599370df4ddc58d246a3ae4a795597e50eb6a32afb5",
        mojom::CoinType::ETH);
    ASSERT_TRUE(imported_eth_account.has_value());
    EXPECT_EQ("0xDc06aE500aD5ebc5972A0D8Ada4733006E905976",
              *imported_eth_account);

    absl::optional<std::string> imported_sol_account = ImportAccount(
        &service, "Imported Account 1",
        "sCzwsBKmKtk5Hgb4YUJAduQ5nmJq4GTyzCXhrKonAGaexa83MgSZuTSMS6TSZTndnC"
        "YbQtaJQKLXET9jVjepWXe",
        mojom::CoinType::SOL);
    ASSERT_TRUE(imported_sol_account.has_value());
    EXPECT_EQ(*imported_sol_account,
              "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ");

    ASSERT_TRUE(
        AddFilecoinAccount(&service, "FIL Account 1", mojom::kFilecoinMainnet));
    absl::optional<std::string> imported_fil_account = ImportFilecoinAccount(
        &service, "fil m acc 1",
        "7b2254797065223a22736563703235366b31222c22507269766174"
        "654b6579223a224169776f6a344469323155316844776835735348"
        "434d7a37342b346c45303472376e5349454d706d6258493d227d",
        mojom::kFilecoinMainnet);
    ASSERT_TRUE(imported_fil_account.has_value());
    EXPECT_EQ(*imported_fil_account,
              "f1syhomjrwhjmavadwmrofjpiocb6r72h4qoy7ucq");

    absl::optional<std::string> imported_fil_test_account =
        ImportFilecoinAccount(
            &service, "fil t acc 2",
            "7b2254797065223a22736563703235366b31222c22507269766174"
            "654b6579223a226376414367502f53344f3274796c4f42466a6348"
            "33583154373677696661456c6646435057612b6a474a453d227d",
            mojom::kFilecoinTestnet);
    ASSERT_TRUE(imported_fil_test_account.has_value());
    EXPECT_EQ(*imported_fil_test_account,
              "t17puhwpgtnjr54kw7dwnjiphgn6kxlsyzbizwdhy");
  }

  void ValidateImportedAccountsForUnlockedKeyring(KeyringService* service) {
    EXPECT_FALSE(service->IsLocked(mojom::kDefaultKeyringId));
    EXPECT_FALSE(service->IsLocked(mojom::kSolanaKeyringId));
    EXPECT_FALSE(service->IsLocked(mojom::kFilecoinKeyringId));
    EXPECT_FALSE(service->IsLocked(mojom::kFilecoinTestnetKeyringId));

    // Imported accounts still work.
    EXPECT_EQ(
        "d118a12a1e3b595d7d9e5599370df4ddc58d246a3ae4a795597e50eb6a32afb5",
        *EncodePrivateKeyForExport(service,
                                   "0xDc06aE500aD5ebc5972A0D8Ada4733006E905976",
                                   mojom::CoinType::ETH));

    EXPECT_EQ(
        "sCzwsBKmKtk5Hgb4YUJAduQ5nmJq4GTyzCXhrKonAGaexa83MgSZuTSMS6TSZTndnC"
        "YbQtaJQKLXET9jVjepWXe",
        *EncodePrivateKeyForExport(
            service, "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ",
            mojom::CoinType::SOL));

    EXPECT_EQ(
        "7b2254797065223a22736563703235366b31222c22507269766174"
        "654b6579223a224169776f6a344469323155316844776835735348"
        "434d7a37342b346c45303472376e5349454d706d6258493d227d",
        *EncodePrivateKeyForExport(service,
                                   "f1syhomjrwhjmavadwmrofjpiocb6r72h4qoy7ucq",
                                   mojom::CoinType::FIL));

    EXPECT_EQ(
        "7b2254797065223a22736563703235366b31222c22507269766174"
        "654b6579223a226376414367502f53344f3274796c4f42466a6348"
        "33583154373677696661456c6646435057612b6a474a453d227d",
        *EncodePrivateKeyForExport(service,
                                   "t17puhwpgtnjr54kw7dwnjiphgn6kxlsyzbizwdhy",
                                   mojom::CoinType::FIL));
  }

  void ValidateNoImportedAccountsForUnlockedKeyring(KeyringService* service) {
    EXPECT_FALSE(service->IsLocked(mojom::kDefaultKeyringId));
    EXPECT_FALSE(service->IsLocked(mojom::kSolanaKeyringId));
    EXPECT_FALSE(service->IsLocked(mojom::kFilecoinKeyringId));
    EXPECT_FALSE(service->IsLocked(mojom::kFilecoinTestnetKeyringId));

    // Imported accounts are missing.
    EXPECT_EQ(absl::nullopt,
              EncodePrivateKeyForExport(
                  service, "0xDc06aE500aD5ebc5972A0D8Ada4733006E905976",
                  mojom::CoinType::ETH));

    EXPECT_EQ(absl::nullopt,
              EncodePrivateKeyForExport(
                  service, "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ",
                  mojom::CoinType::SOL));

    EXPECT_EQ(absl::nullopt,
              EncodePrivateKeyForExport(
                  service, "f1syhomjrwhjmavadwmrofjpiocb6r72h4qoy7ucq",
                  mojom::CoinType::FIL));

    EXPECT_EQ(absl::nullopt,
              EncodePrivateKeyForExport(
                  service, "t17puhwpgtnjr54kw7dwnjiphgn6kxlsyzbizwdhy",
                  mojom::CoinType::FIL));
  }

  const std::string& saved_mnemonic() const { return saved_mnemonic_; }

 private:
  std::string saved_mnemonic_;
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(KeyringServiceEncryptionKeysMigrationUnitTest, NoMigration) {
  EXPECT_FALSE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));
  SetupKeyring();
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  EXPECT_TRUE(Unlock(&service, "brave"));
  ValidateImportedAccountsForUnlockedKeyring(&service);
}

TEST_F(KeyringServiceEncryptionKeysMigrationUnitTest, MigrateWithUnlock) {
  // Setup prefs with legacy iterations count value.
  KeyringService::GetPbkdf2IterationsForTesting() = 100000;
  SetupKeyring();
  KeyringService::GetPbkdf2IterationsForTesting() = absl::nullopt;

  // Reset migration pref so migration runs on next unlock.
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));
  GetPrefs()->ClearPref(kBraveWalletKeyringEncryptionKeysMigrated);
  EXPECT_FALSE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  EXPECT_TRUE(Unlock(&service, "brave"));
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));

  service.Lock();
  // Unlock with wrong password fails.
  EXPECT_FALSE(Unlock(&service, "another password"));
  KeyringService::GetPbkdf2IterationsForTesting() = 100000;
  // Unlock with legacy iterations fails.
  EXPECT_FALSE(Unlock(&service, "brave"));
  KeyringService::GetPbkdf2IterationsForTesting() = absl::nullopt;

  // Unlocking again works.
  EXPECT_TRUE(Unlock(&service, "brave"));
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));

  ValidateImportedAccountsForUnlockedKeyring(&service);
}

TEST_F(KeyringServiceEncryptionKeysMigrationUnitTest,
       MigrateWithRestoreDoingResume) {
  // Setup prefs with legacy iterations count value.
  KeyringService::GetPbkdf2IterationsForTesting() = 100000;
  SetupKeyring();
  KeyringService::GetPbkdf2IterationsForTesting() = absl::nullopt;

  // Reset migration pref so migration runs on next restore.
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));
  GetPrefs()->ClearPref(kBraveWalletKeyringEncryptionKeysMigrated);
  EXPECT_FALSE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));

  // Restore wallet.
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  EXPECT_TRUE(RestoreWallet(&service, saved_mnemonic(), "brave", false));
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));

  // Check wallet state after restore.
  ValidateImportedAccountsForUnlockedKeyring(&service);
}

TEST_F(KeyringServiceEncryptionKeysMigrationUnitTest,
       MigrateWithRestoreAndNewPassword) {
  // Setup prefs with legacy iterations count value.
  KeyringService::GetPbkdf2IterationsForTesting() = 100000;
  SetupKeyring();
  KeyringService::GetPbkdf2IterationsForTesting() = absl::nullopt;

  // Reset migration pref so migration runs on next restore.
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));
  GetPrefs()->ClearPref(kBraveWalletKeyringEncryptionKeysMigrated);
  EXPECT_FALSE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));

  // Restore wallet.
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  EXPECT_TRUE(RestoreWallet(&service, saved_mnemonic(), "brave123", false));

  // No migration needed after wallet is reset with a new password.
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));

  // Check wallet state after restore.
  ValidateNoImportedAccountsForUnlockedKeyring(&service);
}

TEST_F(KeyringServiceEncryptionKeysMigrationUnitTest, NoMigrationAfterResote) {
  EXPECT_FALSE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));

  // Restore wallet.
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  EXPECT_TRUE(RestoreWallet(&service, kMnemonic1, "brave", false));

  // No migration needed after wallet is created with reset.
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));
}

TEST_F(KeyringServiceEncryptionKeysMigrationUnitTest,
       ValidatePasswordWorksAfterMigrate) {
  // Setup prefs with legacy iterations count value.
  KeyringService::GetPbkdf2IterationsForTesting() = 100000;
  SetupKeyring();
  KeyringService::GetPbkdf2IterationsForTesting() = absl::nullopt;

  // Reset migration pref so migration runs on next unlock.
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));
  GetPrefs()->ClearPref(kBraveWalletKeyringEncryptionKeysMigrated);
  EXPECT_FALSE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  // Password is good, but no migration happens.
  EXPECT_TRUE(ValidatePassword(&service, "brave"));
  EXPECT_FALSE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));

  // Unlock triggers migration, password is still good.
  EXPECT_TRUE(Unlock(&service, "brave"));
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));
  EXPECT_TRUE(ValidatePassword(&service, "brave"));
}

TEST_F(KeyringServiceUnitTest, AccountsAdded) {
  // Verifies AccountsAdded event is emitted as expected in AddAccount
  // CreateWallet, RestoreWallet, AddHardwareAccounts, and
  // ImportAccountForKeyring
  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  // RestoreWallet
  RestoreWallet(&service, kMnemonic1, kPasswordBrave, false);
  base::RunLoop().RunUntilIdle();
  observer.ExpectAddressesAddedEq(
      {{"0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db"},
       {"BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8"}});
  task_environment_.FastForwardBy(
      base::Minutes(kAssetDiscoveryMinutesPerRequest));

  // AddAccount ETH
  ASSERT_TRUE(AddAccount(&service, "Account", mojom::CoinType::ETH));
  base::RunLoop().RunUntilIdle();
  observer.ExpectAddressesAddedEq(
      {{"0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0"}});
  task_environment_.FastForwardBy(
      base::Minutes(kAssetDiscoveryMinutesPerRequest));

  // AddAccount SOL
  ASSERT_TRUE(AddAccount(&service, "Account", mojom::CoinType::SOL));
  base::RunLoop().RunUntilIdle();
  observer.ExpectAddressesAddedEq(
      {{"JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV"}});
  task_environment_.FastForwardBy(
      base::Minutes(kAssetDiscoveryMinutesPerRequest));

  // AddHardwareAccounts
  std::vector<mojom::HardwareWalletAccountPtr> hardware_accounts;
  hardware_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0x595a0583621FDe81A935021707e81343f75F9324", "m/44'/60'/1'/0/0",
      "name 1", "Ledger", "device1", mojom::CoinType::ETH, absl::nullopt));
  service.AddHardwareAccounts(std::move(hardware_accounts));
  base::RunLoop().RunUntilIdle();
  observer.ExpectAddressesAddedEq(
      {{"0x595a0583621FDe81A935021707e81343f75F9324"}});
  task_environment_.FastForwardBy(
      base::Minutes(kAssetDiscoveryMinutesPerRequest));

  // ImportAccountForKeyring
  const std::string private_key_str =
      "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
  std::vector<uint8_t> private_key_bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(private_key_str, &private_key_bytes));
  ASSERT_TRUE(service.ImportAccountForKeyring(
      mojom::kDefaultKeyringId, "Imported Account", private_key_bytes));
  base::RunLoop().RunUntilIdle();
  observer.ExpectAddressesAddedEq(
      {{"0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266"}});
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
    EXPECT_TRUE(service.IsLocked(mojom::kDefaultKeyringId));
  }

  // Unlocked on start with right password.
  {
    cmdline->AppendSwitchASCII(switches::kDevWalletPassword, "some_password");
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    EXPECT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
    cmdline->RemoveSwitch(switches::kDevWalletPassword);
  }

  // Locked on start with wrong password.
  {
    cmdline->AppendSwitchASCII(switches::kDevWalletPassword, "wrong_password");
    KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());
    EXPECT_TRUE(service.IsLocked(mojom::kDefaultKeyringId));
    cmdline->RemoveSwitch(switches::kDevWalletPassword);
  }
}
#endif  // !defined(OFFICIAL_BUILD)

TEST_F(KeyringServiceUnitTest, BitcoinReceiveChangeAddresses) {
  // TODO(apaymyshev): update existing tests above to also cover Bitcoin
  // keyring.

  base::test::ScopedFeatureList feature_list{
      features::kBraveWalletBitcoinFeature};

  KeyringService service(json_rpc_service(), GetPrefs(), GetLocalState());

  // https://github.com/bitcoin/bips/blob/master/bip-0084.mediawiki#test-vectors
  ASSERT_TRUE(RestoreWallet(
      &service,
      "abandon abandon abandon abandon abandon abandon abandon abandon abandon "
      "abandon abandon about",
      "brave", false));

  EXPECT_THAT(
      service.GetKeyringInfoSync(mojom::kBitcoinKeyringId)->account_infos,
      testing::IsEmpty());

  EXPECT_EQ(service.GetBitcoinReceivingAddress(mojom::kBitcoinKeyringId, 0, 0),
            absl::nullopt);
  EXPECT_EQ(service.GetBitcoinChangeAddress(mojom::kBitcoinKeyringId, 0, 0),
            absl::nullopt);
  EXPECT_EQ(service.GetBitcoinReceivingAddress(mojom::kBitcoinKeyringId, 1, 0),
            absl::nullopt);
  EXPECT_EQ(service.GetBitcoinChangeAddress(mojom::kBitcoinKeyringId, 1, 0),
            absl::nullopt);

  AddAccount(&service, "Btc Acc", mojom::CoinType::BTC);

  EXPECT_THAT(
      service.GetKeyringInfoSync(mojom::kBitcoinKeyringId)->account_infos,
      testing::SizeIs(1u));
  auto btc_acc = service.GetKeyringInfoSync(mojom::kBitcoinKeyringId)
                     ->account_infos[0]
                     ->Clone();
  EXPECT_EQ(btc_acc->address, "bc1ql5f64jdzjsvgehlpxvdgm9ygp0xta7xpnueh03");
  EXPECT_EQ(btc_acc->name, "Btc Acc");
  EXPECT_FALSE(btc_acc->is_imported);
  EXPECT_FALSE(btc_acc->hardware);
  EXPECT_EQ(btc_acc->coin, mojom::CoinType::BTC);
  EXPECT_EQ(btc_acc->keyring_id, mojom::kBitcoinKeyringId);

  EXPECT_EQ(service.GetBitcoinReceivingAddress(mojom::kBitcoinKeyringId, 0, 0),
            "bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu");
  EXPECT_EQ(service.GetBitcoinChangeAddress(mojom::kBitcoinKeyringId, 0, 0),
            "bc1q8c6fshw2dlwun7ekn9qwf37cu2rn755upcp6el");
  EXPECT_EQ(service.GetBitcoinReceivingAddress(mojom::kBitcoinKeyringId, 1, 0),
            absl::nullopt);
  EXPECT_EQ(service.GetBitcoinChangeAddress(mojom::kBitcoinKeyringId, 1, 0),
            absl::nullopt);
}

}  // namespace brave_wallet
