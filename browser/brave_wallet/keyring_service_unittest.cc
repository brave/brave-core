/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/keyring_service.h"

#include <utility>

#include "base/base64.h"
#include "base/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/fil_transaction.h"
#include "brave/components/brave_wallet/browser/filecoin_keyring.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-shared.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "build/build_config.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
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
const char kPasswordEncryptorSalt[] = "password_encryptor_salt";
const char kPasswordEncryptorNonce[] = "password_encryptor_nonce";
const char kEncryptedMnemonic[] = "encrypted_mnemonic";
const char kBackupComplete[] = "backup_complete";
const char kAccountMetas[] = "account_metas";
const char kHardwareAccounts[] = "hardware";
const char kImportedAccounts[] = "imported_accounts";
const char kAccountAddress[] = "account_address";
const char kEncryptedPrivateKey[] = "encrypted_private_key";
const char kSelectedAccount[] = "selected_account";

const char kMnemonic1[] =
    "divide cruise upon flag harsh carbon filter merit once advice bright "
    "drive";
const char kMnemonic2[] =
    "misery jeans response tiny nominee civil zoo strong correct taxi chimney "
    "goat";

base::Value GetHardwareKeyringValueForTesting() {
  base::DictionaryValue dict;
  dict.SetPath("hardware.A1.account_metas.0x111.account_name",
               base::Value("test1"));
  dict.SetPath("hardware.A1.account_metas.0x111.derivation_path",
               base::Value("path1"));
  dict.SetPath("hardware.A1.account_metas.0x111.hardware_vendor",
               base::Value("vendor1"));
  dict.SetPath("hardware.B2.account_metas.0x222.account_name",
               base::Value("test2"));
  dict.SetPath("hardware.B2.account_metas.0x222.derivation_path",
               base::Value("path2"));
  dict.SetPath("hardware.B2.account_metas.0x222.hardware_vendor",
               base::Value("vendor2"));
  return dict;
}

}  // namespace

class TestKeyringServiceObserver
    : public brave_wallet::mojom::KeyringServiceObserver {
 public:
  TestKeyringServiceObserver() {}

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
  void Locked() override {}
  void Unlocked() override {}
  void BackedUp() override {}

  void SelectedAccountChanged(mojom::CoinType coin) override {
    selected_account_change_fired_.insert(coin);
  }

  void AccountsChanged() override { accounts_changed_fired_count_++; }

  bool AutoLockMinutesChangedFired() {
    return auto_lock_minutes_changed_fired_;
  }
  bool SelectedAccountChangedFired(mojom::CoinType coin) {
    return selected_account_change_fired_.contains(coin);
  }
  bool AccountsChangedFired() { return accounts_changed_fired_count_ > 0; }
  int AccountsChangedFiredCount() { return accounts_changed_fired_count_; }
  bool KeyringResetFired() { return keyring_reset_fired_; }
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

  void Reset() {
    auto_lock_minutes_changed_fired_ = false;
    accounts_changed_fired_count_ = 0;
    keyring_reset_fired_ = false;
    selected_account_change_fired_.clear();
    keyring_created_.clear();
    keyring_restored_.clear();
  }

 private:
  bool auto_lock_minutes_changed_fired_ = false;
  int accounts_changed_fired_count_ = 0;
  bool keyring_reset_fired_ = false;
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

  PrefService* GetPrefs() { return profile_->GetPrefs(); }

  content::BrowserContext* browser_context() { return profile_.get(); }

  JsonRpcService* json_rpc_service() { return json_rpc_service_; }

  network::TestURLLoaderFactory& url_loader_factory() {
    return url_loader_factory_;
  }

  bool HasPrefForKeyring(const std::string& key, const std::string& id) {
    return KeyringService::HasPrefForKeyring(GetPrefs(), key, id);
  }

  std::string GetStringPrefForKeyring(const std::string& key,
                                      const std::string& id) {
    const base::Value* value =
        KeyringService::GetPrefForKeyring(GetPrefs(), key, id);
    if (!value)
      return std::string();

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
              keyring_info->account_infos.empty())
            result = true;
          run_loop.Quit();
        }));
    run_loop.Run();
    return result;
  }

  static std::string GetMnemonicForDefaultKeyring(KeyringService* service) {
    base::RunLoop run_loop;
    std::string mnemonic;
    service->GetMnemonicForDefaultKeyring(
        base::BindLambdaForTesting([&](const std::string& v) {
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
                                    mojom::CoinType coin) {
    bool success;
    base::RunLoop run_loop;
    service->RemoveImportedAccount(address, coin,
                                   base::BindLambdaForTesting([&](bool v) {
                                     success = v;
                                     run_loop.Quit();
                                   }));
    run_loop.Run();
    return success;
  }

  static bool GetPrivateKeyForImportedAccount(KeyringService* service,
                                              const std::string& address,
                                              mojom::CoinType coin,
                                              std::string* private_key) {
    bool success;
    base::RunLoop run_loop;
    service->GetPrivateKeyForImportedAccount(
        address, coin,
        base::BindLambdaForTesting([&](bool v, const std::string& key) {
          success = v;
          *private_key = key;
          run_loop.Quit();
        }));
    run_loop.Run();
    return success;
  }

  static absl::optional<std::string> GetPrivateKeyForKeyringAccount(
      KeyringService* service,
      const std::string& address,
      mojom::CoinType coin) {
    absl::optional<std::string> private_key;
    base::RunLoop run_loop;
    service->GetPrivateKeyForKeyringAccount(
        address, coin,
        base::BindLambdaForTesting([&](bool success, const std::string& key) {
          if (success)
            private_key = key;
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
              if (success)
                account = address;
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
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};  // namespace brave_wallet

TEST_F(KeyringServiceUnitTest, HasAndGetPrefForKeyring) {
  base::DictionaryValue dict;
  dict.SetPath("default.pref1", base::Value("123"));
  GetPrefs()->Set(kBraveWalletKeyrings, dict);
  EXPECT_TRUE(KeyringService::HasPrefForKeyring(GetPrefs(), "pref1",
                                                mojom::kDefaultKeyringId));
  const base::Value* value = KeyringService::GetPrefForKeyring(
      GetPrefs(), "pref1", mojom::kDefaultKeyringId);
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->GetString(), "123");

  EXPECT_FALSE(
      KeyringService::HasPrefForKeyring(GetPrefs(), "pref1", "keyring2"));
  EXPECT_EQ(KeyringService::GetPrefForKeyring(GetPrefs(), "pref1", "keyring2"),
            nullptr);

  EXPECT_FALSE(KeyringService::HasPrefForKeyring(GetPrefs(), "pref2",
                                                 mojom::kDefaultKeyringId));
  EXPECT_EQ(KeyringService::GetPrefForKeyring(GetPrefs(), "pref2",
                                              mojom::kDefaultKeyringId),
            nullptr);
}

TEST_F(KeyringServiceUnitTest, SetPrefForKeyring) {
  KeyringService::SetPrefForKeyring(GetPrefs(), "pref1", base::Value("123"),
                                    mojom::kDefaultKeyringId);
  const base::Value* keyrings_pref =
      GetPrefs()->GetDictionary(kBraveWalletKeyrings);
  ASSERT_NE(keyrings_pref, nullptr);
  const base::Value* value = keyrings_pref->FindPath("default.pref1");
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->GetString(), "123");

  EXPECT_EQ(keyrings_pref->FindPath("default.pref2"), nullptr);
  EXPECT_EQ(keyrings_pref->FindPath("keyring2.pref1"), nullptr);
}

TEST_F(KeyringServiceUnitTest, GetAvailableKeyringsFromPrefs) {
  ASSERT_TRUE(
      KeyringService::GetAvailableKeyringsFromPrefs(GetPrefs()).empty());
  KeyringService::SetPrefForKeyring(GetPrefs(), "pref1", base::Value("123"),
                                    mojom::kDefaultKeyringId);
  KeyringService::SetPrefForKeyring(GetPrefs(), "pref1", base::Value("123"),
                                    mojom::kFilecoinKeyringId);
  EXPECT_EQ(KeyringService::GetAvailableKeyringsFromPrefs(GetPrefs()).front(),
            mojom::kDefaultKeyringId);
  EXPECT_EQ(KeyringService::GetAvailableKeyringsFromPrefs(GetPrefs()).back(),
            mojom::kFilecoinKeyringId);
  KeyringService::SetPrefForKeyring(GetPrefs(), "pref1", base::Value("123"),
                                    mojom::kSolanaKeyringId);
  auto keyrings = KeyringService::GetAvailableKeyringsFromPrefs(GetPrefs());
  EXPECT_EQ(keyrings.size(), 3u);
  EXPECT_EQ(keyrings[0], mojom::kDefaultKeyringId);
  EXPECT_EQ(keyrings[1], mojom::kFilecoinKeyringId);
  EXPECT_EQ(keyrings[2], mojom::kSolanaKeyringId);
}

TEST_F(KeyringServiceUnitTest, GetPrefInBytesForKeyring) {
  KeyringService service(json_rpc_service(), GetPrefs());
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

  std::vector<uint8_t> mnemonic;
  ASSERT_TRUE(service.GetPrefInBytesForKeyring(kEncryptedMnemonic, &mnemonic,
                                               mojom::kDefaultKeyringId));
  verify_bytes(mnemonic);

  std::vector<uint8_t> mnemonic_fixed(4);
  ASSERT_TRUE(service.GetPrefInBytesForKeyring(
      kEncryptedMnemonic, &mnemonic_fixed, mojom::kDefaultKeyringId));
  verify_bytes(mnemonic_fixed);

  std::vector<uint8_t> mnemonic_smaller(2);
  ASSERT_TRUE(service.GetPrefInBytesForKeyring(
      kEncryptedMnemonic, &mnemonic_smaller, mojom::kDefaultKeyringId));
  verify_bytes(mnemonic_smaller);

  std::vector<uint8_t> mnemonic_bigger(8);
  ASSERT_TRUE(service.GetPrefInBytesForKeyring(
      kEncryptedMnemonic, &mnemonic_bigger, mojom::kDefaultKeyringId));
  verify_bytes(mnemonic_bigger);

  // invalid base64 encoded
  mnemonic.clear();
  KeyringService::SetPrefForKeyring(GetPrefs(), kEncryptedMnemonic,
                                    base::Value("3q2+7w^^"),
                                    mojom::kDefaultKeyringId);
  EXPECT_FALSE(service.GetPrefInBytesForKeyring(kEncryptedMnemonic, &mnemonic,
                                                mojom::kDefaultKeyringId));

  // default pref value (empty)
  mnemonic.clear();
  GetPrefs()->ClearPref(kBraveWalletKeyrings);
  EXPECT_FALSE(service.GetPrefInBytesForKeyring(kEncryptedMnemonic, &mnemonic,
                                                mojom::kDefaultKeyringId));

  // bytes is nullptr
  EXPECT_FALSE(service.GetPrefInBytesForKeyring(kEncryptedMnemonic, nullptr,
                                                mojom::kDefaultKeyringId));

  // non-existing pref
  mnemonic.clear();
  EXPECT_FALSE(service.GetPrefInBytesForKeyring("brave.nothinghere", &mnemonic,
                                                mojom::kDefaultKeyringId));

  // non-string pref
  mnemonic.clear();
  KeyringService::SetPrefForKeyring(GetPrefs(), "test_num", base::Value(123),
                                    mojom::kDefaultKeyringId);
  EXPECT_FALSE(service.GetPrefInBytesForKeyring("test_num", &mnemonic,
                                                mojom::kDefaultKeyringId));
}

TEST_F(KeyringServiceUnitTest, SetPrefInBytesForKeyring) {
  const uint8_t bytes_array[] = {0xde, 0xad, 0xbe, 0xef};
  KeyringService service(json_rpc_service(), GetPrefs());
  service.SetPrefInBytesForKeyring(kEncryptedMnemonic, bytes_array,
                                   mojom::kDefaultKeyringId);
  EXPECT_EQ(
      GetStringPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId),
      "3q2+7w==");

  GetPrefs()->ClearPref(kBraveWalletKeyrings);
  const std::vector<uint8_t> bytes_vector = {0xde, 0xad, 0xbe, 0xef};
  service.SetPrefInBytesForKeyring(kEncryptedMnemonic, bytes_vector,
                                   mojom::kDefaultKeyringId);
  EXPECT_EQ(
      GetStringPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId),
      "3q2+7w==");
}

TEST_F(KeyringServiceUnitTest, GetOrCreateNonceForKeyring) {
  std::string encoded_nonce;
  std::string encoded_nonce2;
  {
    KeyringService service(json_rpc_service(), GetPrefs());
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
    KeyringService service(json_rpc_service(), GetPrefs());
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
    KeyringService service(json_rpc_service(), GetPrefs());
    const std::vector<uint8_t> nonce =
        service.GetOrCreateNonceForKeyring(mojom::kDefaultKeyringId);
    EXPECT_NE(base::Base64Encode(nonce), encoded_nonce);
    const std::vector<uint8_t> nonce2 =
        service.GetOrCreateNonceForKeyring("keyring2");
    EXPECT_NE(base::Base64Encode(nonce2), encoded_nonce2);
  }
}

TEST_F(KeyringServiceUnitTest, CreateEncryptorForKeyring) {
  std::string encoded_salt;
  std::string encoded_salt2;
  {
    KeyringService service(json_rpc_service(), GetPrefs());
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
    KeyringService service(json_rpc_service(), GetPrefs());
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
    KeyringService service(json_rpc_service(), GetPrefs());
    EXPECT_FALSE(
        service.CreateEncryptorForKeyring("", mojom::kDefaultKeyringId));
    ASSERT_TRUE(service.encryptors_.empty());
    EXPECT_FALSE(service.CreateEncryptorForKeyring("", "keyring2"));
    ASSERT_TRUE(service.encryptors_.empty());
  }
}

TEST_F(KeyringServiceUnitTest, CreateDefaultKeyringInternal) {
  KeyringService service(json_rpc_service(), GetPrefs());

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
    KeyringService service(json_rpc_service(), GetPrefs());
    EXPECT_EQ(service.CreateKeyring(mojom::kDefaultKeyringId, ""), nullptr);
    EXPECT_FALSE(
        HasPrefForKeyring(kPasswordEncryptorSalt, mojom::kDefaultKeyringId));
    EXPECT_FALSE(
        HasPrefForKeyring(kPasswordEncryptorNonce, mojom::kDefaultKeyringId));
    EXPECT_FALSE(
        HasPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId));

    HDKeyring* keyring =
        service.CreateKeyring(mojom::kDefaultKeyringId, "brave1");
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
    keyring = service.CreateKeyring(mojom::kDefaultKeyringId, "brave2");
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
  KeyringService service(json_rpc_service(), GetPrefs());
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
  EXPECT_EQ(service.GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId)
                ->GetAccountsNumber(),
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
  EXPECT_EQ(service.GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId)
                ->GetAccountsNumber(),
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
  EXPECT_EQ(service.GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId)
                ->GetAccountsNumber(),
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
  EXPECT_EQ(service.GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId)
                ->GetAccountsNumber(),
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
  EXPECT_EQ(service.GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId)
                ->GetAccountsNumber(),
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
    KeyringService service(json_rpc_service(), GetPrefs());
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
    KeyringService service(json_rpc_service(), GetPrefs());
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
    KeyringService service(json_rpc_service(), GetPrefs());
    // wrong password
    EXPECT_FALSE(Unlock(&service, "brave123"));
    ASSERT_TRUE(service.IsLocked());
    // empty password
    EXPECT_FALSE(Unlock(&service, ""));
    ASSERT_TRUE(service.IsLocked());
  }
}

