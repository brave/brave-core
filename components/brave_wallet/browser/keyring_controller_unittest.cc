/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/keyring_controller.h"

#include <utility>

#include "base/base64.h"
#include "base/test/bind.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {
const char kPasswordEncryptorSalt[] = "password_encryptor_salt";
const char kPasswordEncryptorNonce[] = "password_encryptor_nonce";
const char kEncryptedMnemonic[] = "encrypted_mnemonic";
const char kBackupComplete[] = "backup_complete";
const char kAccountMetas[] = "account_metas";
}  // namespace

class KeyringControllerUnitTest : public testing::Test {
 public:
  KeyringControllerUnitTest() {}
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

  bool bool_value() { return bool_value_; }
  const std::string string_value() { return string_value_; }

 private:
  content::BrowserTaskEnvironment task_environment_;
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
  const std::string mnemonic1 =
      "divide cruise upon flag harsh carbon filter merit once advice bright "
      "drive";
  // encryptor is nullptr
  ASSERT_FALSE(controller.CreateDefaultKeyringInternal(mnemonic1));

  EXPECT_TRUE(controller.CreateEncryptorForKeyring("brave", "default"));
  ASSERT_TRUE(controller.CreateDefaultKeyringInternal(mnemonic1));
  controller.default_keyring_->AddAccounts(1);
  EXPECT_EQ(controller.default_keyring_->GetAddress(0),
            "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db");
  const std::string encrypted_mnemonic1 =
      GetStringPrefForKeyring(kEncryptedMnemonic, "default");
  // The pref is encrypted
  EXPECT_NE(base::Base64Encode(
                std::vector<uint8_t>(mnemonic1.begin(), mnemonic1.end())),
            encrypted_mnemonic1);

  // default keyring will be overwritten
  const std::string mnemonic2 =
      "misery jeans response tiny nominee civil zoo strong correct taxi "
      "chimney goat";
  ASSERT_TRUE(controller.CreateDefaultKeyringInternal(mnemonic2));
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
  controller.CreateWallet("brave", base::DoNothing::Once<const std::string&>());
  base::RunLoop().RunUntilIdle();
  std::string salt = GetStringPrefForKeyring(kPasswordEncryptorSalt, "default");
  std::string encrypted_mnemonic =
      GetStringPrefForKeyring(kEncryptedMnemonic, "default");
  std::string nonce =
      GetStringPrefForKeyring(kPasswordEncryptorNonce, "default");
  const std::string mnemonic = controller.GetMnemonicForDefaultKeyringImpl();

