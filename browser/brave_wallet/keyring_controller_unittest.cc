/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/keyring_controller.h"

#include <utility>

#include "base/base64.h"
#include "base/callback_helpers.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {
const char kPasswordEncryptorSalt[] = "password_encryptor_salt";
const char kPasswordEncryptorNonce[] = "password_encryptor_nonce";
const char kEncryptedMnemonic[] = "encrypted_mnemonic";
const char kBackupComplete[] = "backup_complete";
const char kAccountMetas[] = "account_metas";
const char kImportedAccounts[] = "imported_accounts";
const char kAccountAddress[] = "account_address";
const char kEncryptedPrivateKey[] = "encrypted_private_key";

const char kMnemonic1[] =
    "divide cruise upon flag harsh carbon filter merit once advice bright "
    "drive";
const char kMnemonic2[] =
    "misery jeans response tiny nominee civil zoo strong correct taxi chimney "
    "goat";
}  // namespace

class TestKeyringControllerObserver
    : public brave_wallet::mojom::KeyringControllerObserver {
 public:
  TestKeyringControllerObserver() {}

  void AutoLockMinutesChanged() override {
    autoLockMinutesChangedFired_ = true;
  }

  // TODO(bbondy): We should be testing all of these observer events
  void KeyringCreated() override {}
  void KeyringRestored() override {}
  void Locked() override {}
  void Unlocked() override {}
  void BackedUp() override {}

  void SelectedAccountChanged() override {
    selectedAccountChangedFired_ = true;
  }

  void AccountsChanged() override { accountsChangedFired_ = true; }

  bool AutoLockMinutesChangedFired() { return autoLockMinutesChangedFired_; }
  bool SelectedAccountChangedFired() { return selectedAccountChangedFired_; }
  bool AccountsChangedFired() { return accountsChangedFired_; }

  mojo::PendingRemote<brave_wallet::mojom::KeyringControllerObserver>
  GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }

  void Reset() {
    autoLockMinutesChangedFired_ = false;
    selectedAccountChangedFired_ = false;
    accountsChangedFired_ = false;
  }

 private:
  bool autoLockMinutesChangedFired_ = false;
  bool selectedAccountChangedFired_ = false;
  bool accountsChangedFired_ = false;
  mojo::Receiver<brave_wallet::mojom::KeyringControllerObserver>
      observer_receiver_{this};
};

class KeyringControllerUnitTest : public testing::Test {
 public:
  KeyringControllerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~KeyringControllerUnitTest() override {}

  void GetBooleanCallback(bool value) { bool_value_ = value; }
  void GetStringCallback(const std::string& value) { string_value_ = value; }

 protected:
  void SetUp() override {
    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));
    profile_ = builder.Build();
  }

  PrefService* GetPrefs() { return profile_->GetPrefs(); }

  bool HasPrefForKeyring(const std::string& key, const std::string& id) {
    return KeyringController::HasPrefForKeyring(GetPrefs(), key, id);
  }

  std::string GetStringPrefForKeyring(const std::string& key,
                                      const std::string& id) {
    const base::Value* value =
        KeyringController::GetPrefForKeyring(GetPrefs(), key, id);
    if (!value)
      return std::string();

    return value->GetString();
  }

  static absl::optional<std::string> GetSelectedAccount(
      KeyringController* controller) {
    absl::optional<std::string> account;
    base::RunLoop run_loop;
    controller->GetSelectedAccount(
        base::BindLambdaForTesting([&](const absl::optional<std::string>& v) {
          account = v;
          run_loop.Quit();
        }));
    run_loop.Run();
    return account;
  }

  static bool SetSelectedAccount(KeyringController* controller,
                                 TestKeyringControllerObserver* observer,
                                 const std::string& account) {
    EXPECT_FALSE(observer->SelectedAccountChangedFired());
    bool success = false;
    base::RunLoop run_loop;
    controller->SetSelectedAccount(account,
                                   base::BindLambdaForTesting([&](bool v) {
                                     success = v;
                                     run_loop.Quit();
                                   }));
    run_loop.Run();
    base::RunLoop().RunUntilIdle();
    if (success) {
      EXPECT_TRUE(observer->SelectedAccountChangedFired());
      observer->Reset();
    }
    EXPECT_FALSE(observer->SelectedAccountChangedFired());
    return success;
  }

  static bool RemoveImportedAccount(KeyringController* controller,
                                    const std::string& address) {
    bool success;
    base::RunLoop run_loop;
    controller->RemoveImportedAccount(
        "0xDc06aE500aD5ebc5972A0D8Ada4733006E905976",
        base::BindLambdaForTesting([&](bool v) {
          success = v;
          run_loop.Quit();
        }));
    run_loop.Run();
    return success;
  }

  static absl::optional<std::string> ImportAccount(
      KeyringController* controller,
      const std::string& name,
      const std::string& private_key) {
    absl::optional<std::string> account;
    base::RunLoop run_loop;
    controller->ImportAccount(
        name, private_key,
        base::BindLambdaForTesting(
            [&](bool success, const std::string& address) {
              if (success) {
                account = address;
              }
            }));
    return account;
  }

  static absl::optional<std::string> CreateWallet(KeyringController* controller,
                                                  const std::string& password) {
    std::string mnemonic;
    base::RunLoop run_loop;
    controller->CreateWallet(
        password, base::BindLambdaForTesting([&](const std::string& v) {
          mnemonic = v;
          run_loop.Quit();
        }));
    run_loop.Run();
    return mnemonic;
  }

  static void AddHardwareAccount(
      KeyringController* controller,
      std::vector<mojom::HardwareWalletAccountPtr> new_accounts) {
    controller->AddHardwareAccounts(std::move(new_accounts));
  }

  static bool Unlock(KeyringController* controller,
                     const std::string& password) {
    bool success = false;
    base::RunLoop run_loop;
    controller->Unlock(password, base::BindLambdaForTesting([&](bool v) {
                         success = v;
                         run_loop.Quit();
                       }));
    run_loop.Run();
    return success;
  }

  static int32_t GetAutoLockMinutes(KeyringController* controller) {
    int32_t minutes;
    base::RunLoop run_loop;
    controller->GetAutoLockMinutes(base::BindLambdaForTesting([&](int32_t v) {
      minutes = v;
      run_loop.Quit();
    }));
    run_loop.Run();
    return minutes;
  }

  static bool SetAutoLockMinutes(KeyringController* controller,
                                 TestKeyringControllerObserver* observer,
                                 int32_t minutes) {
    bool success = false;
    base::RunLoop run_loop;
    int32_t old_minutes = GetAutoLockMinutes(controller);
    controller->SetAutoLockMinutes(minutes,
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

  static bool Lock(KeyringController* controller) {
    controller->Lock();
    return controller->IsLocked();
  }

  bool bool_value() { return bool_value_; }
  const std::string string_value() { return string_value_; }
  content::BrowserTaskEnvironment task_environment_;

 private:
  std::unique_ptr<TestingProfile> profile_;
  bool bool_value_;
  std::string string_value_;
};

TEST_F(KeyringControllerUnitTest, HasAndGetPrefForKeyring) {
  base::DictionaryValue dict;
  dict.SetPath("default.pref1", base::Value("123"));
  GetPrefs()->Set(kBraveWalletKeyrings, dict);
  EXPECT_TRUE(
      KeyringController::HasPrefForKeyring(GetPrefs(), "pref1", "default"));
  const base::Value* value =
      KeyringController::GetPrefForKeyring(GetPrefs(), "pref1", "default");
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->GetString(), "123");

  EXPECT_FALSE(
      KeyringController::HasPrefForKeyring(GetPrefs(), "pref1", "keyring2"));
  EXPECT_EQ(
      KeyringController::GetPrefForKeyring(GetPrefs(), "pref1", "keyring2"),
      nullptr);

  EXPECT_FALSE(
      KeyringController::HasPrefForKeyring(GetPrefs(), "pref2", "default"));
  EXPECT_EQ(
      KeyringController::GetPrefForKeyring(GetPrefs(), "pref2", "default"),
      nullptr);
}
TEST_F(KeyringControllerUnitTest, SetPrefForKeyring) {
  KeyringController::SetPrefForKeyring(GetPrefs(), "pref1", base::Value("123"),
                                       "default");
  const base::DictionaryValue* keyrings_pref =
      GetPrefs()->GetDictionary(kBraveWalletKeyrings);
  ASSERT_NE(keyrings_pref, nullptr);
  const base::Value* value = keyrings_pref->FindPath("default.pref1");
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->GetString(), "123");

  EXPECT_EQ(keyrings_pref->FindPath("default.pref2"), nullptr);
  EXPECT_EQ(keyrings_pref->FindPath("keyring2.pref1"), nullptr);
}