TEST_F(KeyringServiceUnitTest, GetMnemonicForDefaultKeyring) {
  KeyringService service(json_rpc_service(), GetPrefs());
  ASSERT_TRUE(
      service.CreateEncryptorForKeyring("brave", mojom::kDefaultKeyringId));

  // no pref exists yet
  EXPECT_TRUE(GetMnemonicForDefaultKeyring(&service).empty());

  ASSERT_TRUE(service.CreateKeyringInternal(mojom::kDefaultKeyringId,
                                            kMnemonic1, false));
  EXPECT_EQ(GetMnemonicForDefaultKeyring(&service), kMnemonic1);

  // Lock service
  service.Lock();
  ASSERT_TRUE(service.IsLocked(mojom::kDefaultKeyringId));
  EXPECT_TRUE(GetMnemonicForDefaultKeyring(&service).empty());

  // unlock with wrong password
  ASSERT_FALSE(Unlock(&service, "brave123"));
  ASSERT_TRUE(service.IsLocked());

  EXPECT_TRUE(GetMnemonicForDefaultKeyring(&service).empty());

  ASSERT_TRUE(Unlock(&service, "brave"));
  ASSERT_FALSE(service.IsLocked());

  EXPECT_EQ(GetMnemonicForDefaultKeyring(&service), kMnemonic1);
}

TEST_F(KeyringServiceUnitTest, ValidatePassword) {
  KeyringService service(json_rpc_service(), GetPrefs());
  absl::optional<std::string> mnemonic = CreateWallet(&service, "brave");
  ASSERT_TRUE(mnemonic);

  EXPECT_TRUE(ValidatePassword(&service, "brave"));
  EXPECT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
  EXPECT_FALSE(ValidatePassword(&service, "brave123"));
  EXPECT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
}

TEST_F(KeyringServiceUnitTest, GetKeyringInfo) {
  KeyringService service(json_rpc_service(), GetPrefs());

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
#if BUILDFLAG(IS_ANDROID)
  EXPECT_TRUE(IsKeyringInfoEmpty(&service, mojom::kSolanaKeyringId));
#endif
  EXPECT_TRUE(IsKeyringInfoEmpty(&service, "invalid_id"));
}

TEST_F(KeyringServiceUnitTest, LockAndUnlock) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeatures(
      {brave_wallet::features::kBraveWalletFilecoinFeature,
       brave_wallet::features::kBraveWalletSolanaFeature},
      {});
  {
    KeyringService service(json_rpc_service(), GetPrefs());
    // No encryptor
    service.Lock();
    EXPECT_TRUE(service.IsLocked());
    EXPECT_TRUE(service.IsLocked(mojom::kFilecoinKeyringId));
    EXPECT_TRUE(service.IsLocked(mojom::kSolanaKeyringId));

    EXPECT_TRUE(
        service.CreateEncryptorForKeyring("123", mojom::kDefaultKeyringId));
    EXPECT_FALSE(service.IsLocked());
    EXPECT_TRUE(service.IsLocked(mojom::kFilecoinKeyringId));
    EXPECT_TRUE(service.IsLocked(mojom::kSolanaKeyringId));

    EXPECT_TRUE(
        service.CreateEncryptorForKeyring("123", mojom::kFilecoinKeyringId));
    EXPECT_FALSE(service.IsLocked());
    EXPECT_FALSE(service.IsLocked(mojom::kFilecoinKeyringId));
    EXPECT_TRUE(service.IsLocked(mojom::kSolanaKeyringId));

    EXPECT_TRUE(
        service.CreateEncryptorForKeyring("123", mojom::kSolanaKeyringId));
    EXPECT_FALSE(service.IsLocked());
    EXPECT_FALSE(service.IsLocked(mojom::kFilecoinKeyringId));
    EXPECT_FALSE(service.IsLocked(mojom::kSolanaKeyringId));

    // No default keyring
    service.Lock();
    EXPECT_TRUE(service.IsLocked());
    EXPECT_TRUE(service.IsLocked(mojom::kFilecoinKeyringId));
    EXPECT_TRUE(service.IsLocked(mojom::kSolanaKeyringId));
  }
  {
    KeyringService service(json_rpc_service(), GetPrefs());
    ASSERT_NE(
        service.CreateKeyring(brave_wallet::mojom::kDefaultKeyringId, "brave"),
        nullptr);
    ASSERT_TRUE(AddAccount(&service, "ETH Account 1", mojom::CoinType::ETH));
    EXPECT_FALSE(service.IsLocked());
    EXPECT_TRUE(service.IsLocked(mojom::kFilecoinKeyringId));
    EXPECT_TRUE(service.IsLocked(mojom::kSolanaKeyringId));
    ASSERT_NE(
        service.CreateKeyring(brave_wallet::mojom::kFilecoinKeyringId, "brave"),
        nullptr);
    ASSERT_TRUE(AddAccount(&service, "FIL Account 1", mojom::CoinType::FIL));
    ASSERT_NE(
        service.CreateKeyring(brave_wallet::mojom::kSolanaKeyringId, "brave"),
        nullptr);
    ASSERT_TRUE(AddAccount(&service, "SOL Account 1", mojom::CoinType::SOL));
    EXPECT_FALSE(service.IsLocked(mojom::kDefaultKeyringId));
    EXPECT_FALSE(service.IsLocked(mojom::kFilecoinKeyringId));
    EXPECT_FALSE(service.IsLocked(mojom::kSolanaKeyringId));

    service.Lock();
    EXPECT_TRUE(service.IsLocked());
    EXPECT_TRUE(service.IsLocked(mojom::kFilecoinKeyringId));
    EXPECT_TRUE(service.IsLocked(mojom::kSolanaKeyringId));
    EXPECT_FALSE(
        service.GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId));
    EXPECT_FALSE(
        service.GetHDKeyringById(brave_wallet::mojom::kFilecoinKeyringId));
    EXPECT_FALSE(
        service.GetHDKeyringById(brave_wallet::mojom::kSolanaKeyringId));

    EXPECT_FALSE(Unlock(&service, "abc"));
    EXPECT_TRUE(service.IsLocked());
    EXPECT_TRUE(service.IsLocked(mojom::kFilecoinKeyringId));
    EXPECT_TRUE(service.IsLocked(mojom::kSolanaKeyringId));

    EXPECT_TRUE(Unlock(&service, "brave"));
    EXPECT_FALSE(service.IsLocked());
    EXPECT_FALSE(service.IsLocked(mojom::kFilecoinKeyringId));
    EXPECT_FALSE(service.IsLocked(mojom::kSolanaKeyringId));
  }
}