  // Restore with same mnemonic and same password
  EXPECT_NE(controller.RestoreDefaultKeyring(mnemonic, "brave"), nullptr);
  EXPECT_EQ(GetStringPrefForKeyring(kEncryptedMnemonic, "default"),
            encrypted_mnemonic);
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorSalt, "default"), salt);
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorNonce, "default"), nonce);
  EXPECT_EQ(controller.default_keyring_->GetAccountsNumber(), 1u);

  // Restore with same mnemonic but different password
  EXPECT_NE(controller.RestoreDefaultKeyring(mnemonic, "brave377"), nullptr);
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
  EXPECT_EQ(controller.RestoreDefaultKeyring("", "brave"), nullptr);
  // Keyring prefs won't be cleared
  EXPECT_EQ(GetStringPrefForKeyring(kEncryptedMnemonic, "default"),
            encrypted_mnemonic);
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorSalt, "default"), salt);
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorNonce, "default"), nonce);
  EXPECT_EQ(controller.default_keyring_->GetAccountsNumber(), 0u);

  // Restore with same mnemonic but empty password
  EXPECT_EQ(controller.RestoreDefaultKeyring(mnemonic, ""), nullptr);
  // Keyring prefs won't be cleared
  EXPECT_EQ(GetStringPrefForKeyring(kEncryptedMnemonic, "default"),
            encrypted_mnemonic);
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorSalt, "default"), salt);
  EXPECT_EQ(GetStringPrefForKeyring(kPasswordEncryptorNonce, "default"), nonce);
  EXPECT_EQ(controller.default_keyring_->GetAccountsNumber(), 0u);

  const std::string mnemonic2 =
      "divide cruise upon flag harsh carbon filter merit once advice bright "
      "drive";
  // default keyring will be overwritten by new seed which will be encrypted by
  // new key even though the passphrase is same.
  EXPECT_NE(controller.RestoreDefaultKeyring(mnemonic2, "brave"), nullptr);
  EXPECT_NE(GetStringPrefForKeyring(kEncryptedMnemonic, "default"),
            encrypted_mnemonic);
  // salt is regenerated and account num is cleared
  EXPECT_NE(GetStringPrefForKeyring(kPasswordEncryptorSalt, "default"), salt);
  EXPECT_NE(GetStringPrefForKeyring(kPasswordEncryptorNonce, "default"), nonce);
  controller.AddAccount("Account 1", base::DoNothing::Once<bool>());
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
    controller.CreateWallet("brave",
                            base::DoNothing::Once<const std::string&>());
    base::RunLoop().RunUntilIdle();
    controller.AddAccount("Account2", base::DoNothing::Once<bool>());
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
  const std::string mnemonic =
      "divide cruise upon flag harsh carbon filter merit once advice bright "
      "drive";
  KeyringController controller(GetPrefs());
  ASSERT_TRUE(controller.CreateEncryptorForKeyring("brave", "default"));

  // no pref exists yet
  controller.GetMnemonicForDefaultKeyring(base::BindOnce(
      &KeyringControllerUnitTest::GetStringCallback, base::Unretained(this)));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(string_value().empty());

  ASSERT_TRUE(controller.CreateDefaultKeyringInternal(mnemonic));
  controller.GetMnemonicForDefaultKeyring(base::BindOnce(
      &KeyringControllerUnitTest::GetStringCallback, base::Unretained(this)));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(string_value(), mnemonic);

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
  EXPECT_EQ(string_value(), mnemonic);
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

  controller.CreateWallet("brave", base::DoNothing::Once<const std::string&>());
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
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  controller.NotifyWalletBackupComplete();
  controller.AddAccount("Account5566", base::DoNothing::Once<bool>());
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

  controller.Reset();
  EXPECT_FALSE(HasPrefForKeyring(kPasswordEncryptorSalt, "default"));
  EXPECT_FALSE(HasPrefForKeyring(kPasswordEncryptorNonce, "default"));
  EXPECT_FALSE(HasPrefForKeyring(kEncryptedMnemonic, "default"));
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletKeyrings));
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
  const std::string mnemonic =
      "divide cruise upon flag harsh carbon filter merit once advice bright "
      "drive";
  EXPECT_TRUE(controller.CreateEncryptorForKeyring("brave", "default"));
  ASSERT_TRUE(controller.CreateDefaultKeyringInternal(mnemonic));
  controller.default_keyring_->AddAccounts(2);
  const std::string address1 = controller.default_keyring_->GetAddress(0);
  const std::string name1 = "Account1";
  const std::string account_path1 = KeyringController::GetAccountPathByIndex(0);
  const std::string address2 = controller.default_keyring_->GetAddress(1);
  const std::string name2 = "Account2";
  const std::string account_path2 = KeyringController::GetAccountPathByIndex(1);

  KeyringController::SetAccountNameForKeyring(GetPrefs(), account_path1, name1,
                                              "default");
  KeyringController::SetAccountNameForKeyring(GetPrefs(), account_path2, name2,
                                              "default");

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
  EXPECT_EQ(KeyringController::GetAccountNameForKeyring(
                GetPrefs(), account_path2, "default"),
            name2);
  EXPECT_EQ(controller.GetAccountMetasNumberForKeyring("default"), 2u);
  EXPECT_EQ(controller.GetAccountMetasNumberForKeyring("keyring1"), 0u);

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
  callback_called = false;
  controller.RestoreWallet(mnemonic_to_be_restored, "brave1",
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
}

TEST_F(KeyringControllerUnitTest, AddAccount) {
  KeyringController controller(GetPrefs());
  controller.CreateWallet("brave", base::DoNothing::Once<const std::string&>());
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

}  // namespace brave_wallet