TEST_F(KeyringControllerUnitTest, GetPrefInBytesForKeyring) {
  KeyringController controller(GetPrefs());
  KeyringController::SetPrefForKeyring(GetPrefs(), kEncryptedMnemonic,
                                       base::Value("3q2+7w=="), "default");

  auto verify_bytes = [](const std::vector<uint8_t>& bytes) {
    ASSERT_EQ(bytes.size(), 4u);
    EXPECT_EQ(bytes[0], 0xde);
    EXPECT_EQ(bytes[1], 0xad);
    EXPECT_EQ(bytes[2], 0xbe);
    EXPECT_EQ(bytes[3], 0xef);
  };

  std::vector<uint8_t> mnemonic;
  ASSERT_TRUE(controller.GetPrefInBytesForKeyring(kEncryptedMnemonic, &mnemonic,
                                                  "default"));
  verify_bytes(mnemonic);

  std::vector<uint8_t> mnemonic_fixed(4);
  ASSERT_TRUE(controller.GetPrefInBytesForKeyring(kEncryptedMnemonic,
                                                  &mnemonic_fixed, "default"));
  verify_bytes(mnemonic_fixed);

  std::vector<uint8_t> mnemonic_smaller(2);
  ASSERT_TRUE(controller.GetPrefInBytesForKeyring(
      kEncryptedMnemonic, &mnemonic_smaller, "default"));
  verify_bytes(mnemonic_smaller);

  std::vector<uint8_t> mnemonic_bigger(8);
  ASSERT_TRUE(controller.GetPrefInBytesForKeyring(kEncryptedMnemonic,
                                                  &mnemonic_bigger, "default"));
  verify_bytes(mnemonic_bigger);

  // invalid base64 encoded
  mnemonic.clear();
  KeyringController::SetPrefForKeyring(GetPrefs(), kEncryptedMnemonic,
                                       base::Value("3q2+7w^^"), "default");
  EXPECT_FALSE(controller.GetPrefInBytesForKeyring(kEncryptedMnemonic,
                                                   &mnemonic, "default"));

  // default pref value (empty)
  mnemonic.clear();
  GetPrefs()->ClearPref(kBraveWalletKeyrings);
  EXPECT_FALSE(controller.GetPrefInBytesForKeyring(kEncryptedMnemonic,
                                                   &mnemonic, "default"));

  // bytes is nullptr
  EXPECT_FALSE(controller.GetPrefInBytesForKeyring(kEncryptedMnemonic, nullptr,
                                                   "default"));

  // non-existing pref
  mnemonic.clear();
  EXPECT_FALSE(controller.GetPrefInBytesForKeyring("brave.nothinghere",
                                                   &mnemonic, "default"));

  // non-string pref
  mnemonic.clear();
  KeyringController::SetPrefForKeyring(GetPrefs(), "test_num", base::Value(123),
                                       "default");
  EXPECT_FALSE(
      controller.GetPrefInBytesForKeyring("test_num", &mnemonic, "default"));
}

TEST_F(KeyringControllerUnitTest, SetPrefInBytesForKeyring) {
  const uint8_t bytes_array[] = {0xde, 0xad, 0xbe, 0xef};
  KeyringController controller(GetPrefs());
  controller.SetPrefInBytesForKeyring(kEncryptedMnemonic, bytes_array,
                                      "default");
  EXPECT_EQ(GetStringPrefForKeyring(kEncryptedMnemonic, "default"), "3q2+7w==");

  GetPrefs()->ClearPref(kBraveWalletKeyrings);
  const std::vector<uint8_t> bytes_vector = {0xde, 0xad, 0xbe, 0xef};
  controller.SetPrefInBytesForKeyring(kEncryptedMnemonic, bytes_vector,
                                      "default");
  EXPECT_EQ(GetStringPrefForKeyring(kEncryptedMnemonic, "default"), "3q2+7w==");
}

TEST_F(KeyringControllerUnitTest, GetOrCreateNonceForKeyring) {
  std::string encoded_nonce;
  std::string encoded_nonce2;
  {
    KeyringController controller(GetPrefs());
    const std::vector<uint8_t> nonce =
        controller.GetOrCreateNonceForKeyring("default");
    encoded_nonce = base::Base64Encode(nonce);
    const std::vector<uint8_t> nonce2 =
        controller.GetOrCreateNonceForKeyring("keyring2");
    encoded_nonce2 = base::Base64Encode(nonce2);
    EXPECT_EQ(encoded_nonce,
              GetStringPrefForKeyring(kPasswordEncryptorNonce, "default"));
    EXPECT_EQ(encoded_nonce2,
              GetStringPrefForKeyring(kPasswordEncryptorNonce, "keyring2"));
  }
  {  // It should be the same nonce as long as the pref exists
    KeyringController controller(GetPrefs());
    const std::vector<uint8_t> nonce =
        controller.GetOrCreateNonceForKeyring("default");
    EXPECT_EQ(base::Base64Encode(nonce), encoded_nonce);
    const std::vector<uint8_t> nonce2 =
        controller.GetOrCreateNonceForKeyring("keyring2");
    EXPECT_EQ(base::Base64Encode(nonce2), encoded_nonce2);
    EXPECT_EQ(encoded_nonce,
              GetStringPrefForKeyring(kPasswordEncryptorNonce, "default"));
    EXPECT_EQ(encoded_nonce2,
              GetStringPrefForKeyring(kPasswordEncryptorNonce, "keyring2"));
  }
  GetPrefs()->ClearPref(kBraveWalletKeyrings);
  {  // nonce should be different now
    KeyringController controller(GetPrefs());
    const std::vector<uint8_t> nonce =
        controller.GetOrCreateNonceForKeyring("default");
    EXPECT_NE(base::Base64Encode(nonce), encoded_nonce);
    const std::vector<uint8_t> nonce2 =
        controller.GetOrCreateNonceForKeyring("keyring2");
    EXPECT_NE(base::Base64Encode(nonce2), encoded_nonce2);
  }
}

TEST_F(KeyringControllerUnitTest, CreateEncryptorForKeyring) {
  std::string encoded_salt;
  std::string encoded_salt2;
  {
    KeyringController controller(GetPrefs());
    EXPECT_TRUE(controller.CreateEncryptorForKeyring("123", "default"));
    EXPECT_NE(controller.encryptor_, nullptr);
    EXPECT_TRUE(controller.CreateEncryptorForKeyring("456", "keyring2"));
    EXPECT_NE(controller.encryptor_, nullptr);
    encoded_salt = GetStringPrefForKeyring(kPasswordEncryptorSalt, "default");
    EXPECT_FALSE(encoded_salt.empty());
    encoded_salt2 = GetStringPrefForKeyring(kPasswordEncryptorSalt, "keyring2");
    EXPECT_FALSE(encoded_salt2.empty());
  }
  {
    KeyringController controller(GetPrefs());
    EXPECT_TRUE(controller.CreateEncryptorForKeyring("123", "default"));
    EXPECT_TRUE(controller.CreateEncryptorForKeyring("456", "keyring2"));
    EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorSalt, "default"),
              encoded_salt);
    EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorSalt, "keyring2"),
              encoded_salt2);
  }
  {
    KeyringController controller(GetPrefs());
    EXPECT_FALSE(controller.CreateEncryptorForKeyring("", "default"));
    EXPECT_EQ(controller.encryptor_, nullptr);
    EXPECT_FALSE(controller.CreateEncryptorForKeyring("", "keyring2"));
    EXPECT_EQ(controller.encryptor_, nullptr);
  }
}

TEST_F(KeyringControllerUnitTest, CreateDefaultKeyringInternal) {
  KeyringController controller(GetPrefs());

  TestKeyringControllerObserver observer;
  controller.AddObserver(observer.GetReceiver());

  // encryptor is nullptr
  ASSERT_FALSE(controller.CreateDefaultKeyringInternal(kMnemonic1, false));

  EXPECT_TRUE(controller.CreateEncryptorForKeyring("brave", "default"));
  ASSERT_TRUE(controller.CreateDefaultKeyringInternal(kMnemonic1, false));
  base::RunLoop().RunUntilIdle();
  controller.default_keyring_->AddAccounts(1);
  EXPECT_EQ(controller.default_keyring_->GetAddress(0),
            "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db");
  const std::string encrypted_mnemonic1 =
      GetStringPrefForKeyring(kEncryptedMnemonic, "default");
  // The pref is encrypted
  EXPECT_NE(
      base::Base64Encode(std::vector<uint8_t>(
          reinterpret_cast<const uint8_t*>(kMnemonic1),
          reinterpret_cast<const uint8_t*>(kMnemonic1) + strlen(kMnemonic1))),
      encrypted_mnemonic1);

  // default keyring will be overwritten
  ASSERT_TRUE(controller.CreateDefaultKeyringInternal(kMnemonic2, false));
  controller.default_keyring_->AddAccounts(1);
  EXPECT_EQ(controller.default_keyring_->GetAddress(0),
            "0xf83C3cBfF68086F276DD4f87A82DF73B57b28820");
  EXPECT_NE(GetStringPrefForKeyring(kEncryptedMnemonic, "default"),
            encrypted_mnemonic1);
}