TEST_F(KeyringServiceUnitTest, Reset) {
  KeyringService service(json_rpc_service(), GetPrefs());
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
  KeyringService service(json_rpc_service(), GetPrefs());

  EXPECT_FALSE(IsWalletBackedUp(&service));

  service.NotifyWalletBackupComplete();

  EXPECT_TRUE(IsWalletBackedUp(&service));

  service.Reset();

  EXPECT_FALSE(IsWalletBackedUp(&service));
}

TEST_F(KeyringServiceUnitTest, AccountMetasForKeyring) {
  KeyringService service(json_rpc_service(), GetPrefs());
  EXPECT_TRUE(
      service.CreateEncryptorForKeyring("brave", mojom::kDefaultKeyringId));
  ASSERT_TRUE(service.CreateKeyringInternal(
      brave_wallet::mojom::kDefaultKeyringId, kMnemonic1, false));
  auto* default_keyring =
      service.GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId);
  default_keyring->AddAccounts(2);
  const std::string address1 = default_keyring->GetAddress(0);
  const std::string name1 = "Account1";
  const std::string account_path1 = KeyringService::GetAccountPathByIndex(0);
  const std::string address2 = default_keyring->GetAddress(1);
  const std::string name2 = "Account2";
  const std::string account_path2 = KeyringService::GetAccountPathByIndex(1);

  KeyringService::SetAccountMetaForKeyring(GetPrefs(), account_path1, name1,
                                           address1, mojom::kDefaultKeyringId);
  KeyringService::SetAccountMetaForKeyring(GetPrefs(), account_path2, name2,
                                           address2, mojom::kDefaultKeyringId);

  const base::Value* account_metas = KeyringService::GetPrefForKeyring(
      GetPrefs(), kAccountMetas, mojom::kDefaultKeyringId);
  ASSERT_NE(account_metas, nullptr);

  EXPECT_EQ(
      account_metas->FindPath(account_path1 + ".account_name")->GetString(),
      name1);
  EXPECT_EQ(
      account_metas->FindPath(account_path2 + ".account_name")->GetString(),
      name2);
  EXPECT_EQ(KeyringService::GetAccountNameForKeyring(GetPrefs(), account_path1,
                                                     mojom::kDefaultKeyringId),
            name1);
  EXPECT_EQ(KeyringService::GetAccountAddressForKeyring(
                GetPrefs(), account_path1, mojom::kDefaultKeyringId),
            address1);
  EXPECT_EQ(KeyringService::GetAccountNameForKeyring(GetPrefs(), account_path2,
                                                     mojom::kDefaultKeyringId),
            name2);
  EXPECT_EQ(KeyringService::GetAccountAddressForKeyring(
                GetPrefs(), account_path2, mojom::kDefaultKeyringId),
            address2);
  EXPECT_EQ(service.GetAccountMetasNumberForKeyring(mojom::kDefaultKeyringId),
            2u);
  EXPECT_EQ(service.GetAccountMetasNumberForKeyring("keyring1"), 0u);

  // GetAccountInfosForKeyring should work even if the keyring is locked
  service.Lock();
  std::vector<mojom::AccountInfoPtr> account_infos =
      service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
  EXPECT_EQ(account_infos.size(), 2u);
  EXPECT_EQ(account_infos[0]->address, address1);
  EXPECT_EQ(account_infos[0]->name, name1);
  EXPECT_EQ(account_infos[1]->address, address2);
  EXPECT_EQ(account_infos[1]->name, name2);
}

TEST_F(KeyringServiceUnitTest, CreateAndRestoreWallet) {
  KeyringService service(json_rpc_service(), GetPrefs());

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
  EXPECT_FALSE(account_infos[0]->address.empty());
  const std::string address0 = account_infos[0]->address;
  EXPECT_EQ(account_infos[0]->name, "Account 1");

  service.Reset();

  auto verify_restore_wallet = base::BindLambdaForTesting(
      [&mnemonic_to_be_restored, &service, &address0]() {
        EXPECT_TRUE(
            RestoreWallet(&service, *mnemonic_to_be_restored, "brave1", false));
        std::vector<mojom::AccountInfoPtr> account_infos =
            service.GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
        EXPECT_EQ(account_infos.size(), 1u);
        EXPECT_EQ(account_infos[0]->address, address0);
        EXPECT_EQ(account_infos[0]->name, "Account 1");
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

TEST_F(KeyringServiceUnitTest, AddAccount) {
  KeyringService service(json_rpc_service(), GetPrefs());
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

TEST_F(KeyringServiceUnitTest, GetAccountPathByIndex) {
  EXPECT_EQ(KeyringService::GetAccountPathByIndex(0), "m/44'/60'/0'/0/0");
  EXPECT_EQ(KeyringService::GetAccountPathByIndex(3), "m/44'/60'/0'/0/3");
  EXPECT_EQ(KeyringService::GetAccountPathByIndex(0, mojom::kFilecoinKeyringId),
            "m/44'/461'/0'/0/0");
  EXPECT_EQ(KeyringService::GetAccountPathByIndex(3, mojom::kFilecoinKeyringId),
            "m/44'/461'/0'/0/3");
  EXPECT_EQ(KeyringService::GetAccountPathByIndex(0, mojom::kSolanaKeyringId),
            "m/44'/501'/0'/0'");
  EXPECT_EQ(KeyringService::GetAccountPathByIndex(3, mojom::kSolanaKeyringId),
            "m/44'/501'/3'/0'");
}

TEST_F(KeyringServiceUnitTest, MigrationImportedFilecoinPrefs) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      brave_wallet::features::kBraveWalletFilecoinFeature);
  base::Value imported_accounts(base::Value::Type::LIST);
  base::Value f_address(base::Value::Type::DICTIONARY);
  f_address.SetStringKey(kAccountAddress,
                         "f1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q");
  f_address.SetStringKey("account_name", "1");
  f_address.SetIntKey("coin_type", 461);
  f_address.SetStringKey("encrypted_private_key", "key");
  base::Value t_address(base::Value::Type::DICTIONARY);
  t_address.SetStringKey(kAccountAddress,
                         "t3h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q");
  t_address.SetStringKey("account_name", "2");
  t_address.SetIntKey("coin_type", 461);
  t_address.SetStringKey("encrypted_private_key", "key2");
  imported_accounts.Append(f_address.Clone());
  imported_accounts.Append(t_address.Clone());

  base::Value filecoin_keyring(base::Value::Type::DICTIONARY);
  filecoin_keyring.SetKey(kImportedAccounts, std::move(imported_accounts));
  base::Value wallet_keyrings(base::Value::Type::DICTIONARY);
  wallet_keyrings.SetKey("filecoin", std::move(filecoin_keyring));
  GetPrefs()->Set(kBraveWalletKeyrings, std::move(wallet_keyrings));
  KeyringService::MigrateObsoleteProfilePrefs(GetPrefs());
  {
    SetNetwork(brave_wallet::mojom::kFilecoinMainnet, mojom::CoinType::FIL);
    auto* value = KeyringService::GetImportedAccountsPrefForKeyring(
        GetPrefs(), mojom::kFilecoinKeyringId);
    EXPECT_TRUE(value->is_list());
    EXPECT_EQ(value->GetList().size(), 1u);
    EXPECT_EQ(value->GetList()[0], f_address);
  }
  {
    SetNetwork(brave_wallet::mojom::kFilecoinTestnet, mojom::CoinType::FIL);
    auto* value = KeyringService::GetImportedAccountsPrefForKeyring(
        GetPrefs(), mojom::kFilecoinKeyringId);
    EXPECT_TRUE(value->is_list());
    EXPECT_EQ(value->GetList().size(), 1u);
    EXPECT_EQ(value->GetList()[0], t_address);
  }
  {
    SetNetwork(brave_wallet::mojom::kLocalhostChainId, mojom::CoinType::FIL);
    auto* value = KeyringService::GetImportedAccountsPrefForKeyring(
        GetPrefs(), mojom::kFilecoinKeyringId);
    EXPECT_TRUE(value->is_list());
    EXPECT_EQ(value->GetList().size(), 1u);
    EXPECT_EQ(value->GetList()[0], t_address);
  }
}

TEST_F(KeyringServiceUnitTest, MigrationImportedHardwareFilecoinPrefs) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      brave_wallet::features::kBraveWalletFilecoinFeature);
  base::Value account_metas(base::Value::Type::DICTIONARY);
  base::Value f_address(base::Value::Type::DICTIONARY);
  f_address.SetStringKey(kAccountAddress,
                         "f1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q");
  f_address.SetStringKey("account_name", "1");
  f_address.SetIntKey("coin_type", 461);
  f_address.SetStringKey("encrypted_private_key", "key");
  base::Value t_address(base::Value::Type::DICTIONARY);
  t_address.SetStringKey(kAccountAddress,
                         "t3h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q");
  t_address.SetStringKey("account_name", "2");
  t_address.SetIntKey("coin_type", 461);
  t_address.SetStringKey("encrypted_private_key", "key2");
  account_metas.SetKey("f1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                       f_address.Clone());
  account_metas.SetKey("t3h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                       t_address.Clone());
  base::Value device(base::Value::Type::DICTIONARY);
  EXPECT_EQ(account_metas.GetDict().size(), 2u);
  device.SetKey("Ledger", std::move(account_metas));

  base::Value filecoin_keyring(base::Value::Type::DICTIONARY);
  filecoin_keyring.SetKey("hardware", std::move(device));
  base::Value wallet_keyrings(base::Value::Type::DICTIONARY);
  wallet_keyrings.SetKey("filecoin", std::move(filecoin_keyring));
  GetPrefs()->Set(kBraveWalletKeyrings, std::move(wallet_keyrings));
  KeyringService::MigrateObsoleteProfilePrefs(GetPrefs());
  {
    SetNetwork(brave_wallet::mojom::kFilecoinMainnet, mojom::CoinType::FIL);
    auto* value = KeyringService::GetHardwareAccountsPrefForKeyringUpdate(
        GetPrefs(), mojom::kFilecoinKeyringId);

    EXPECT_TRUE(value->is_dict());
    auto* device = value->FindDictPath("Ledger");
    EXPECT_TRUE(device);
    EXPECT_EQ(device->GetDict().size(), 1u);
    EXPECT_EQ(
        *value->FindPath("Ledger.f1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q"),
        f_address);
  }
  {
    SetNetwork(brave_wallet::mojom::kFilecoinTestnet, mojom::CoinType::FIL);
    auto* value = KeyringService::GetHardwareAccountsPrefForKeyringUpdate(
        GetPrefs(), mojom::kFilecoinKeyringId);
    EXPECT_TRUE(value->is_dict());
    auto* ledger = value->FindDictPath("Ledger");
    EXPECT_TRUE(ledger);
    EXPECT_EQ(ledger->GetDict().size(), 1u);
    EXPECT_EQ(
        *value->FindPath("Ledger.t3h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q"),
        t_address);
  }
  {
    SetNetwork(brave_wallet::mojom::kLocalhostChainId, mojom::CoinType::FIL);
    auto* value = KeyringService::GetHardwareAccountsPrefForKeyringUpdate(
        GetPrefs(), mojom::kFilecoinKeyringId);
    EXPECT_TRUE(value->is_dict());
    auto* ledger = value->FindDictPath("Ledger");
    EXPECT_TRUE(ledger);
    EXPECT_EQ(ledger->GetDict().size(), 1u);
    EXPECT_EQ(
        *value->FindPath("Ledger.t3h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q"),
        t_address);
  }
}

TEST_F(KeyringServiceUnitTest, MigrationPrefs) {
  GetPrefs()->SetString(kBraveWalletPasswordEncryptorSalt, "test_salt");
  GetPrefs()->SetString(kBraveWalletPasswordEncryptorNonce, "test_nonce");
  GetPrefs()->SetString(kBraveWalletEncryptedMnemonic, "test_mnemonic");
  GetPrefs()->SetString(kBraveWalletSelectedAccount, "0x111");
  GetPrefs()->SetInteger(kBraveWalletDefaultKeyringAccountNum, 3);
  GetPrefs()->Set(kBraveWalletKeyrings, GetHardwareKeyringValueForTesting());
  EXPECT_EQ(
      *GetPrefs()
           ->Get(kBraveWalletKeyrings)
           ->FindStringPath("hardware.A1.account_metas.0x111.account_name"),
      "test1");

  base::Value account_names(base::Value::Type::LIST);
  account_names.Append(base::Value("Account1"));
  account_names.Append(base::Value("Account2"));
  account_names.Append(base::Value("Account3"));
  GetPrefs()->Set(kBraveWalletAccountNames, account_names);

  GetPrefs()->SetBoolean(kBraveWalletBackupComplete, true);

  KeyringService::MigrateObsoleteProfilePrefs(GetPrefs());

  EXPECT_EQ(
      GetStringPrefForKeyring(kPasswordEncryptorSalt, mojom::kDefaultKeyringId),
      "test_salt");
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorNonce,
                                    mojom::kDefaultKeyringId),
            "test_nonce");
  EXPECT_EQ(
      GetStringPrefForKeyring(kEncryptedMnemonic, mojom::kDefaultKeyringId),
      "test_mnemonic");

  const base::Value* backup_complete = KeyringService::GetPrefForKeyring(
      GetPrefs(), kBackupComplete, mojom::kDefaultKeyringId);
  ASSERT_TRUE(backup_complete);
  EXPECT_TRUE(backup_complete->GetBool());

  const base::Value* selected_account = KeyringService::GetPrefForKeyring(
      GetPrefs(), kSelectedAccount, mojom::kDefaultKeyringId);
  ASSERT_TRUE(selected_account);
  EXPECT_EQ(selected_account->GetString(), "0x111");
  ASSERT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletSelectedAccount));

  const base::Value* account_metas = KeyringService::GetPrefForKeyring(
      GetPrefs(), kAccountMetas, mojom::kDefaultKeyringId);
  EXPECT_EQ(account_metas->DictSize(), 3u);
  EXPECT_EQ(KeyringService::GetAccountNameForKeyring(
                GetPrefs(), KeyringService::GetAccountPathByIndex(0),
                mojom::kDefaultKeyringId),
            "Account1");
  EXPECT_EQ(KeyringService::GetAccountNameForKeyring(
                GetPrefs(), KeyringService::GetAccountPathByIndex(1),
                mojom::kDefaultKeyringId),
            "Account2");
  EXPECT_EQ(KeyringService::GetAccountNameForKeyring(
                GetPrefs(), KeyringService::GetAccountPathByIndex(2),
                mojom::kDefaultKeyringId),
            "Account3");

  const base::Value* hardware_accounts = KeyringService::GetPrefForKeyring(
      GetPrefs(), kHardwareAccounts, mojom::kDefaultKeyringId);
  EXPECT_EQ(hardware_accounts->DictSize(), 2u);
  EXPECT_EQ(
      *hardware_accounts->FindStringPath("A1.account_metas.0x111.account_name"),
      "test1");
  EXPECT_EQ(*hardware_accounts->FindStringPath(
                "A1.account_metas.0x111.derivation_path"),
            "path1");
  EXPECT_EQ(*hardware_accounts->FindStringPath(
                "A1.account_metas.0x111.hardware_vendor"),
            "vendor1");

  EXPECT_EQ(
      *hardware_accounts->FindStringPath("B2.account_metas.0x222.account_name"),
      "test2");
  EXPECT_EQ(*hardware_accounts->FindStringPath(
                "B2.account_metas.0x222.derivation_path"),
            "path2");
  EXPECT_EQ(*hardware_accounts->FindStringPath(
                "B2.account_metas.0x222.hardware_vendor"),
            "vendor2");
  ASSERT_FALSE(
      GetPrefs()
          ->Get(kBraveWalletKeyrings)
          ->FindStringPath("hardware.A1.account_metas.0x111.account_name"));
}

TEST_F(KeyringServiceUnitTest, MigrationPrefsFailSafe) {
  GetPrefs()->SetInteger(kBraveWalletDefaultKeyringAccountNum, 2);

  base::Value account_names(base::Value::Type::LIST);
  account_names.Append(base::Value("Account1"));
  account_names.Append(base::Value("Account2"));
  account_names.Append(base::Value("Account3"));
  GetPrefs()->Set(kBraveWalletAccountNames, account_names);

  KeyringService::MigrateObsoleteProfilePrefs(GetPrefs());
  const base::Value* account_metas = KeyringService::GetPrefForKeyring(
      GetPrefs(), kAccountMetas, mojom::kDefaultKeyringId);
  EXPECT_EQ(account_metas->DictSize(), 1u);
  EXPECT_EQ(KeyringService::GetAccountNameForKeyring(
                GetPrefs(), KeyringService::GetAccountPathByIndex(0),
                mojom::kDefaultKeyringId),
            "Account 1");
}

TEST_F(KeyringServiceUnitTest, ImportedAccounts) {
  KeyringService service(json_rpc_service(), GetPrefs());

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  ASSERT_TRUE(CreateWallet(&service, "brave"));
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
  for (size_t i = 0;
       i < sizeof(imported_accounts) / sizeof(imported_accounts[0]); ++i) {
    absl::optional<std::string> imported_account =
        ImportAccount(&service, imported_accounts[i].name,
                      imported_accounts[i].private_key, mojom::CoinType::ETH);
    ASSERT_TRUE(imported_account.has_value());
    EXPECT_EQ(imported_accounts[i].address, *imported_account);

    std::string private_key;
    EXPECT_TRUE(
        GetPrivateKeyForImportedAccount(&service, imported_accounts[i].address,
                                        mojom::CoinType::ETH, &private_key));
    EXPECT_EQ(imported_accounts[i].private_key, private_key);
  }

  observer.Reset();
  EXPECT_TRUE(RemoveImportedAccount(&service, imported_accounts[1].address,
                                    mojom::CoinType::ETH));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());

  observer.Reset();
  // remove invalid address
  EXPECT_FALSE(
      RemoveImportedAccount(&service, "0xxxxxxxxxx0", mojom::CoinType::ETH));
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
  std::string private_key;
  EXPECT_FALSE(
      GetPrivateKeyForImportedAccount(&service, imported_accounts[0].address,
                                      mojom::CoinType::ETH, &private_key));
  EXPECT_TRUE(private_key.empty());

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

  // private key should also be available now
  private_key.clear();
  EXPECT_TRUE(
      GetPrivateKeyForImportedAccount(&service, imported_accounts[0].address,
                                      mojom::CoinType::ETH, &private_key));
  EXPECT_EQ(imported_accounts[0].private_key, private_key);

  // Imported accounts should also be restored in default keyring
  auto* default_keyring =
      service.GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId);
  EXPECT_EQ(default_keyring->GetImportedAccountsNumber(), 2u);

  const base::Value* imported_accounts_value =
      KeyringService::GetPrefForKeyring(GetPrefs(), kImportedAccounts,
                                        mojom::kDefaultKeyringId);
  ASSERT_TRUE(imported_accounts_value);
  EXPECT_EQ(imported_accounts_value->GetList()[0]
                .FindKey(kAccountAddress)
                ->GetString(),
            imported_accounts[0].address);
  // private key is encrypted
  const std::string encrypted_private_key =
      imported_accounts_value->GetList()[0]
          .FindKey(kEncryptedPrivateKey)
          ->GetString();
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

  KeyringService service(json_rpc_service(), GetPrefs());
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
  std::string private_key;
  EXPECT_TRUE(GetPrivateKeyForImportedAccount(
      &service, expected_address, mojom::CoinType::ETH, &private_key));
  EXPECT_EQ(expected_private_key, private_key);

  // private key is encrypted
  const base::Value* imported_accounts_value =
      KeyringService::GetPrefForKeyring(GetPrefs(), kImportedAccounts,
                                        mojom::kDefaultKeyringId);
  ASSERT_TRUE(imported_accounts_value);
  const std::string encrypted_private_key =
      imported_accounts_value->GetList()[0]
          .FindKey(kEncryptedPrivateKey)
          ->GetString();
  EXPECT_FALSE(encrypted_private_key.empty());

  std::vector<uint8_t> private_key_bytes;
  ASSERT_TRUE(base::HexStringToBytes(expected_private_key, &private_key_bytes));
  EXPECT_NE(encrypted_private_key, base::Base64Encode(private_key_bytes));
}