TEST_F(KeyringControllerUnitTest, CreateDefaultKeyring) {
  std::string salt;
  std::string nonce;
  std::string mnemonic;
  {
    KeyringController controller(GetPrefs());
    EXPECT_EQ(controller.CreateDefaultKeyring(""), nullptr);
    EXPECT_FALSE(HasPrefForKeyring(kPasswordEncryptorSalt, "default"));
    EXPECT_FALSE(HasPrefForKeyring(kPasswordEncryptorNonce, "default"));
    EXPECT_FALSE(HasPrefForKeyring(kEncryptedMnemonic, "default"));

    HDKeyring* keyring = controller.CreateDefaultKeyring("brave1");
    EXPECT_EQ(keyring->type(), HDKeyring::Type::kDefault);
    keyring->AddAccounts(1);
    const std::string address1 = keyring->GetAddress(0);
    EXPECT_FALSE(address1.empty());
    EXPECT_TRUE(HasPrefForKeyring(kPasswordEncryptorSalt, "default"));
    EXPECT_TRUE(HasPrefForKeyring(kPasswordEncryptorNonce, "default"));
    EXPECT_TRUE(HasPrefForKeyring(kEncryptedMnemonic, "default"));

    // default keyring will be overwritten
    keyring = controller.CreateDefaultKeyring("brave2");
    keyring->AddAccounts(1);
    const std::string address2 = keyring->GetAddress(0);
    EXPECT_FALSE(address2.empty());
    EXPECT_NE(address1, address2);

    salt = GetStringPrefForKeyring(kPasswordEncryptorSalt, "default");
    nonce = GetStringPrefForKeyring(kPasswordEncryptorNonce, "default");
    mnemonic = GetStringPrefForKeyring(kEncryptedMnemonic, "default");
  }

  // mnemonic, salt and account number don't get clear unless Reset() is called
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorSalt, "default"), salt);
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorNonce, "default"), nonce);
  EXPECT_EQ(GetStringPrefForKeyring(kEncryptedMnemonic, "default"), mnemonic);
}

TEST_F(KeyringControllerUnitTest, RestoreDefaultKeyring) {
  KeyringController controller(GetPrefs());
  controller.CreateWallet("brave", base::DoNothing());
  base::RunLoop().RunUntilIdle();
  std::string salt = GetStringPrefForKeyring(kPasswordEncryptorSalt, "default");
  std::string encrypted_mnemonic =
      GetStringPrefForKeyring(kEncryptedMnemonic, "default");
  std::string nonce =
      GetStringPrefForKeyring(kPasswordEncryptorNonce, "default");
  const std::string mnemonic = controller.GetMnemonicForDefaultKeyringImpl();

  // Restore with same mnemonic and same password
  EXPECT_NE(controller.RestoreDefaultKeyring(mnemonic, "brave", false),
            nullptr);
  EXPECT_EQ(GetStringPrefForKeyring(kEncryptedMnemonic, "default"),
            encrypted_mnemonic);
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorSalt, "default"), salt);
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorNonce, "default"), nonce);
  EXPECT_EQ(controller.default_keyring_->GetAccountsNumber(), 1u);

  // Restore with same mnemonic but different password
  EXPECT_NE(controller.RestoreDefaultKeyring(mnemonic, "brave377", false),
            nullptr);
  EXPECT_NE(GetStringPrefForKeyring(kEncryptedMnemonic, "default"),
            encrypted_mnemonic);
  EXPECT_NE(GetStringPrefForKeyring(kPasswordEncryptorSalt, "default"), salt);
  EXPECT_NE(GetStringPrefForKeyring(kPasswordEncryptorNonce, "default"), nonce);
  EXPECT_EQ(controller.GetMnemonicForDefaultKeyringImpl(), mnemonic);
  EXPECT_EQ(controller.default_keyring_->GetAccountsNumber(), 0u);

  // Update salt for next test case
  encrypted_mnemonic = GetStringPrefForKeyring(kEncryptedMnemonic, "default");
  salt = GetStringPrefForKeyring(kPasswordEncryptorSalt, "default");
  nonce = GetStringPrefForKeyring(kPasswordEncryptorNonce, "default");

  // Restore with invalid mnemonic but same password
  EXPECT_EQ(controller.RestoreDefaultKeyring("", "brave", false), nullptr);
  // Keyring prefs won't be cleared
  EXPECT_EQ(GetStringPrefForKeyring(kEncryptedMnemonic, "default"),
            encrypted_mnemonic);
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorSalt, "default"), salt);
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorNonce, "default"), nonce);
  EXPECT_EQ(controller.default_keyring_->GetAccountsNumber(), 0u);

  // Restore with same mnemonic but empty password
  EXPECT_EQ(controller.RestoreDefaultKeyring(mnemonic, "", false), nullptr);
  // Keyring prefs won't be cleared
  EXPECT_EQ(GetStringPrefForKeyring(kEncryptedMnemonic, "default"),
            encrypted_mnemonic);
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorSalt, "default"), salt);
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorNonce, "default"), nonce);
  EXPECT_EQ(controller.default_keyring_->GetAccountsNumber(), 0u);

  // default keyring will be overwritten by new seed which will be encrypted by
  // new key even though the passphrase is same.
  EXPECT_NE(controller.RestoreDefaultKeyring(kMnemonic1, "brave", false),
            nullptr);
  EXPECT_NE(GetStringPrefForKeyring(kEncryptedMnemonic, "default"),
            encrypted_mnemonic);
  // salt is regenerated and account num is cleared
  EXPECT_NE(GetStringPrefForKeyring(kPasswordEncryptorSalt, "default"), salt);
  EXPECT_NE(GetStringPrefForKeyring(kPasswordEncryptorNonce, "default"), nonce);
  controller.AddAccount("Account 1", base::DoNothing());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(controller.default_keyring_->GetAccountsNumber(), 1u);
  EXPECT_EQ(controller.default_keyring_->GetAddress(0),
            "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db");
}

TEST_F(KeyringControllerUnitTest, UnlockResumesDefaultKeyring) {
  std::string salt;
  std::string mnemonic;
  std::string nonce;
  {
    KeyringController controller(GetPrefs());
    controller.CreateWallet("brave", base::DoNothing());
    base::RunLoop().RunUntilIdle();
    controller.AddAccount("Account2", base::DoNothing());
    base::RunLoop().RunUntilIdle();

    salt = GetStringPrefForKeyring(kPasswordEncryptorSalt, "default");
    nonce = GetStringPrefForKeyring(kPasswordEncryptorNonce, "default");
    mnemonic = GetStringPrefForKeyring(kEncryptedMnemonic, "default");
  }
  {
    // KeyringController is now destructed, simlulating relaunch
    KeyringController controller(GetPrefs());
    controller.Unlock(
        "brave", base::BindOnce(&KeyringControllerUnitTest::GetBooleanCallback,
                                base::Unretained(this)));
    base::RunLoop().RunUntilIdle();
    ASSERT_EQ(true, bool_value());
    ASSERT_FALSE(controller.IsLocked());

    EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorSalt, "default"), salt);
    EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorNonce, "default"),
              nonce);
    EXPECT_EQ(GetStringPrefForKeyring(kEncryptedMnemonic, "default"), mnemonic);
    EXPECT_EQ(2u, controller.GetAccountInfosForKeyring("default").size());
  }
  {
    KeyringController controller(GetPrefs());
    // wrong password
    controller.Unlock(
        "brave123",
        base::BindOnce(&KeyringControllerUnitTest::GetBooleanCallback,
                       base::Unretained(this)));
    ASSERT_TRUE(controller.IsLocked());
    // empty password
    controller.Unlock(
        "", base::BindOnce(&KeyringControllerUnitTest::GetBooleanCallback,
                           base::Unretained(this)));
    ASSERT_TRUE(controller.IsLocked());
  }
}

TEST_F(KeyringControllerUnitTest, GetMnemonicForDefaultKeyring) {
  KeyringController controller(GetPrefs());
  ASSERT_TRUE(controller.CreateEncryptorForKeyring("brave", "default"));

  // no pref exists yet
  controller.GetMnemonicForDefaultKeyring(base::BindOnce(
      &KeyringControllerUnitTest::GetStringCallback, base::Unretained(this)));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(string_value().empty());

  ASSERT_TRUE(controller.CreateDefaultKeyringInternal(kMnemonic1, false));
  controller.GetMnemonicForDefaultKeyring(base::BindOnce(
      &KeyringControllerUnitTest::GetStringCallback, base::Unretained(this)));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(string_value(), kMnemonic1);

  // Lock controller
  controller.Lock();
  EXPECT_TRUE(controller.IsLocked());
  controller.GetMnemonicForDefaultKeyring(base::BindOnce(
      &KeyringControllerUnitTest::GetStringCallback, base::Unretained(this)));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(string_value().empty());

  // unlock with wrong password
  controller.Unlock(
      "brave123", base::BindOnce(&KeyringControllerUnitTest::GetBooleanCallback,
                                 base::Unretained(this)));
  EXPECT_TRUE(controller.IsLocked());
  controller.GetMnemonicForDefaultKeyring(base::BindOnce(
      &KeyringControllerUnitTest::GetStringCallback, base::Unretained(this)));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(string_value().empty());

  controller.Unlock(
      "brave", base::BindOnce(&KeyringControllerUnitTest::GetBooleanCallback,
                              base::Unretained(this)));
  EXPECT_FALSE(controller.IsLocked());
  controller.GetMnemonicForDefaultKeyring(base::BindOnce(
      &KeyringControllerUnitTest::GetStringCallback, base::Unretained(this)));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(string_value(), kMnemonic1);
}