TEST_F(KeyringServiceUnitTest, GetPrivateKeyForKeyringAccount) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      brave_wallet::features::kBraveWalletSolanaFeature);
  KeyringService service(json_rpc_service(), GetPrefs());
  ASSERT_TRUE(RestoreWallet(&service, kMnemonic1, "brave", false));

  absl::optional<std::string> private_key = GetPrivateKeyForKeyringAccount(
      &service, "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db",
      mojom::CoinType::ETH);
  ASSERT_TRUE(private_key.has_value());
  EXPECT_EQ(*private_key,
            "919af8081ce2a02d9650bf3e10ffb6b7cbadbb1dca749122d7d982cdb6cbcc50");

  // account not added yet
  EXPECT_FALSE(GetPrivateKeyForKeyringAccount(
      &service, "0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0",
      mojom::CoinType::ETH));
  ASSERT_TRUE(AddAccount(&service, "Account 2", mojom::CoinType::ETH));

  private_key = GetPrivateKeyForKeyringAccount(
      &service, "0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0",
      mojom::CoinType::ETH);
  ASSERT_TRUE(private_key.has_value());
  EXPECT_EQ(*private_key,
            "17c31fdade7d84f22462f398df300405a76fc11b1fe5a9e286dc8c3b0913e31c");

  EXPECT_FALSE(
      GetPrivateKeyForKeyringAccount(&service, "", mojom::CoinType::ETH));
  EXPECT_FALSE(
      GetPrivateKeyForKeyringAccount(&service, "0x123", mojom::CoinType::ETH));

  // Other keyrings
  // account not added yet
  EXPECT_FALSE(GetPrivateKeyForKeyringAccount(
      &service, "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
      mojom::CoinType::SOL));
  ASSERT_TRUE(AddAccount(&service, "Account 1", mojom::CoinType::SOL));
  private_key = GetPrivateKeyForKeyringAccount(
      &service, "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
      mojom::CoinType::SOL);
  ASSERT_TRUE(private_key.has_value());
  EXPECT_EQ(*private_key,
            "LNWjgQq8NhxWTUhz9jAD7koZfsKDwdJuLmVHyMxfjaFAamqXbtyUd3TcYQV2vPeRoM"
            "58gw7Ez8qsvKSZee6KdUQ");
}

TEST_F(KeyringServiceUnitTest, GetKeyringIdForCoin) {
  EXPECT_EQ(KeyringService::GetKeyringIdForCoin(mojom::CoinType::FIL),
            mojom::kFilecoinKeyringId);
  EXPECT_EQ(KeyringService::GetKeyringIdForCoin(mojom::CoinType::SOL),
            mojom::kSolanaKeyringId);
  EXPECT_EQ(KeyringService::GetKeyringIdForCoin(mojom::CoinType::ETH),
            mojom::kDefaultKeyringId);
}

TEST_F(KeyringServiceUnitTest, SetDefaultKeyringDerivedAccountMeta) {
  KeyringService service(json_rpc_service(), GetPrefs());

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  const std::string kUpdatedName = "Updated";
  ASSERT_FALSE(observer.AccountsChangedFired());

  // no keyring yet
  EXPECT_FALSE(SetKeyringDerivedAccountName(
      &service, mojom::kDefaultKeyringId,
      "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db", kUpdatedName));

  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.AccountsChangedFired());
  observer.Reset();

  ASSERT_TRUE(RestoreWallet(&service, kMnemonic1, "brave", false));

  auto* default_keyring =
      service.GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId);
  default_keyring->AddAccounts(1);
  const std::string address1 = default_keyring->GetAddress(0);
  const std::string name1 = "Account1";
  const std::string account_path1 = KeyringService::GetAccountPathByIndex(0);
  const std::string address2 = default_keyring->GetAddress(1);
  const std::string name2 = "Account2";
  const std::string account_path2 = KeyringService::GetAccountPathByIndex(1);

  KeyringService::SetAccountMetaForKeyring(GetPrefs(), account_path1, name1,
                                           address1, mojom::kDefaultKeyringId);
  KeyringService::SetAccountMetaForKeyring(GetPrefs(), account_path2, name2,
                                           address2, mojom::kDefaultKeyringId);
  EXPECT_EQ(KeyringService::GetAccountNameForKeyring(GetPrefs(), account_path1,
                                                     mojom::kDefaultKeyringId),
            name1);
  EXPECT_EQ(KeyringService::GetAccountAddressForKeyring(
                GetPrefs(), account_path1, mojom::kDefaultKeyringId),
            address1);
  EXPECT_EQ(KeyringService::GetAccountNameForKeyring(GetPrefs(), account_path2,
                                                     mojom::kDefaultKeyringId),
            name2);
  EXPECT_EQ(KeyringService::GetAccountAddressForKeyring(
                GetPrefs(), account_path2, mojom::kDefaultKeyringId),
            address2);

  ASSERT_FALSE(observer.AccountsChangedFired());
  // empty address
  EXPECT_FALSE(SetKeyringDerivedAccountName(&service, mojom::kDefaultKeyringId,
                                            "", kUpdatedName));
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
                                           address2, kUpdatedName));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();

  EXPECT_EQ(KeyringService::GetAccountNameForKeyring(GetPrefs(), account_path1,
                                                     mojom::kDefaultKeyringId),
            name1);
  EXPECT_EQ(KeyringService::GetAccountNameForKeyring(GetPrefs(), account_path2,
                                                     mojom::kDefaultKeyringId),
            kUpdatedName);
}

TEST_F(KeyringServiceUnitTest, SetDefaultKeyringImportedAccountName) {
  KeyringService service(json_rpc_service(), GetPrefs());

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  ASSERT_TRUE(CreateWallet(&service, "barve"));

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
  for (size_t i = 0;
       i < sizeof(imported_accounts) / sizeof(imported_accounts[0]); ++i) {
    ASSERT_FALSE(observer.AccountsChangedFired());

    absl::optional<std::string> imported_account =
        ImportAccount(&service, imported_accounts[i].name,
                      imported_accounts[i].private_key, mojom::CoinType::ETH);
    ASSERT_TRUE(imported_account.has_value());
    EXPECT_EQ(imported_accounts[i].address, *imported_account);

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
  for (size_t i = 0;
       i < sizeof(imported_accounts) / sizeof(imported_accounts[0]); ++i) {
    std::string private_key;
    EXPECT_TRUE(
        GetPrivateKeyForImportedAccount(&service, imported_accounts[i].address,
                                        mojom::CoinType::ETH, &private_key));
    EXPECT_EQ(imported_accounts[i].private_key, private_key);
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
  KeyringService service(json_rpc_service(), GetPrefs());
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
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      brave_wallet::features::kBraveWalletFilecoinFeature);

  KeyringService service(json_rpc_service(), GetPrefs());

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  ASSERT_TRUE(CreateWallet(&service, "brave"));
  auto* default_keyring =
      service.GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId);
  std::string first_account = default_keyring->GetAddress(0);
  EXPECT_FALSE(service.IsHardwareAccount(first_account));
  std::vector<mojom::HardwareWalletAccountPtr> new_accounts;
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0x111", "m/44'/60'/1'/0/0", "name 1", "Ledger", "device1",
      mojom::CoinType::ETH));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "t1h3n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "m/44'/461'/0'/0/0",
      "name 2", "Ledger", "device1", mojom::CoinType::FIL));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0xEA0", "m/44'/60'/2'/0/0", "name 3", "Ledger", "device2",
      mojom::CoinType::ETH));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "t3h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "m/44'/461'/2'/0/0",
      "filecoin 1", "Ledger", "device2", mojom::CoinType::FIL));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0x222", "m/44'/60'/3'/0/0", "name 4", "Ledger", "device1",
      mojom::CoinType::ETH));
  SetNetwork(brave_wallet::mojom::kFilecoinTestnet, mojom::CoinType::FIL);
  EXPECT_FALSE(observer.AccountsChangedFired());
  service.AddHardwareAccounts(std::move(new_accounts));
  EXPECT_TRUE(service.IsHardwareAccount("0x111"));
  EXPECT_TRUE(
      service.IsHardwareAccount("t1h3n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q"));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();
  for (const auto& account : new_accounts) {
    auto keyring_id = KeyringService::GetKeyringIdForCoin(account->coin);
    auto path = keyring_id + ".hardware." + account->device_id +
                ".account_metas." + account->address;
    ASSERT_TRUE(
        GetPrefs()->GetDictionary(kBraveWalletKeyrings)->FindPath(path));
  }
  {
    // Checking Default keyring accounts
    base::RunLoop run_loop;
    service.GetKeyringInfo(
        brave_wallet::mojom::kDefaultKeyringId,
        base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
          const auto& accounts = keyring_info->account_infos;
          EXPECT_EQ(accounts.size(), 4u);

          EXPECT_EQ(accounts[1]->address, "0x111");
          EXPECT_EQ(accounts[1]->name, "name 1");
          EXPECT_EQ(accounts[1]->is_imported, false);
          ASSERT_TRUE(accounts[1]->hardware);
          EXPECT_EQ(accounts[1]->hardware->device_id, "device1");
          EXPECT_EQ(accounts[1]->coin, mojom::CoinType::ETH);

          EXPECT_EQ(accounts[2]->address, "0x222");
          EXPECT_EQ(accounts[2]->name, "name 4");
          EXPECT_EQ(accounts[2]->is_imported, false);
          ASSERT_TRUE(accounts[2]->hardware);
          EXPECT_EQ(accounts[2]->hardware->device_id, "device1");
          EXPECT_EQ(accounts[2]->coin, mojom::CoinType::ETH);

          EXPECT_EQ(accounts[3]->address, "0xEA0");
          EXPECT_EQ(accounts[3]->name, "name 3");
          EXPECT_EQ(accounts[3]->is_imported, false);
          ASSERT_TRUE(accounts[3]->hardware);
          EXPECT_EQ(accounts[3]->hardware->device_id, "device2");
          EXPECT_EQ(accounts[3]->coin, mojom::CoinType::ETH);

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

          EXPECT_EQ(accounts[0]->address,
                    "t1h3n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q");
          EXPECT_EQ(accounts[0]->name, "name 2");
          EXPECT_EQ(accounts[0]->is_imported, false);
          ASSERT_TRUE(accounts[0]->hardware);
          EXPECT_EQ(accounts[0]->hardware->device_id, "device1");
          EXPECT_EQ(accounts[0]->coin, mojom::CoinType::FIL);

          EXPECT_EQ(accounts[1]->address,
                    "t3h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q");
          EXPECT_EQ(accounts[1]->name, "filecoin 1");
          EXPECT_EQ(accounts[1]->is_imported, false);
          ASSERT_TRUE(accounts[1]->hardware);
          EXPECT_EQ(accounts[1]->hardware->device_id, "device2");
          EXPECT_EQ(accounts[1]->coin, mojom::CoinType::FIL);

          run_loop.Quit();
        }));
    run_loop.Run();
  }
  ASSERT_FALSE(observer.AccountsChangedFired());
  service.RemoveHardwareAccount("0x111", mojom::CoinType::ETH);

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();
  ASSERT_FALSE(GetPrefs()
                   ->GetDictionary(kBraveWalletKeyrings)
                   ->FindPath("default.hardware.device1.account_metas.0x111"));

  ASSERT_FALSE(GetPrefs()
                   ->GetDictionary(kBraveWalletKeyrings)
                   ->FindPath("default.hardware.t.device1.account_metas."
                              "t1h3n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q"));

  ASSERT_TRUE(GetPrefs()
                  ->GetDictionary(kBraveWalletKeyrings)
                  ->FindPath("default.hardware.device2.account_metas.0xEA0"));

  ASSERT_TRUE(GetPrefs()
                  ->GetDictionary(kBraveWalletKeyrings)
                  ->FindPath("filecoin.hardware.t.device2.account_metas."
                             "t3h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q"));

  ASSERT_FALSE(observer.AccountsChangedFired());
  service.RemoveHardwareAccount("0xEA0", mojom::CoinType::ETH);

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();

  bool callback_called = false;
  service.GetKeyringInfo(
      brave_wallet::mojom::kDefaultKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        const auto& accounts = keyring_info->account_infos;
        EXPECT_EQ(accounts.size(), size_t(2));

        EXPECT_EQ(accounts[1]->address, "0x222");
        EXPECT_EQ(accounts[1]->name, "name 4");
        EXPECT_EQ(accounts[1]->is_imported, false);
        ASSERT_TRUE(accounts[1]->hardware);
        EXPECT_EQ(accounts[1]->hardware->device_id, "device1");
        EXPECT_EQ(accounts[1]->coin, mojom::CoinType::ETH);

        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  ASSERT_FALSE(observer.AccountsChangedFired());

  service.RemoveHardwareAccount("0x222", mojom::CoinType::ETH);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();

  ASSERT_FALSE(GetPrefs()
                   ->GetDictionary(kBraveWalletKeyrings)
                   ->FindPath("default.hardware.device2.account_metas.0xEA0"));

  ASSERT_FALSE(GetPrefs()
                   ->GetDictionary(kBraveWalletKeyrings)
                   ->FindPath("default.hardware.device2"));

  service.RemoveHardwareAccount("t3h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                                mojom::CoinType::FIL);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();
  ASSERT_FALSE(GetPrefs()
                   ->GetDictionary(kBraveWalletKeyrings)
                   ->FindPath("filecoin.hardware.device2.account_metas."
                              "t3h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q"));
}

TEST_F(KeyringServiceUnitTest, AutoLock) {
  KeyringService service(json_rpc_service(), GetPrefs());
  absl::optional<std::string> mnemonic = CreateWallet(&service, "brave");
  ASSERT_TRUE(mnemonic.has_value());
  ASSERT_FALSE(service.IsLocked());

  // Should not be locked yet after 4 minutes
  task_environment_.FastForwardBy(base::Minutes(4));
  ASSERT_FALSE(service.IsLocked());

  // After the 5th minute, it should be locked
  task_environment_.FastForwardBy(base::Minutes(1));
  ASSERT_TRUE(service.IsLocked());
  // Locking after it is auto locked won't cause a crash
  service.Lock();
  ASSERT_TRUE(service.IsLocked());

  // Unlocking will reset the timer
  EXPECT_TRUE(Unlock(&service, "brave"));
  ASSERT_FALSE(service.IsLocked());
  task_environment_.FastForwardBy(base::Minutes(5));
  ASSERT_TRUE(service.IsLocked());

  // Locking before the timer fires won't cause any problems after the
  // timer fires.
  EXPECT_TRUE(Unlock(&service, "brave"));
  ASSERT_FALSE(service.IsLocked());
  task_environment_.FastForwardBy(base::Minutes(1));
  service.Lock();
  ASSERT_TRUE(service.IsLocked());
  task_environment_.FastForwardBy(base::Minutes(4));
  ASSERT_TRUE(service.IsLocked());

  // Restoring keyring will auto lock too
  service.Reset();
  ASSERT_TRUE(RestoreWallet(&service, *mnemonic, "brave", false));
  ASSERT_FALSE(service.IsLocked());
  task_environment_.FastForwardBy(base::Minutes(6));
  ASSERT_TRUE(service.IsLocked());

  // Changing the auto lock pref should reset the timer
  EXPECT_TRUE(Unlock(&service, "brave"));
  ASSERT_FALSE(service.IsLocked());
  task_environment_.FastForwardBy(base::Minutes(4));
  GetPrefs()->SetInteger(kBraveWalletAutoLockMinutes, 3);
  task_environment_.FastForwardBy(base::Minutes(2));
  EXPECT_FALSE(service.IsLocked());
  task_environment_.FastForwardBy(base::Minutes(1));
  EXPECT_TRUE(service.IsLocked());

  // Changing the auto lock pref should reset the timer even if higher
  // for simplicity of logic
  EXPECT_TRUE(Unlock(&service, "brave"));
  ASSERT_FALSE(service.IsLocked());
  task_environment_.FastForwardBy(base::Minutes(2));
  EXPECT_FALSE(service.IsLocked());
  GetPrefs()->SetInteger(kBraveWalletAutoLockMinutes, 10);
  task_environment_.FastForwardBy(base::Minutes(9));
  EXPECT_FALSE(service.IsLocked());
  task_environment_.FastForwardBy(base::Minutes(1));
  EXPECT_TRUE(service.IsLocked());
}

TEST_F(KeyringServiceUnitTest, NotifyUserInteraction) {
  KeyringService service(json_rpc_service(), GetPrefs());
  ASSERT_TRUE(CreateWallet(&service, "brave"));
  ASSERT_FALSE(service.IsLocked());

  // Notifying of user interaction should keep the wallet unlocked
  task_environment_.FastForwardBy(base::Minutes(4));
  service.NotifyUserInteraction();
  task_environment_.FastForwardBy(base::Minutes(1));
  service.NotifyUserInteraction();
  task_environment_.FastForwardBy(base::Minutes(4));
  ASSERT_FALSE(service.IsLocked());
  task_environment_.FastForwardBy(base::Minutes(1));
  ASSERT_TRUE(service.IsLocked());
}

TEST_F(KeyringServiceUnitTest, SetSelectedAccount) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeatures(
      {brave_wallet::features::kBraveWalletFilecoinFeature,
       brave_wallet::features::kBraveWalletSolanaFeature},
      {});

  KeyringService service(json_rpc_service(), GetPrefs());

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
  EXPECT_EQ(absl::nullopt, GetSelectedAccount(&service, mojom::CoinType::ETH));

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
      &service, "0xDc06aE500aD5ebc5972A0D8Ada4733006E905976",
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
      mojom::CoinType::ETH));
  service.AddHardwareAccounts(std::move(new_accounts));
  EXPECT_TRUE(SetSelectedAccount(&service, &observer, hardware_account,
                                 mojom::CoinType::ETH));
  observer.Reset();

  EXPECT_TRUE(Unlock(&service, "brave"));
  // Can set Filecoin account
  {
    absl::optional<std::string> imported_account = ImportFilecoinAccount(
        &service, "Imported Filecoin account 1",
        // t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q
        "7b2254797065223a22736563703235366b31222c22507269766174654b6579223a2257"
        "6b4545645a45794235364b5168512b453338786a7663464c2b545a4842464e732b696a"
        "58533535794b383d227d",
        mojom::kFilecoinTestnet);
    ASSERT_TRUE(imported_account.has_value());
    EXPECT_EQ(*imported_account, "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q");
    EXPECT_TRUE(SetSelectedAccount(
        &service, &observer, imported_account.value(), mojom::CoinType::FIL));
    EXPECT_EQ(imported_account.value(),
              GetSelectedAccount(&service, mojom::CoinType::FIL));
  }
  // Can set Solana account
  {
    // lazily create keyring when importing SOL account
    absl::optional<std::string> imported_account = ImportAccount(
        &service, "Imported Account 1",
        // C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ
        "sCzwsBKmKtk5Hgb4YUJAduQ5nmJq4GTyzCXhrKonAGaexa83MgSZuTSMS6TSZTndnC"
        "YbQtaJQKLXET9jVjepWXe",
        mojom::CoinType::SOL);
    ASSERT_TRUE(imported_account.has_value());
    EXPECT_EQ(*imported_account,
              "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ");
    EXPECT_TRUE(SetSelectedAccount(
        &service, &observer, imported_account.value(), mojom::CoinType::SOL));
    EXPECT_EQ(imported_account.value(),
              GetSelectedAccount(&service, mojom::CoinType::SOL));
  }
  EXPECT_EQ(hardware_account,
            GetSelectedAccount(&service, mojom::CoinType::ETH));
  EXPECT_EQ("t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
            GetSelectedAccount(&service, mojom::CoinType::FIL));
  EXPECT_EQ("C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ",
            GetSelectedAccount(&service, mojom::CoinType::SOL));

  RemoveImportedAccount(&service, "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                        mojom::CoinType::FIL);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.SelectedAccountChangedFired(mojom::CoinType::FIL));
  observer.Reset();

  service.RemoveHardwareAccount(hardware_account, mojom::CoinType::ETH);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.SelectedAccountChangedFired(mojom::CoinType::ETH));
  observer.Reset();
  RemoveImportedAccount(&service,
                        "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ",
                        mojom::CoinType::SOL);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.SelectedAccountChangedFired(mojom::CoinType::SOL));
  observer.Reset();
}

TEST_F(KeyringServiceUnitTest, AddAccountsWithDefaultName) {
  KeyringService service(json_rpc_service(), GetPrefs());
  ASSERT_TRUE(CreateWallet(&service, "brave"));
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(service.IsLocked());

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
  KeyringService service(json_rpc_service(), GetPrefs());
  ASSERT_TRUE(RestoreWallet(&service, kMnemonic1, "brave", false));
  ASSERT_FALSE(service.IsLocked());

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
  KeyringService service(json_rpc_service(), GetPrefs());
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

TEST_F(KeyringServiceUnitTest, SetDefaultKeyringHardwareAccountName) {
  KeyringService service(json_rpc_service(), GetPrefs());

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
        it.coin));
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
  KeyringService service(json_rpc_service(), GetPrefs());
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
  KeyringService service(json_rpc_service(), GetPrefs());
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
  KeyringService service(json_rpc_service(), GetPrefs());
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
  KeyringService service(json_rpc_service(), GetPrefs());
  {
    ASSERT_TRUE(CreateWallet(&service, "brave"));

    ASSERT_FALSE(AddAccount(&service, "FIL account1", mojom::CoinType::FIL));
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
  ASSERT_TRUE(AddAccount(&service, "FIL account1", mojom::CoinType::FIL));
  EXPECT_TRUE(service.IsKeyringCreated(mojom::kFilecoinKeyringId));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.IsKeyringCreated(mojom::kFilecoinKeyringId));
  EXPECT_FALSE(observer.IsKeyringRestored(mojom::kFilecoinKeyringId));
  observer.Reset();

  // Lock and unlock won't fired created event again
  service.Lock();
  EXPECT_TRUE(Unlock(&service, "brave"));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(service.IsKeyringCreated(mojom::kFilecoinKeyringId));
  EXPECT_FALSE(observer.IsKeyringCreated(mojom::kFilecoinKeyringId));
  observer.Reset();

  // FIL keyring already exists
  ASSERT_TRUE(AddAccount(&service, "FIL account2", mojom::CoinType::FIL));
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
        run_loop.Quit();
      }));
  run_loop.Run();
}