TEST_F(KeyringControllerUnitTest, GetDefaultKeyringInfo) {
  KeyringController controller(GetPrefs());
  bool callback_called = false;
  controller.GetDefaultKeyringInfo(
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_FALSE(keyring_info->is_default_keyring_created);
        EXPECT_TRUE(keyring_info->is_locked);
        EXPECT_FALSE(keyring_info->is_backed_up);
        EXPECT_TRUE(keyring_info->account_infos.empty());
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  controller.CreateWallet("brave", base::DoNothing());
  base::RunLoop().RunUntilIdle();

  callback_called = false;
  controller.GetDefaultKeyringInfo(
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_TRUE(keyring_info->is_default_keyring_created);
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

  controller.NotifyWalletBackupComplete();
  controller.AddAccount("Account5566", base::DoNothing());
  base::RunLoop().RunUntilIdle();

  callback_called = false;
  controller.GetDefaultKeyringInfo(
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_TRUE(keyring_info->is_default_keyring_created);
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
}

TEST_F(KeyringControllerUnitTest, LockAndUnlock) {
  {
    KeyringController controller(GetPrefs());
    // No encryptor
    controller.Lock();
    EXPECT_TRUE(controller.IsLocked());
    EXPECT_TRUE(controller.CreateEncryptorForKeyring("123", "default"));
    EXPECT_FALSE(controller.IsLocked());
    // No default keyring
    controller.Lock();
  }
  {
    KeyringController controller(GetPrefs());
    ASSERT_NE(controller.CreateDefaultKeyring("brave"), nullptr);
    controller.default_keyring_->AddAccounts(1);
    EXPECT_FALSE(controller.IsLocked());

    controller.Lock();
    EXPECT_TRUE(controller.IsLocked());
    EXPECT_FALSE(controller.default_keyring_);

    controller.Unlock(
        "abc", base::BindOnce(&KeyringControllerUnitTest::GetBooleanCallback,
                              base::Unretained(this)));
    EXPECT_TRUE(controller.IsLocked());

    controller.Unlock(
        "brave", base::BindOnce(&KeyringControllerUnitTest::GetBooleanCallback,
                                base::Unretained(this)));
    EXPECT_FALSE(controller.IsLocked());
    controller.default_keyring_->AddAccounts(1);

    controller.Lock();
    EXPECT_TRUE(controller.IsLocked());
    EXPECT_FALSE(controller.default_keyring_);

    // Simulate unlock shutdown
    controller.Unlock(
        "brave", base::BindOnce(&KeyringControllerUnitTest::GetBooleanCallback,
                                base::Unretained(this)));
    EXPECT_FALSE(controller.IsLocked());
    controller.default_keyring_->AddAccounts(1);
  }
}

TEST_F(KeyringControllerUnitTest, Reset) {
  KeyringController controller(GetPrefs());
  HDKeyring* keyring = controller.CreateDefaultKeyring("brave");
  keyring->AddAccounts();
  // Trigger account number saving
  controller.Lock();

  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletKeyrings));
  EXPECT_TRUE(HasPrefForKeyring(kPasswordEncryptorSalt, "default"));
  EXPECT_TRUE(HasPrefForKeyring(kPasswordEncryptorNonce, "default"));
  EXPECT_TRUE(HasPrefForKeyring(kEncryptedMnemonic, "default"));
  GetPrefs()->Set(kBraveWalletCustomNetworks, base::ListValue());
  GetPrefs()->SetString(kBraveWalletCurrentChainId,
                        brave_wallet::mojom::kMainnetChainId);
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletCustomNetworks));
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletCurrentChainId));
  controller.Reset();
  EXPECT_FALSE(HasPrefForKeyring(kPasswordEncryptorSalt, "default"));
  EXPECT_FALSE(HasPrefForKeyring(kPasswordEncryptorNonce, "default"));
  EXPECT_FALSE(HasPrefForKeyring(kEncryptedMnemonic, "default"));
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletKeyrings));
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletCustomNetworks));
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletCurrentChainId));
  EXPECT_EQ(controller.default_keyring_, nullptr);
  EXPECT_EQ(controller.encryptor_, nullptr);
}