TEST_F(KeyringServiceUnitTest, ImportFilecoinAccounts) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      brave_wallet::features::kBraveWalletFilecoinFeature);

  KeyringService service(json_rpc_service(), GetPrefs());

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  ASSERT_TRUE(CreateWallet(&service, "brave"));

  ASSERT_FALSE(observer.IsKeyringCreated(mojom::kFilecoinKeyringId));
  EXPECT_FALSE(service.IsKeyringCreated(mojom::kFilecoinKeyringId));
  const struct {
    const char* name;
    const char* import_payload;
    const char* address;
    const char* private_key;
    // const char* public_key;
  } imported_accounts[] = {{"Imported Filecoin account 1",
                            "7b2254797065223a22736563703235366b31222c2250726976"
                            "6174654b6579223a22576b4"
                            "545645a45794235364b5168512b453338786a7663464c2b545"
                            "a4842464e732b696a585335"
                            "35794b383d227d",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                            "WkEEdZEyB56KQhQ+E38xjvcFL+TZHBFNs+ijXS55yK8="},
                           {"Imported Filecoin account 2",
                            "7b2254797065223a22736563703235366b31222c2250726976"
                            "6174654b6579223a22774d5"
                            "267766730734d6a764657356e32515472705a5658414c596a7"
                            "44d7036725156714d52535a"
                            "6a482f513d227d",
                            "t1par4kjqybnejlyuvpa3rodmluidq34ba6muafda",
                            "wMRgvg0sMjvFW5n2QTrpZVXALYjtMp6rQVqMRSZjH/Q="},
                           {"Imported Filecoin account 3",
                            "7b2254797065223a22736563703235366b31222c2250726976"
                            "6174654b6579223a22774e5"
                            "3667774514d2f466b665334423334496a475750343553546b2"
                            "f737434304c724379433955"
                            "6a7761773d227d",
                            "t1zvggbhs5sxyeifzcrmik5oljbley7lvo57ovusy",
                            "wNSfwtQM/FkfS4B34IjGWP45STk/st40LrCyC9Ujwaw="},
                           {"Imported Filecoin account 4",
                            "7b2254797065223a22626c73222c22507269766174654b6579"
                            "223a2270536e7752332f385"
                            "5616b53516f777858742b345a75393257586d424d526e74716"
                            "d6448696136724853453d22"
                            "7d",
                            "t3wwtato54ee5aod7j5uv2n75jpyn4hpwx3f2kx5cijtoxgyti"
                            "ul2dczrak3ghlbt5zjnj574"
                            "y3snhcb5bthva",
                            "pSnwR3/8UakSQowxXt+4Zu92WXmBMRntqmdHia6rHSE="}};
  auto amount = sizeof(imported_accounts) / sizeof(imported_accounts[0]);
  for (size_t i = 0; i < amount; ++i) {
    absl::optional<std::string> address = ImportFilecoinAccount(
        &service, imported_accounts[i].name,
        imported_accounts[i].import_payload, mojom::kFilecoinTestnet);
    ASSERT_TRUE(address.has_value());
    EXPECT_EQ(address, imported_accounts[i].address);

    base::RunLoop().RunUntilIdle();
    if (i == 0) {
      EXPECT_TRUE(observer.IsKeyringCreated(mojom::kFilecoinKeyringId));
      EXPECT_TRUE(service.IsKeyringCreated(mojom::kFilecoinKeyringId));
      observer.Reset();
    } else {
      EXPECT_FALSE(observer.IsKeyringCreated(mojom::kFilecoinKeyringId));
      EXPECT_TRUE(service.IsKeyringCreated(mojom::kFilecoinKeyringId));
    }

    std::string private_key;
    EXPECT_TRUE(
        GetPrivateKeyForImportedAccount(&service, imported_accounts[i].address,
                                        mojom::CoinType::FIL, &private_key));
    EXPECT_EQ(imported_accounts[i].private_key, private_key);
  }
  // filecoin keyring will be lazily created in first FIL import
  auto* filecoin_keyring =
      service.GetHDKeyringById(brave_wallet::mojom::kFilecoinKeyringId);
  EXPECT_EQ(filecoin_keyring->GetImportedAccountsNumber(), amount);

  EXPECT_TRUE(RemoveImportedAccount(&service, imported_accounts[1].address,
                                    mojom::CoinType::FIL));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());

  observer.Reset();
  EXPECT_EQ(filecoin_keyring->GetImportedAccountsNumber(), amount - 1);
  // remove invalid address
  EXPECT_FALSE(
      RemoveImportedAccount(&service, "0xxxxxxxxxx0", mojom::CoinType::FIL));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.AccountsChangedFired());

  bool callback_called = false;
  service.GetKeyringInfo(
      brave_wallet::mojom::kFilecoinKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_EQ(keyring_info->id, mojom::kFilecoinKeyringId);
        EXPECT_TRUE(keyring_info->is_keyring_created);
        EXPECT_FALSE(keyring_info->is_locked);
        EXPECT_FALSE(keyring_info->is_backed_up);
        EXPECT_EQ(keyring_info->account_infos.size(), amount - 1);
        EXPECT_EQ(keyring_info->account_infos[0]->address,
                  imported_accounts[0].address);
        EXPECT_EQ(keyring_info->account_infos[0]->name,
                  imported_accounts[0].name);
        EXPECT_TRUE(keyring_info->account_infos[0]->is_imported);
        EXPECT_EQ(keyring_info->account_infos[1]->address,
                  imported_accounts[2].address);
        EXPECT_EQ(keyring_info->account_infos[1]->name,
                  imported_accounts[2].name);
        EXPECT_TRUE(keyring_info->account_infos[1]->is_imported);
        EXPECT_EQ(keyring_info->account_infos[2]->address,
                  imported_accounts[3].address);
        EXPECT_EQ(keyring_info->account_infos[2]->name,
                  imported_accounts[3].name);
        EXPECT_TRUE(keyring_info->account_infos[2]->is_imported);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(filecoin_keyring->GetImportedAccountsNumber(), amount - 1);
  service.Lock();
  // cannot get private key when locked
  std::string private_key;
  EXPECT_FALSE(
      GetPrivateKeyForImportedAccount(&service, imported_accounts[0].address,
                                      mojom::CoinType::FIL, &private_key));
  EXPECT_TRUE(private_key.empty());

  EXPECT_TRUE(Unlock(&service, "brave"));

  callback_called = false;
  // Imported accounts should be restored
  service.GetKeyringInfo(
      brave_wallet::mojom::kFilecoinKeyringId,
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_EQ(keyring_info->account_infos.size(), amount - 1);
        EXPECT_EQ(keyring_info->account_infos[0]->address,
                  imported_accounts[0].address);
        EXPECT_EQ(keyring_info->account_infos[0]->name,
                  imported_accounts[0].name);
        EXPECT_TRUE(keyring_info->account_infos[0]->is_imported);
        EXPECT_EQ(keyring_info->account_infos[1]->address,
                  imported_accounts[2].address);
        EXPECT_EQ(keyring_info->account_infos[1]->name,
                  imported_accounts[2].name);
        EXPECT_TRUE(keyring_info->account_infos[1]->is_imported);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(service.GetHDKeyringById(brave_wallet::mojom::kFilecoinKeyringId)
                ->GetImportedAccountsNumber(),
            amount - 1);
  // private key should also be available now
  private_key.clear();
  EXPECT_TRUE(
      GetPrivateKeyForImportedAccount(&service, imported_accounts[0].address,
                                      mojom::CoinType::FIL, &private_key));
  EXPECT_EQ(imported_accounts[0].private_key, private_key);

  auto* default_keyring =
      service.GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId);
  // Imported accounts should also be restored in filecoin keyring
  EXPECT_EQ(default_keyring->GetImportedAccountsNumber(), 0u);
  EXPECT_EQ(service.GetHDKeyringById(brave_wallet::mojom::kFilecoinKeyringId)
                ->GetImportedAccountsNumber(),
            amount - 1);
}