TEST_F(KeyringControllerUnitTest, BackupComplete) {
  KeyringController controller(GetPrefs());

  bool callback_called = false;
  controller.IsWalletBackedUp(base::BindLambdaForTesting([&](bool backed_up) {
    EXPECT_FALSE(backed_up);
    callback_called = true;
  }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  controller.NotifyWalletBackupComplete();

  callback_called = false;
  controller.IsWalletBackedUp(base::BindLambdaForTesting([&](bool backed_up) {
    EXPECT_TRUE(backed_up);
    callback_called = true;
  }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  controller.Reset();

  callback_called = false;
  controller.IsWalletBackedUp(base::BindLambdaForTesting([&](bool backed_up) {
    EXPECT_FALSE(backed_up);
    callback_called = true;
  }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(KeyringControllerUnitTest, AccountMetasForKeyring) {
  KeyringController controller(GetPrefs());
  EXPECT_TRUE(controller.CreateEncryptorForKeyring("brave", "default"));
  ASSERT_TRUE(controller.CreateDefaultKeyringInternal(kMnemonic1, false));
  controller.default_keyring_->AddAccounts(2);
  const std::string address1 = controller.default_keyring_->GetAddress(0);
  const std::string name1 = "Account1";
  const std::string account_path1 = KeyringController::GetAccountPathByIndex(0);
  const std::string address2 = controller.default_keyring_->GetAddress(1);
  const std::string name2 = "Account2";
  const std::string account_path2 = KeyringController::GetAccountPathByIndex(1);

  KeyringController::SetAccountMetaForKeyring(GetPrefs(), account_path1, name1,
                                              address1, "default");
  KeyringController::SetAccountMetaForKeyring(GetPrefs(), account_path2, name2,
                                              address2, "default");

  const base::Value* account_metas = KeyringController::GetPrefForKeyring(
      GetPrefs(), kAccountMetas, "default");
  ASSERT_NE(account_metas, nullptr);

  EXPECT_EQ(
      account_metas->FindPath(account_path1 + ".account_name")->GetString(),
      name1);
  EXPECT_EQ(
      account_metas->FindPath(account_path2 + ".account_name")->GetString(),
      name2);
  EXPECT_EQ(KeyringController::GetAccountNameForKeyring(
                GetPrefs(), account_path1, "default"),
            name1);
  EXPECT_EQ(KeyringController::GetAccountAddressForKeyring(
                GetPrefs(), account_path1, "default"),
            address1);
  EXPECT_EQ(KeyringController::GetAccountNameForKeyring(
                GetPrefs(), account_path2, "default"),
            name2);
  EXPECT_EQ(KeyringController::GetAccountAddressForKeyring(
                GetPrefs(), account_path2, "default"),
            address2);
  EXPECT_EQ(controller.GetAccountMetasNumberForKeyring("default"), 2u);
  EXPECT_EQ(controller.GetAccountMetasNumberForKeyring("keyring1"), 0u);

  // GetAccountInfosForKeyring should work even if the keyring is locked
  controller.Lock();
  std::vector<mojom::AccountInfoPtr> account_infos =
      controller.GetAccountInfosForKeyring("default");
  EXPECT_EQ(account_infos.size(), 2u);
  EXPECT_EQ(account_infos[0]->address, address1);
  EXPECT_EQ(account_infos[0]->name, name1);
  EXPECT_EQ(account_infos[1]->address, address2);
  EXPECT_EQ(account_infos[1]->name, name2);
}

TEST_F(KeyringControllerUnitTest, CreateAndRestoreWallet) {
  KeyringController controller(GetPrefs());
  bool callback_called = false;
  std::string mnemonic_to_be_restored;
  controller.CreateWallet(
      "brave", base::BindLambdaForTesting([&](const std::string& mnemonic) {
        EXPECT_FALSE(mnemonic.empty());
        mnemonic_to_be_restored = mnemonic;
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  std::vector<mojom::AccountInfoPtr> account_infos =
      controller.GetAccountInfosForKeyring("default");
  EXPECT_EQ(account_infos.size(), 1u);
  EXPECT_FALSE(account_infos[0]->address.empty());
  const std::string address0 = account_infos[0]->address;
  EXPECT_EQ(account_infos[0]->name, "Account 1");

  controller.Reset();
  auto verify_restore_wallet = base::BindLambdaForTesting(
      [&mnemonic_to_be_restored, &controller, &address0]() {
        bool callback_called = false;
        controller.RestoreWallet(mnemonic_to_be_restored, "brave1", false,
                                 base::BindLambdaForTesting([&](bool success) {
                                   EXPECT_TRUE(success);
                                   callback_called = true;
                                 }));
        base::RunLoop().RunUntilIdle();
        EXPECT_TRUE(callback_called);
        {
          std::vector<mojom::AccountInfoPtr> account_infos =
              controller.GetAccountInfosForKeyring("default");
          EXPECT_EQ(account_infos.size(), 1u);
          EXPECT_EQ(account_infos[0]->address, address0);
          EXPECT_EQ(account_infos[0]->name, "Account 1");
        }
      });
  verify_restore_wallet.Run();
  // Restore twice consecutively should succeed and have only one account
  verify_restore_wallet.Run();
}

TEST_F(KeyringControllerUnitTest, AddAccount) {
  KeyringController controller(GetPrefs());
  controller.CreateWallet("brave", base::DoNothing());
  base::RunLoop().RunUntilIdle();
  bool callback_called = false;
  controller.AddAccount("Account5566",
                        base::BindLambdaForTesting([&](bool success) {
                          EXPECT_TRUE(success);
                          callback_called = true;
                        }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  std::vector<mojom::AccountInfoPtr> account_infos =
      controller.GetAccountInfosForKeyring("default");
  EXPECT_EQ(account_infos.size(), 2u);
  EXPECT_FALSE(account_infos[0]->address.empty());
  EXPECT_EQ(account_infos[0]->name, "Account 1");
  EXPECT_FALSE(account_infos[1]->address.empty());
  EXPECT_EQ(account_infos[1]->name, "Account5566");
}

TEST_F(KeyringControllerUnitTest, GetAccountPathByIndex) {
  EXPECT_EQ(KeyringController::GetAccountPathByIndex(0), "m/44'/60'/0'/0/0");
  EXPECT_EQ(KeyringController::GetAccountPathByIndex(3), "m/44'/60'/0'/0/3");
}

TEST_F(KeyringControllerUnitTest, MigrationPrefs) {
  GetPrefs()->SetString(kBraveWalletPasswordEncryptorSalt, "test_salt");
  GetPrefs()->SetString(kBraveWalletPasswordEncryptorNonce, "test_nonce");
  GetPrefs()->SetString(kBraveWalletEncryptedMnemonic, "test_mnemonic");
  GetPrefs()->SetInteger(kBraveWalletDefaultKeyringAccountNum, 3);

  base::Value account_names(base::Value::Type::LIST);
  account_names.Append(base::Value("Account1"));
  account_names.Append(base::Value("Account2"));
  account_names.Append(base::Value("Account3"));
  GetPrefs()->Set(kBraveWalletAccountNames, account_names);

  GetPrefs()->SetBoolean(kBraveWalletBackupComplete, true);

  KeyringController::MigrateObsoleteProfilePrefs(GetPrefs());

  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorSalt, "default"),
            "test_salt");
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorNonce, "default"),
            "test_nonce");
  EXPECT_EQ(GetStringPrefForKeyring(kEncryptedMnemonic, "default"),
            "test_mnemonic");

  const base::Value* backup_complete = KeyringController::GetPrefForKeyring(
      GetPrefs(), kBackupComplete, "default");
  ASSERT_TRUE(backup_complete);
  EXPECT_TRUE(backup_complete->GetBool());

  const base::Value* account_metas = KeyringController::GetPrefForKeyring(
      GetPrefs(), kAccountMetas, "default");
  EXPECT_EQ(account_metas->DictSize(), 3u);
  EXPECT_EQ(
      KeyringController::GetAccountNameForKeyring(
          GetPrefs(), KeyringController::GetAccountPathByIndex(0), "default"),
      "Account1");
  EXPECT_EQ(
      KeyringController::GetAccountNameForKeyring(
          GetPrefs(), KeyringController::GetAccountPathByIndex(1), "default"),
      "Account2");
  EXPECT_EQ(
      KeyringController::GetAccountNameForKeyring(
          GetPrefs(), KeyringController::GetAccountPathByIndex(2), "default"),
      "Account3");
}

TEST_F(KeyringControllerUnitTest, MigrationPrefsFailSafe) {
  GetPrefs()->SetInteger(kBraveWalletDefaultKeyringAccountNum, 2);

  base::Value account_names(base::Value::Type::LIST);
  account_names.Append(base::Value("Account1"));
  account_names.Append(base::Value("Account2"));
  account_names.Append(base::Value("Account3"));
  GetPrefs()->Set(kBraveWalletAccountNames, account_names);

  KeyringController::MigrateObsoleteProfilePrefs(GetPrefs());
  const base::Value* account_metas = KeyringController::GetPrefForKeyring(
      GetPrefs(), kAccountMetas, "default");
  EXPECT_EQ(account_metas->DictSize(), 1u);
  EXPECT_EQ(
      KeyringController::GetAccountNameForKeyring(
          GetPrefs(), KeyringController::GetAccountPathByIndex(0), "default"),
      "Account 1");
}

TEST_F(KeyringControllerUnitTest, ImportedAccounts) {
  KeyringController controller(GetPrefs());

  TestKeyringControllerObserver observer;
  controller.AddObserver(observer.GetReceiver());

  controller.CreateWallet("brave", base::DoNothing());
  base::RunLoop().RunUntilIdle();
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
    bool callback_called = false;
    controller.ImportAccount(
        imported_accounts[i].name, imported_accounts[i].private_key,
        base::BindLambdaForTesting(
            [&](bool success, const std::string& address) {
              EXPECT_TRUE(success);
              EXPECT_EQ(imported_accounts[i].address, address);
              callback_called = true;
            }));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(callback_called);

    callback_called = false;
    controller.GetPrivateKeyForImportedAccount(
        imported_accounts[i].address,
        base::BindLambdaForTesting(
            [&](bool success, const std::string& private_key) {
              EXPECT_TRUE(success);
              EXPECT_EQ(imported_accounts[i].private_key, private_key);
              callback_called = true;
            }));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(callback_called);
  }

  bool callback_called = false;
  base::RunLoop().RunUntilIdle();
  observer.Reset();
  EXPECT_FALSE(observer.AccountsChangedFired());
  controller.RemoveImportedAccount(
      imported_accounts[1].address,
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_TRUE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();

  // remove invalid address
  EXPECT_FALSE(observer.AccountsChangedFired());
  controller.RemoveImportedAccount(
      "0xxxxxxxxxx0", base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(observer.AccountsChangedFired());

  callback_called = false;
  controller.GetDefaultKeyringInfo(
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_TRUE(keyring_info->is_default_keyring_created);
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

  controller.Lock();
  // cannot get private key when locked
  callback_called = false;
  controller.GetPrivateKeyForImportedAccount(
      imported_accounts[0].address,
      base::BindLambdaForTesting(
          [&](bool success, const std::string& private_key) {
            EXPECT_FALSE(success);
            EXPECT_TRUE(private_key.empty());
            callback_called = true;
          }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  controller.Unlock("brave", base::DoNothing());
  base::RunLoop().RunUntilIdle();

  callback_called = false;
  // Imported accounts should be restored
  controller.GetDefaultKeyringInfo(
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
  callback_called = false;
  controller.GetPrivateKeyForImportedAccount(
      imported_accounts[0].address,
      base::BindLambdaForTesting(
          [&](bool success, const std::string& private_key) {
            EXPECT_TRUE(success);
            EXPECT_EQ(imported_accounts[0].private_key, private_key);
            callback_called = true;
          }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Imported accounts should also be restored in default keyring
  EXPECT_EQ(controller.default_keyring_->GetImportedAccountsNumber(), 2u);

  const base::Value* imported_accounts_value =
      KeyringController::GetPrefForKeyring(GetPrefs(), kImportedAccounts,
                                           "default");
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

TEST_F(KeyringControllerUnitTest, ImportedAccountFromJson) {
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

  KeyringController controller(GetPrefs());
  controller.CreateWallet("brave", base::DoNothing());
  base::RunLoop().RunUntilIdle();

  bool callback_called = false;
  controller.ImportAccountFromJson(
      "Imported 1", "wrong password", json,
      base::BindLambdaForTesting([&](bool success, const std::string& address) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(address.empty());
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  controller.ImportAccountFromJson(
      "Imported 1", "testtest", "{crypto: 123}",
      base::BindLambdaForTesting([&](bool success, const std::string& address) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(address.empty());
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  controller.ImportAccountFromJson(
      "Imported 1", "testtest", json,
      base::BindLambdaForTesting([&](bool success, const std::string& address) {
        EXPECT_TRUE(success);
        EXPECT_EQ(address, expected_address);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  controller.Lock();
  controller.Unlock("brave", base::DoNothing());
  base::RunLoop().RunUntilIdle();

  // check restore by getting private key
  callback_called = false;
  controller.GetPrivateKeyForImportedAccount(
      expected_address, base::BindLambdaForTesting(
                            [&](bool success, const std::string& private_key) {
                              EXPECT_TRUE(success);
                              EXPECT_EQ(expected_private_key, private_key);
                              callback_called = true;
                            }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // private key is encrypted
  const base::Value* imported_accounts_value =
      KeyringController::GetPrefForKeyring(GetPrefs(), kImportedAccounts,
                                           "default");
  ASSERT_TRUE(imported_accounts_value);
  const std::string encrypted_private_key =
      imported_accounts_value->GetList()[0]
          .FindKey(kEncryptedPrivateKey)
          ->GetString();
  EXPECT_FALSE(encrypted_private_key.empty());

  std::vector<uint8_t> private_key;
  ASSERT_TRUE(base::HexStringToBytes(expected_private_key, &private_key));
  EXPECT_NE(encrypted_private_key, base::Base64Encode(private_key));
}

TEST_F(KeyringControllerUnitTest, GetPrivateKeyForDefaultKeyringAccount) {
  KeyringController controller(GetPrefs());
  EXPECT_TRUE(controller.CreateEncryptorForKeyring("brave", "default"));
  ASSERT_TRUE(controller.CreateDefaultKeyringInternal(kMnemonic1, false));

  bool callback_called = false;
  controller.GetPrivateKeyForDefaultKeyringAccount(
      "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db",
      base::BindLambdaForTesting(
          [&](bool success, const std::string& private_key) {
            EXPECT_FALSE(success);
            EXPECT_TRUE(private_key.empty());
            callback_called = true;
          }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  controller.default_keyring_->AddAccounts(1);
  EXPECT_EQ(controller.default_keyring_->GetAddress(0),
            "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db");

  callback_called = false;
  controller.GetPrivateKeyForDefaultKeyringAccount(
      "", base::BindLambdaForTesting(
              [&](bool success, const std::string& private_key) {
                EXPECT_FALSE(success);
                EXPECT_TRUE(private_key.empty());
                callback_called = true;
              }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  controller.GetPrivateKeyForDefaultKeyringAccount(
      "0x123", base::BindLambdaForTesting(
                   [&](bool success, const std::string& private_key) {
                     EXPECT_FALSE(success);
                     EXPECT_TRUE(private_key.empty());
                     callback_called = true;
                   }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  controller.GetPrivateKeyForDefaultKeyringAccount(
      "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db",
      base::BindLambdaForTesting([&](bool success,
                                     const std::string& private_key) {
        EXPECT_TRUE(success);
        EXPECT_EQ(
            "919af8081ce2a02d9650bf3e10ffb6b7cbadbb1dca749122d7d982cdb6cbcc50",
            private_key);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(KeyringControllerUnitTest, SetDefaultKeyringDerivedAccountMeta) {
  KeyringController controller(GetPrefs());

  TestKeyringControllerObserver observer;
  controller.AddObserver(observer.GetReceiver());

  const std::string kUpdatedName = "Updated";
  bool callback_called = false;
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.AccountsChangedFired());
  controller.SetDefaultKeyringDerivedAccountName(
      "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db", kUpdatedName,
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(observer.AccountsChangedFired());
  observer.Reset();

  EXPECT_TRUE(controller.CreateEncryptorForKeyring("brave", "default"));
  ASSERT_TRUE(controller.CreateDefaultKeyringInternal(kMnemonic1, false));
  controller.default_keyring_->AddAccounts(2);
  const std::string address1 = controller.default_keyring_->GetAddress(0);
  const std::string name1 = "Account1";
  const std::string account_path1 = KeyringController::GetAccountPathByIndex(0);
  const std::string address2 = controller.default_keyring_->GetAddress(1);
  const std::string name2 = "Account2";
  const std::string account_path2 = KeyringController::GetAccountPathByIndex(1);

  KeyringController::SetAccountMetaForKeyring(GetPrefs(), account_path1, name1,
                                              address1, "default");
  KeyringController::SetAccountMetaForKeyring(GetPrefs(), account_path2, name2,
                                              address2, "default");
  EXPECT_EQ(KeyringController::GetAccountNameForKeyring(
                GetPrefs(), account_path1, "default"),
            name1);
  EXPECT_EQ(KeyringController::GetAccountAddressForKeyring(
                GetPrefs(), account_path1, "default"),
            address1);
  EXPECT_EQ(KeyringController::GetAccountNameForKeyring(
                GetPrefs(), account_path2, "default"),
            name2);
  EXPECT_EQ(KeyringController::GetAccountAddressForKeyring(
                GetPrefs(), account_path2, "default"),
            address2);

  callback_called = false;
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.AccountsChangedFired());
  controller.SetDefaultKeyringDerivedAccountName(
      "", kUpdatedName, base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.AccountsChangedFired());
  observer.Reset();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  EXPECT_FALSE(observer.AccountsChangedFired());
  controller.SetDefaultKeyringDerivedAccountName(
      address2, "", base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.AccountsChangedFired());
  observer.Reset();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  EXPECT_FALSE(observer.AccountsChangedFired());
  controller.SetDefaultKeyringDerivedAccountName(
      address2, kUpdatedName, base::BindLambdaForTesting([&](bool success) {
        EXPECT_TRUE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();
  EXPECT_TRUE(callback_called);

  EXPECT_EQ(KeyringController::GetAccountNameForKeyring(
                GetPrefs(), account_path1, "default"),
            name1);
  EXPECT_EQ(KeyringController::GetAccountNameForKeyring(
                GetPrefs(), account_path2, "default"),
            kUpdatedName);
}

TEST_F(KeyringControllerUnitTest, SetDefaultKeyringImportedAccountName) {
  KeyringController controller(GetPrefs());

  TestKeyringControllerObserver observer;
  controller.AddObserver(observer.GetReceiver());

  controller.CreateWallet("brave", base::DoNothing());
  base::RunLoop().RunUntilIdle();

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
  bool callback_called = false;
  controller.SetDefaultKeyringImportedAccountName(
      imported_accounts[1].address, kUpdatedName,
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Add import accounts.
  for (size_t i = 0;
       i < sizeof(imported_accounts) / sizeof(imported_accounts[0]); ++i) {
    callback_called = false;
    EXPECT_FALSE(observer.AccountsChangedFired());
    controller.ImportAccount(
        imported_accounts[i].name, imported_accounts[i].private_key,
        base::BindLambdaForTesting(
            [&](bool success, const std::string& address) {
              EXPECT_TRUE(success);
              EXPECT_EQ(imported_accounts[i].address, address);
              callback_called = true;
            }));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(callback_called);
    EXPECT_TRUE(observer.AccountsChangedFired());
    observer.Reset();
  }

  // Empty address should fail.
  callback_called = false;
  controller.SetDefaultKeyringImportedAccountName(
      "", kUpdatedName, base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Empty name should fail.
  callback_called = false;
  controller.SetDefaultKeyringImportedAccountName(
      imported_accounts[1].address, "",
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Update second imported account's name.
  callback_called = false;
  controller.SetDefaultKeyringImportedAccountName(
      imported_accounts[1].address, kUpdatedName,
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_TRUE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Private key of imported accounts should not be changed.
  for (size_t i = 0;
       i < sizeof(imported_accounts) / sizeof(imported_accounts[0]); ++i) {
    callback_called = false;
    controller.GetPrivateKeyForImportedAccount(
        imported_accounts[i].address,
        base::BindLambdaForTesting(
            [&](bool success, const std::string& private_key) {
              EXPECT_TRUE(success);
              EXPECT_EQ(imported_accounts[i].private_key, private_key);
              callback_called = true;
            }));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(callback_called);
  }

  // Only second imported account's name is updated.
  callback_called = false;
  controller.GetDefaultKeyringInfo(
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_TRUE(keyring_info->is_default_keyring_created);
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
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(KeyringControllerUnitTest, RestoreLegacyBraveWallet) {
  const char* mnemonic24 =
      "cushion pitch impact album daring marine much annual budget social "
      "clarify balance rose almost area busy among bring hidden bind later "
      "capable pulp laundry";
  const char* mnemonic12 =
      "drip caution abandon festival order clown oven regular absorb evidence "
      "crew where";
  KeyringController controller(GetPrefs());
  auto verify_restore_wallet = base::BindLambdaForTesting(
      [&controller](const char* mnemonic, const char* address, bool is_legacy,
                    bool expect_result) {
        bool callback_called = false;
        controller.RestoreWallet(mnemonic, "brave1", is_legacy,
                                 base::BindLambdaForTesting([&](bool success) {
                                   EXPECT_EQ(success, expect_result);
                                   callback_called = true;
                                 }));
        base::RunLoop().RunUntilIdle();
        EXPECT_TRUE(callback_called);
        if (expect_result) {
          std::vector<mojom::AccountInfoPtr> account_infos =
              controller.GetAccountInfosForKeyring("default");
          ASSERT_EQ(account_infos.size(), 1u);
          EXPECT_EQ(account_infos[0]->address, address);
          EXPECT_EQ(account_infos[0]->name, "Account 1");

          // Test lock & unlock to check if it read the right
          // legacy_brave_wallet pref so it will use the right seed
          controller.Lock();
          controller.Unlock("brave1", base::DoNothing());
          base::RunLoop().RunUntilIdle();
          account_infos.clear();
          account_infos = controller.GetAccountInfosForKeyring("default");
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

TEST_F(KeyringControllerUnitTest, HardwareAccounts) {
  KeyringController controller(GetPrefs());

  TestKeyringControllerObserver observer;
  controller.AddObserver(observer.GetReceiver());

  controller.CreateWallet("brave", base::DoNothing());
  base::RunLoop().RunUntilIdle();

  std::vector<mojom::HardwareWalletAccountPtr> new_accounts;
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0x111", "m/44'/60'/1'/0/0", "name 1", "Ledger", "device1"));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0x264", "m/44'/60'/2'/0/0", "name 2", "Ledger", "device1"));
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0xEA0", "m/44'/60'/3'/0/0", "name 3", "Ledger", "device2"));

  EXPECT_FALSE(observer.AccountsChangedFired());
  controller.AddHardwareAccounts(std::move(new_accounts));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();
  ASSERT_TRUE(GetPrefs()
                  ->GetDictionary(kBraveWalletKeyrings)
                  ->FindPath("hardware.device1.account_metas.0x111"));

  bool callback_called = false;
  controller.GetDefaultKeyringInfo(
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        const auto& accounts = keyring_info->account_infos;
        EXPECT_EQ(accounts.size(), 4u);

        EXPECT_EQ(accounts[1]->address, "0x111");
        EXPECT_EQ(accounts[1]->name, "name 1");
        EXPECT_EQ(accounts[1]->is_imported, false);
        ASSERT_TRUE(accounts[1]->hardware);
        EXPECT_EQ(accounts[1]->hardware->device_id, "device1");

        EXPECT_EQ(accounts[2]->address, "0x264");
        EXPECT_EQ(accounts[2]->name, "name 2");
        EXPECT_EQ(accounts[2]->is_imported, false);
        ASSERT_TRUE(accounts[2]->hardware);
        EXPECT_EQ(accounts[2]->hardware->device_id, "device1");

        EXPECT_EQ(accounts[3]->address, "0xEA0");
        EXPECT_EQ(accounts[3]->name, "name 3");
        EXPECT_EQ(accounts[3]->is_imported, false);
        ASSERT_TRUE(accounts[3]->hardware);
        EXPECT_EQ(accounts[3]->hardware->device_id, "device2");

        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  EXPECT_FALSE(observer.AccountsChangedFired());
  controller.RemoveHardwareAccount("0x111");
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();

  ASSERT_FALSE(GetPrefs()
                   ->GetDictionary(kBraveWalletKeyrings)
                   ->FindPath("hardware.device1.account_metas.0x111"));

  ASSERT_TRUE(GetPrefs()
                  ->GetDictionary(kBraveWalletKeyrings)
                  ->FindPath("hardware.device1.account_metas.0x264"));

  ASSERT_TRUE(GetPrefs()
                  ->GetDictionary(kBraveWalletKeyrings)
                  ->FindPath("hardware.device2.account_metas.0xEA0"));

  EXPECT_FALSE(observer.AccountsChangedFired());
  controller.RemoveHardwareAccount("0x264");
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();

  callback_called = false;
  controller.GetDefaultKeyringInfo(
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        const auto& accounts = keyring_info->account_infos;
        EXPECT_EQ(accounts.size(), size_t(2));

        EXPECT_EQ(accounts[1]->address, "0xEA0");
        EXPECT_EQ(accounts[1]->name, "name 3");
        EXPECT_EQ(accounts[1]->is_imported, false);
        ASSERT_TRUE(accounts[1]->hardware);
        EXPECT_EQ(accounts[1]->hardware->device_id, "device2");

        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(observer.AccountsChangedFired());
  controller.RemoveHardwareAccount("0xEA0");
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.AccountsChangedFired());
  observer.Reset();

  ASSERT_FALSE(GetPrefs()
                   ->GetDictionary(kBraveWalletKeyrings)
                   ->FindPath("hardware.device2.account_metas.0xEA0"));

  ASSERT_FALSE(GetPrefs()
                   ->GetDictionary(kBraveWalletKeyrings)
                   ->FindPath("hardware.device2"));
}

TEST_F(KeyringControllerUnitTest, AutoLock) {
  KeyringController controller(GetPrefs());
  controller.CreateWallet("brave", base::DoNothing());
  base::RunLoop().RunUntilIdle();
  const std::string mnemonic = controller.GetMnemonicForDefaultKeyringImpl();
  ASSERT_FALSE(controller.IsLocked());

  // Should not be locked yet after 4 minutes
  task_environment_.FastForwardBy(base::Minutes(4));
  ASSERT_FALSE(controller.IsLocked());

  // After the 5th minute, it should be locked
  task_environment_.FastForwardBy(base::Minutes(1));
  ASSERT_TRUE(controller.IsLocked());
  // Locking after it is auto locked won't cause a crash
  controller.Lock();
  ASSERT_TRUE(controller.IsLocked());

  // Unlocking will reset the timer
  controller.Unlock(
      "brave", base::BindOnce(&KeyringControllerUnitTest::GetBooleanCallback,
                              base::Unretained(this)));
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(controller.IsLocked());
  task_environment_.FastForwardBy(base::Minutes(5));
  ASSERT_TRUE(controller.IsLocked());

  // Locking before the timer fires won't cause any problems after the
  // timer fires.
  controller.Unlock(
      "brave", base::BindOnce(&KeyringControllerUnitTest::GetBooleanCallback,
                              base::Unretained(this)));
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(controller.IsLocked());
  task_environment_.FastForwardBy(base::Minutes(1));
  controller.Lock();
  ASSERT_TRUE(controller.IsLocked());
  task_environment_.FastForwardBy(base::Minutes(4));
  ASSERT_TRUE(controller.IsLocked());

  // Restoring keyring will auto lock too
  controller.Reset();
  controller.RestoreWallet(mnemonic, "brave", false, base::DoNothing());
  ASSERT_FALSE(controller.IsLocked());
  task_environment_.FastForwardBy(base::Minutes(6));
  ASSERT_TRUE(controller.IsLocked());

  // Changing the auto lock pref should reset the timer
  controller.Unlock(
      "brave", base::BindOnce(&KeyringControllerUnitTest::GetBooleanCallback,
                              base::Unretained(this)));
  ASSERT_FALSE(controller.IsLocked());
  task_environment_.FastForwardBy(base::Minutes(4));
  GetPrefs()->SetInteger(kBraveWalletAutoLockMinutes, 3);
  task_environment_.FastForwardBy(base::Minutes(2));
  EXPECT_FALSE(controller.IsLocked());
  task_environment_.FastForwardBy(base::Minutes(1));
  EXPECT_TRUE(controller.IsLocked());

  // Changing the auto lock pref should reset the timer even if higher
  // for simplicity of logic
  controller.Unlock(
      "brave", base::BindOnce(&KeyringControllerUnitTest::GetBooleanCallback,
                              base::Unretained(this)));
  ASSERT_FALSE(controller.IsLocked());
  task_environment_.FastForwardBy(base::Minutes(2));
  EXPECT_FALSE(controller.IsLocked());
  GetPrefs()->SetInteger(kBraveWalletAutoLockMinutes, 10);
  task_environment_.FastForwardBy(base::Minutes(9));
  EXPECT_FALSE(controller.IsLocked());
  task_environment_.FastForwardBy(base::Minutes(1));
  EXPECT_TRUE(controller.IsLocked());
}

TEST_F(KeyringControllerUnitTest, NotifyUserInteraction) {
  KeyringController controller(GetPrefs());
  CreateWallet(&controller, "brave");
  ASSERT_FALSE(controller.IsLocked());

  // Notifying of user interaction should keep the wallet unlocked
  task_environment_.FastForwardBy(base::Minutes(4));
  controller.NotifyUserInteraction();
  task_environment_.FastForwardBy(base::Minutes(1));
  controller.NotifyUserInteraction();
  task_environment_.FastForwardBy(base::Minutes(4));
  ASSERT_FALSE(controller.IsLocked());
  task_environment_.FastForwardBy(base::Minutes(1));
  ASSERT_TRUE(controller.IsLocked());
}

TEST_F(KeyringControllerUnitTest, SetSelectedAccount) {
  KeyringController controller(GetPrefs());

  TestKeyringControllerObserver observer;
  controller.AddObserver(observer.GetReceiver());

  CreateWallet(&controller, "brave");

  std::string first_account = controller.default_keyring_->GetAddress(0);
  controller.AddAccountForDefaultKeyring("Who does number 2 work for");
  std::string second_account = controller.default_keyring_->GetAddress(1);

  // This does not depend on being locked
  EXPECT_TRUE(Lock(&controller));

  // No account set as the default
  EXPECT_EQ(absl::nullopt, GetSelectedAccount(&controller));

  // Setting account to a valid address works
  EXPECT_TRUE(SetSelectedAccount(&controller, &observer, second_account));
  EXPECT_EQ(second_account, GetSelectedAccount(&controller));

  // Setting account to a non-existing account doesn't work
  EXPECT_FALSE(SetSelectedAccount(
      &controller, &observer, "0xf83C3cBfF68086F276DD4f87A82DF73B57b21559"));
  EXPECT_EQ(second_account, GetSelectedAccount(&controller));
  base::RunLoop().RunUntilIdle();

  // Can import only when unlocked.
  // Then check that the account can be set to an imported account.
  EXPECT_TRUE(Unlock(&controller, "brave"));
  absl::optional<std::string> imported_account = ImportAccount(
      &controller, "Best Evil Son",
      // 0xDc06aE500aD5ebc5972A0D8Ada4733006E905976
      "d118a12a1e3b595d7d9e5599370df4ddc58d246a3ae4a795597e50eb6a32afb5");
  ASSERT_TRUE(imported_account.has_value());
  EXPECT_TRUE(Lock(&controller));
  EXPECT_TRUE(SetSelectedAccount(&controller, &observer, *imported_account));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(*imported_account, GetSelectedAccount(&controller));

  // Removing the imported account resets to no selected account
  observer.Reset();
  EXPECT_TRUE(Unlock(&controller, "brave"));
  EXPECT_TRUE(RemoveImportedAccount(
      &controller, "0xDc06aE500aD5ebc5972A0D8Ada4733006E905976"));
  EXPECT_TRUE(Lock(&controller));
  EXPECT_EQ(absl::nullopt, GetSelectedAccount(&controller));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.SelectedAccountChangedFired());
  observer.Reset();

  // Can set hardware account
  std::vector<mojom::HardwareWalletAccountPtr> new_accounts;
  std::string hardware_account = "0x1111111111111111111111111111111111111111";
  new_accounts.push_back(mojom::HardwareWalletAccount::New(
      hardware_account, "m/44'/60'/1'/0/0", "name 1", "Ledger", "device1"));
  AddHardwareAccount(&controller, std::move(new_accounts));
  EXPECT_TRUE(SetSelectedAccount(&controller, &observer, hardware_account));
  EXPECT_EQ(hardware_account, GetSelectedAccount(&controller));

  // Removing a hardware account resets to no selected account
  observer.Reset();
  controller.RemoveHardwareAccount(
      "0x1111111111111111111111111111111111111111");
  EXPECT_EQ(absl::nullopt, GetSelectedAccount(&controller));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.SelectedAccountChangedFired());
  observer.Reset();
}

TEST_F(KeyringControllerUnitTest, AddAccountsWithDefaultName) {
  KeyringController controller(GetPrefs());
  controller.CreateWallet("brave", base::DoNothing());
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(controller.IsLocked());

  controller.AddAccount("AccountAAAAH", base::DoNothing());

  controller.AddAccountsWithDefaultName(3);

  base::RunLoop run_loop;
  controller.GetDefaultKeyringInfo(
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_TRUE(keyring_info->is_default_keyring_created);
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

TEST_F(KeyringControllerUnitTest, SignMessageByDefaultKeyring) {
  // HDKeyringUnitTest.SignMessage already tests the correctness of signature
  KeyringController controller(GetPrefs());
  controller.RestoreWallet(kMnemonic1, "brave", false, base::DoNothing());
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(controller.IsLocked());

  std::string account1;
  {
    base::RunLoop run_loop;
    controller.GetDefaultKeyringInfo(
        base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
          ASSERT_EQ(keyring_info->account_infos.size(), 1u);
          account1 = keyring_info->account_infos[0]->address;
          run_loop.Quit();
        }));
    run_loop.Run();
  }
  const std::vector<uint8_t> message = {0xde, 0xad, 0xbe, 0xef};
  auto sig_with_err = controller.SignMessageByDefaultKeyring(account1, message);
  EXPECT_NE(sig_with_err.signature, absl::nullopt);
  EXPECT_FALSE(sig_with_err.signature->empty());
  EXPECT_TRUE(sig_with_err.error_message.empty());

  // message is 0x
  sig_with_err =
      controller.SignMessageByDefaultKeyring(account1, std::vector<uint8_t>());
  EXPECT_NE(sig_with_err.signature, absl::nullopt);
  EXPECT_FALSE(sig_with_err.signature->empty());
  EXPECT_TRUE(sig_with_err.error_message.empty());

  // not a valid account in this wallet
  const std::vector<std::string> invalid_accounts(
      {"0xea3C17c81E3baC3472d163b2c8b12ddDAa027874", "", "0x1234"});
  for (const auto& invalid_account : invalid_accounts) {
    sig_with_err =
        controller.SignMessageByDefaultKeyring(invalid_account, message);
    EXPECT_EQ(sig_with_err.signature, absl::nullopt);
    EXPECT_EQ(
        sig_with_err.error_message,
        l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_SIGN_MESSAGE_INVALID_ADDRESS,
                                  base::ASCIIToUTF16(invalid_account)));
  }

  // Cannot sign message when locked
  controller.Lock();
  sig_with_err = controller.SignMessageByDefaultKeyring(account1, message);
  EXPECT_EQ(sig_with_err.signature, absl::nullopt);
  EXPECT_EQ(
      sig_with_err.error_message,
      l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SIGN_MESSAGE_UNLOCK_FIRST));
}

TEST_F(KeyringControllerUnitTest, GetSetAutoLockMinutes) {
  KeyringController controller(GetPrefs());
  TestKeyringControllerObserver observer;
  controller.AddObserver(observer.GetReceiver());
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(5, GetAutoLockMinutes(&controller));
  EXPECT_TRUE(SetAutoLockMinutes(&controller, &observer, 7));
  EXPECT_EQ(7, GetAutoLockMinutes(&controller));
  EXPECT_TRUE(SetAutoLockMinutes(&controller, &observer, 3));
  EXPECT_EQ(3, GetAutoLockMinutes(&controller));

  // Out of bound values cannot be set
  EXPECT_FALSE(
      SetAutoLockMinutes(&controller, &observer, kAutoLockMinutesMin - 1));
  EXPECT_EQ(3, GetAutoLockMinutes(&controller));
  EXPECT_FALSE(
      SetAutoLockMinutes(&controller, &observer, kAutoLockMinutesMax + 1));
  EXPECT_EQ(3, GetAutoLockMinutes(&controller));

  // Bound values can be set
  EXPECT_TRUE(SetAutoLockMinutes(&controller, &observer, kAutoLockMinutesMin));
  EXPECT_EQ(kAutoLockMinutesMin, GetAutoLockMinutes(&controller));
  EXPECT_TRUE(SetAutoLockMinutes(&controller, &observer, kAutoLockMinutesMax));
  EXPECT_EQ(kAutoLockMinutesMax, GetAutoLockMinutes(&controller));
}

TEST_F(KeyringControllerUnitTest, SetDefaultKeyringHardwareAccountName) {
  KeyringController controller(GetPrefs());

  TestKeyringControllerObserver observer;
  controller.AddObserver(observer.GetReceiver());

  controller.CreateWallet("brave", base::DoNothing());
  base::RunLoop().RunUntilIdle();

  const struct {
    const char* address;
    const char* derivation_path;
    const char* name;
    const char* vendor;
    const char* device_id;
  } hardware_accounts[] = {
      {"0x111", "m/44'/60'/1'/0/0", "name 1", "Ledger", "device1"},
      {"0x264", "m/44'/60'/2'/0/0", "name 2", "Ledger", "device1"},
      {"0xEA0", "m/44'/60'/3'/0/0", "name 3", "Ledger", "device2"}};

  std::vector<mojom::HardwareWalletAccountPtr> new_accounts;
  for (const auto& it : hardware_accounts) {
    new_accounts.push_back(mojom::HardwareWalletAccount::New(
        it.address, it.derivation_path, it.name, it.vendor, it.device_id));
  }

  const std::string kUpdatedName = "Updated ledger accoount 2";

  // Fail when no hardware accounts.
  bool callback_called = false;
  controller.SetDefaultKeyringHardwareAccountName(
      hardware_accounts[1].address, kUpdatedName,
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  EXPECT_FALSE(observer.AccountsChangedFired());
  controller.AddHardwareAccounts(std::move(new_accounts));
  base::RunLoop().RunUntilIdle();

  // Empty address should fail.
  callback_called = false;
  controller.SetDefaultKeyringHardwareAccountName(
      "", kUpdatedName, base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Empty name should fail.
  callback_called = false;
  controller.SetDefaultKeyringHardwareAccountName(
      hardware_accounts[1].address, "",
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Update second hardware account's name.
  callback_called = false;
  controller.SetDefaultKeyringHardwareAccountName(
      hardware_accounts[1].address, kUpdatedName,
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_TRUE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Only second hardware account's name is updated.
  callback_called = false;
  controller.GetDefaultKeyringInfo(
      base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
        EXPECT_TRUE(keyring_info->is_default_keyring_created);
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
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

}  // namespace brave_wallet