TEST_F(KeyringServiceUnitTest, PreCreateEncryptors) {
  {
    // Create default wallet with disabled filecoin feature.
    // Solana feature is enabled on desktop and disabled on Android.
    KeyringService service(json_rpc_service(), GetPrefs());
    ASSERT_TRUE(CreateWallet(&service, "brave"));
    EXPECT_NE(service.encryptors_.at(mojom::kDefaultKeyringId), nullptr);
    EXPECT_FALSE(service.encryptors_.contains(mojom::kFilecoinKeyringId));
#if BUILDFLAG(IS_ANDROID)
    EXPECT_FALSE(service.encryptors_.contains(mojom::kSolanaKeyringId));
#else
    EXPECT_NE(service.encryptors_.at(mojom::kSolanaKeyringId), nullptr);
#endif
  }
  {
    // Create wallet with enabled filecoin & solana
    base::test::ScopedFeatureList feature_list;
    feature_list.InitWithFeatures(
        {brave_wallet::features::kBraveWalletFilecoinFeature,
         brave_wallet::features::kBraveWalletSolanaFeature},
        {});

    KeyringService service(json_rpc_service(), GetPrefs());
    ASSERT_TRUE(CreateWallet(&service, "brave"));
    EXPECT_NE(service.encryptors_.at(mojom::kDefaultKeyringId), nullptr);
    EXPECT_NE(service.encryptors_.at(mojom::kFilecoinKeyringId), nullptr);
    EXPECT_NE(service.encryptors_.at(mojom::kSolanaKeyringId), nullptr);
  }
  {
    // Create wallet and enable filecoin & solana before unlock
    KeyringService service(json_rpc_service(), GetPrefs());
    ASSERT_TRUE(CreateWallet(&service, "brave"));
    EXPECT_NE(service.encryptors_.at(mojom::kDefaultKeyringId), nullptr);
    EXPECT_FALSE(service.encryptors_.contains(mojom::kFilecoinKeyringId));
#if BUILDFLAG(IS_ANDROID)
    EXPECT_FALSE(service.encryptors_.contains(mojom::kSolanaKeyringId));
#else
    EXPECT_NE(service.encryptors_.at(mojom::kSolanaKeyringId), nullptr);
#endif
    service.Lock();
    base::test::ScopedFeatureList feature_list;
    feature_list.InitWithFeatures(
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
    KeyringService service(json_rpc_service(), GetPrefs());
    TestKeyringServiceObserver observer;
    service.AddObserver(observer.GetReceiver());
    absl::optional<std::string> mnemonic_to_be_restored =
        CreateWallet(&service, "brave");
    ASSERT_TRUE(mnemonic_to_be_restored.has_value());

    service.Reset();
    ASSERT_TRUE(
        RestoreWallet(&service, *mnemonic_to_be_restored, "brave", false));
    EXPECT_NE(service.encryptors_.at(mojom::kDefaultKeyringId), nullptr);
    EXPECT_FALSE(service.encryptors_.contains(mojom::kFilecoinKeyringId));
#if BUILDFLAG(IS_ANDROID)
    EXPECT_FALSE(service.encryptors_.contains(mojom::kSolanaKeyringId));
#else
    EXPECT_NE(service.encryptors_.at(mojom::kSolanaKeyringId), nullptr);
#endif

    base::test::ScopedFeatureList feature_list;
    feature_list.InitWithFeatures(
        {brave_wallet::features::kBraveWalletFilecoinFeature,
         brave_wallet::features::kBraveWalletSolanaFeature},
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
  feature_list.InitAndEnableFeature(
      brave_wallet::features::kBraveWalletSolanaFeature);
  {
    KeyringService service(json_rpc_service(), GetPrefs());
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
    KeyringService service(json_rpc_service(), GetPrefs());
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
    KeyringService service(json_rpc_service(), GetPrefs());
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
    std::string private_key;
    EXPECT_TRUE(GetPrivateKeyForImportedAccount(
        &service, "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ",
        mojom::CoinType::SOL, &private_key));
    EXPECT_EQ(private_key,
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
                                      mojom::CoinType::SOL));

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
  feature_list.InitAndEnableFeature(
      brave_wallet::features::kBraveWalletSolanaFeature);
  KeyringService service(json_rpc_service(), GetPrefs());
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

    KeyringService service(json_rpc_service(), GetPrefs());
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
    if (*request_value->FindStringKey("method") == "eth_getTransactionCount") {
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
  KeyringService service(json_rpc_service(), GetPrefs());

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  std::vector<std::string> requested_addresses;
  set_transaction_count_callback(base::BindLambdaForTesting(
      [this, &requested_addresses](const std::string& address) -> std::string {
        requested_addresses.push_back(address);

        // 3rd and 10th have transactions.
        if (address == saved_addresses()[3] || address == saved_addresses()[10])
          return R"({"jsonrpc":"2.0","id":"1","result":"0x1"})";
        else
          return R"({"jsonrpc":"2.0","id":"1","result":"0x0"})";
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
  KeyringService service(json_rpc_service(), GetPrefs());

  TestKeyringServiceObserver observer;
  service.AddObserver(observer.GetReceiver());

  std::vector<std::string> requested_addresses;
  set_transaction_count_callback(base::BindLambdaForTesting(
      [this, &requested_addresses](const std::string& address) -> std::string {
        requested_addresses.push_back(address);

        // 3rd account has transactions. Checking 8th account ends with network
        // error.
        if (address == saved_addresses()[3])
          return R"({"jsonrpc":"2.0","id":"1","result":"0x1"})";
        else if (address == saved_addresses()[8])
          return "error";
        else
          return R"({"jsonrpc":"2.0","id":"1","result":"0x0"})";
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
  KeyringService service(json_rpc_service(), GetPrefs());

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
        if (address == saved_addresses()[5] || address == saved_addresses()[6])
          return R"({"jsonrpc":"2.0","id":"1","result":"0x1"})";
        else
          return R"({"jsonrpc":"2.0","id":"1","result":"0x0"})";
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
  KeyringService service(json_rpc_service(), GetPrefs());

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
        if (address == saved_addresses()[3] || address == saved_addresses()[10])
          return R"({"jsonrpc":"2.0","id":"1","result":"0x1"})";
        else
          return R"({"jsonrpc":"2.0","id":"1","result":"0x0"})";
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

TEST_F(KeyringServiceUnitTest, AccountMetasForFilecoinKeyring) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeatures(
      {brave_wallet::features::kBraveWalletFilecoinFeature}, {});

  KeyringService service(json_rpc_service(), GetPrefs());
  SetNetwork(brave_wallet::mojom::kFilecoinTestnet, mojom::CoinType::FIL);
  EXPECT_TRUE(
      service.CreateEncryptorForKeyring("brave", mojom::kFilecoinKeyringId));
  ASSERT_TRUE(service.CreateKeyringInternal(
      brave_wallet::mojom::kFilecoinKeyringId, kMnemonic1, false));
  auto* keyring =
      service.GetHDKeyringById(brave_wallet::mojom::kFilecoinKeyringId);
  keyring->AddAccounts(2);

  EXPECT_EQ(GetCurrentChainId(GetPrefs(), mojom::CoinType::FIL),
            brave_wallet::mojom::kFilecoinTestnet);
  const std::string address1 = keyring->GetAddress(0);
  const std::string name1 = "Filecoin Account 1";
  const std::string account_path1 = KeyringService::GetAccountPathByIndex(
      0, brave_wallet::mojom::kFilecoinKeyringId);
  const std::string address2 = keyring->GetAddress(1);
  const std::string name2 = "Filecoin Account 2";
  const std::string account_path2 = KeyringService::GetAccountPathByIndex(
      1, brave_wallet::mojom::kFilecoinKeyringId);

  KeyringService::SetAccountMetaForKeyring(GetPrefs(), account_path1, name1,
                                           address1, mojom::kFilecoinKeyringId);
  KeyringService::SetAccountMetaForKeyring(GetPrefs(), account_path2, name2,
                                           address2, mojom::kFilecoinKeyringId);

  const base::Value* account_metas = KeyringService::GetPrefForKeyring(
      GetPrefs(), kAccountMetas, mojom::kFilecoinKeyringId);
  ASSERT_NE(account_metas, nullptr);
  std::string prefix = GetCurrentFilecoinNetworkPrefix(GetPrefs());
  const base::Value* account_metas_for_network = account_metas->FindKey(prefix);
  EXPECT_TRUE(account_metas_for_network);

  EXPECT_EQ(account_metas_for_network->FindPath(account_path1 + ".account_name")
                ->GetString(),
            name1);
  EXPECT_EQ(account_metas_for_network->FindPath(account_path2 + ".account_name")
                ->GetString(),
            name2);
  EXPECT_EQ(KeyringService::GetAccountNameForKeyring(GetPrefs(), account_path1,
                                                     mojom::kFilecoinKeyringId),
            name1);
  EXPECT_EQ(KeyringService::GetAccountAddressForKeyring(
                GetPrefs(), account_path1, mojom::kFilecoinKeyringId),
            address1);
  EXPECT_EQ(KeyringService::GetAccountNameForKeyring(GetPrefs(), account_path2,
                                                     mojom::kFilecoinKeyringId),
            name2);
  EXPECT_EQ(KeyringService::GetAccountAddressForKeyring(
                GetPrefs(), account_path2, mojom::kFilecoinKeyringId),
            address2);
  EXPECT_EQ(service.GetAccountMetasNumberForKeyring(mojom::kFilecoinKeyringId),
            2u);
  EXPECT_EQ(service.GetAccountMetasNumberForKeyring("keyring1"), 0u);

  // GetAccountInfosForKeyring should work even if the keyring is locked
  service.Lock();
  std::vector<mojom::AccountInfoPtr> account_infos =
      service.GetAccountInfosForKeyring(mojom::kFilecoinKeyringId);
  EXPECT_EQ(account_infos.size(), 2u);
  EXPECT_EQ(account_infos[0]->address, address1);
  EXPECT_EQ(account_infos[0]->name, name1);
  EXPECT_EQ(account_infos[1]->address, address2);
  EXPECT_EQ(account_infos[1]->name, name2);
}

TEST_F(KeyringServiceUnitTest, SwitchAccountsOnNetworkChange) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeatures(
      {brave_wallet::features::kBraveWalletFilecoinFeature}, {});

  KeyringService service(json_rpc_service(), GetPrefs());
  EXPECT_TRUE(
      service.CreateEncryptorForKeyring("brave", mojom::kFilecoinKeyringId));

  ASSERT_TRUE(service.CreateKeyringInternal(
      brave_wallet::mojom::kFilecoinKeyringId, kMnemonic1, false));

  auto* keyring =
      service.GetHDKeyringById(brave_wallet::mojom::kFilecoinKeyringId);

  SetNetwork(brave_wallet::mojom::kFilecoinMainnet, mojom::CoinType::FIL);
  base::RunLoop().RunUntilIdle();
  service.AddAccountForKeyring(mojom::kFilecoinKeyringId, "");
  service.AddAccountForKeyring(mojom::kFilecoinKeyringId, "");
  EXPECT_EQ(keyring->GetAccountsNumber(), 2u);
  const std::string f_address1 = keyring->GetAddress(0);
  const std::string f_address2 = keyring->GetAddress(1);
  {
    std::vector<mojom::AccountInfoPtr> account_infos =
        service.GetAccountInfosForKeyring(mojom::kFilecoinKeyringId);
    EXPECT_EQ(account_infos.size(), 2u);
    EXPECT_EQ(account_infos[0]->address, f_address1);
    EXPECT_EQ(account_infos[1]->address, f_address2);
    EXPECT_EQ(FilAddress::FromAddress(f_address1).network(),
              brave_wallet::mojom::kFilecoinMainnet);
    EXPECT_EQ(FilAddress::FromAddress(f_address2).network(),
              brave_wallet::mojom::kFilecoinMainnet);
  }

  SetNetwork(brave_wallet::mojom::kFilecoinTestnet, mojom::CoinType::FIL);
  base::RunLoop().RunUntilIdle();
  service.AddAccountForKeyring(mojom::kFilecoinKeyringId, "");
  service.AddAccountForKeyring(mojom::kFilecoinKeyringId, "");

  EXPECT_EQ(keyring->GetAccountsNumber(), 2u);
  const std::string t_address1 = keyring->GetAddress(0);
  const std::string t_address2 = keyring->GetAddress(1);
  {
    std::vector<mojom::AccountInfoPtr> account_infos =
        service.GetAccountInfosForKeyring(mojom::kFilecoinKeyringId);
    EXPECT_EQ(account_infos.size(), 2u);
    EXPECT_EQ(account_infos[0]->address, t_address1);
    EXPECT_EQ(account_infos[1]->address, t_address2);
    EXPECT_EQ(FilAddress::FromAddress(t_address1).network(),
              brave_wallet::mojom::kFilecoinTestnet);
    EXPECT_EQ(FilAddress::FromAddress(t_address2).network(),
              brave_wallet::mojom::kFilecoinTestnet);
  }
  SetNetwork(brave_wallet::mojom::kLocalhostChainId, mojom::CoinType::FIL);
  base::RunLoop().RunUntilIdle();
  service.AddAccountForKeyring(mojom::kFilecoinKeyringId, "");
  service.AddAccountForKeyring(mojom::kFilecoinKeyringId, "");
  EXPECT_EQ(keyring->GetAccountsNumber(), 4u);
  const std::string t_address3 = keyring->GetAddress(2);
  const std::string t_address4 = keyring->GetAddress(3);
  {
    std::vector<mojom::AccountInfoPtr> account_infos =
        service.GetAccountInfosForKeyring(mojom::kFilecoinKeyringId);
    EXPECT_EQ(account_infos.size(), 4u);
    EXPECT_EQ(account_infos[0]->address, t_address1);
    EXPECT_EQ(account_infos[1]->address, t_address2);
    EXPECT_EQ(account_infos[2]->address, t_address3);
    EXPECT_EQ(account_infos[3]->address, t_address4);

    EXPECT_EQ(FilAddress::FromAddress(t_address3).network(),
              brave_wallet::mojom::kFilecoinTestnet);
    EXPECT_EQ(FilAddress::FromAddress(t_address4).network(),
              brave_wallet::mojom::kFilecoinTestnet);
  }
}

}  // namespace brave_wallet
