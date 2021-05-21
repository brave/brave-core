/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/keyring_controller.h"

#include "base/base64.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class KeyringControllerUnitTest : public testing::Test {
 public:
  KeyringControllerUnitTest()
      : testing_profile_manager_(TestingBrowserProcess::GetGlobal()) {}
  ~KeyringControllerUnitTest() override {}

 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(testing_profile_manager_.SetUp(temp_dir_.GetPath()));
  }

  PrefService* GetPrefs() {
    return ProfileManager::GetActiveUserProfile()->GetPrefs();
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfileManager testing_profile_manager_;
  base::ScopedTempDir temp_dir_;
};

TEST_F(KeyringControllerUnitTest, GetPrefsInBytes) {
  KeyringController controller(GetPrefs());
  GetPrefs()->SetString(kBraveWalletEncryptedMnemonic, "3q2+7w==");

  auto verify_bytes = [](const std::vector<uint8_t>& bytes) {
    ASSERT_EQ(bytes.size(), 4u);
    EXPECT_EQ(bytes[0], 0xde);
    EXPECT_EQ(bytes[1], 0xad);
    EXPECT_EQ(bytes[2], 0xbe);
    EXPECT_EQ(bytes[3], 0xef);
  };

  std::vector<uint8_t> mnemonic;
  ASSERT_TRUE(
      controller.GetPrefsInBytes(kBraveWalletEncryptedMnemonic, &mnemonic));
  verify_bytes(mnemonic);

  std::vector<uint8_t> mnemonic_fixed(4);
  ASSERT_TRUE(controller.GetPrefsInBytes(kBraveWalletEncryptedMnemonic,
                                         &mnemonic_fixed));
  verify_bytes(mnemonic_fixed);

  std::vector<uint8_t> mnemonic_smaller(2);
  ASSERT_TRUE(controller.GetPrefsInBytes(kBraveWalletEncryptedMnemonic,
                                         &mnemonic_smaller));
  verify_bytes(mnemonic_smaller);

  std::vector<uint8_t> mnemonic_bigger(8);
  ASSERT_TRUE(controller.GetPrefsInBytes(kBraveWalletEncryptedMnemonic,
                                         &mnemonic_bigger));
  verify_bytes(mnemonic_bigger);

  // invalid base64 encoded
  mnemonic.clear();
  GetPrefs()->SetString(kBraveWalletEncryptedMnemonic, "3q2+7w^^");
  EXPECT_FALSE(
      controller.GetPrefsInBytes(kBraveWalletEncryptedMnemonic, &mnemonic));

  // default pref value (empty)
  mnemonic.clear();
  GetPrefs()->ClearPref(kBraveWalletEncryptedMnemonic);
  EXPECT_FALSE(
      controller.GetPrefsInBytes(kBraveWalletEncryptedMnemonic, &mnemonic));

  // bytes is nullptr
  EXPECT_FALSE(
      controller.GetPrefsInBytes(kBraveWalletEncryptedMnemonic, nullptr));

  // non-existing pref
  mnemonic.clear();
  EXPECT_FALSE(controller.GetPrefsInBytes("brave.nothinghere", &mnemonic));

  // non-string pref
  mnemonic.clear();
  GetPrefs()->SetInteger(kBraveWalletDefaultKeyringAccountNum, 123);
  EXPECT_FALSE(controller.GetPrefsInBytes(kBraveWalletDefaultKeyringAccountNum,
                                          &mnemonic));
}

TEST_F(KeyringControllerUnitTest, SetPrefsInBytes) {
  const uint8_t bytes_array[] = {0xde, 0xad, 0xbe, 0xef};
  KeyringController controller(GetPrefs());
  controller.SetPrefsInBytes(kBraveWalletEncryptedMnemonic, bytes_array);
  EXPECT_EQ(GetPrefs()->GetString(kBraveWalletEncryptedMnemonic), "3q2+7w==");

  GetPrefs()->ClearPref(kBraveWalletEncryptedMnemonic);
  const std::vector<uint8_t> bytes_vector = {0xde, 0xad, 0xbe, 0xef};
  controller.SetPrefsInBytes(kBraveWalletEncryptedMnemonic, bytes_vector);
  EXPECT_EQ(GetPrefs()->GetString(kBraveWalletEncryptedMnemonic), "3q2+7w==");
}

TEST_F(KeyringControllerUnitTest, GetOrCreateNonce) {
  std::string encoded_nonce;
  {
    KeyringController controller(GetPrefs());
    const std::vector<uint8_t> nonce = controller.GetOrCreateNonce();
    encoded_nonce = base::Base64Encode(nonce);
    EXPECT_EQ(encoded_nonce,
              GetPrefs()->GetString(kBraveWalletPasswordEncryptorNonce));
  }
  {  // It should be the same nonce as long as the pref exists
    KeyringController controller(GetPrefs());
    const std::vector<uint8_t> nonce = controller.GetOrCreateNonce();
    EXPECT_EQ(base::Base64Encode(nonce), encoded_nonce);
    EXPECT_EQ(encoded_nonce,
              GetPrefs()->GetString(kBraveWalletPasswordEncryptorNonce));
  }
  GetPrefs()->ClearPref(kBraveWalletPasswordEncryptorNonce);
  {  // nonce should be different now
    KeyringController controller(GetPrefs());
    const std::vector<uint8_t> nonce = controller.GetOrCreateNonce();
    EXPECT_NE(base::Base64Encode(nonce), encoded_nonce);
  }
}

TEST_F(KeyringControllerUnitTest, CreateEncryptor) {
  std::string encoded_salt;
  {
    KeyringController controller(GetPrefs());
    EXPECT_TRUE(controller.CreateEncryptor("123"));
    EXPECT_NE(controller.encryptor_, nullptr);
    encoded_salt = GetPrefs()->GetString(kBraveWalletPasswordEncryptorSalt);
    EXPECT_FALSE(encoded_salt.empty());
  }
  {
    KeyringController controller(GetPrefs());
    EXPECT_TRUE(controller.CreateEncryptor("123"));
    EXPECT_EQ(GetPrefs()->GetString(kBraveWalletPasswordEncryptorSalt),
              encoded_salt);
  }
  {
    KeyringController controller(GetPrefs());
    EXPECT_FALSE(controller.CreateEncryptor(""));
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

  EXPECT_TRUE(controller.CreateEncryptor("brave"));
  ASSERT_TRUE(controller.CreateDefaultKeyringInternal(mnemonic1));
  controller.default_keyring_->AddAccounts(1);
  EXPECT_EQ(controller.default_keyring_->GetAddress(0),
            "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db");
  const std::string encrypted_mnemonic1 =
      GetPrefs()->GetString(kBraveWalletEncryptedMnemonic);
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
  EXPECT_NE(GetPrefs()->GetString(kBraveWalletEncryptedMnemonic),
            encrypted_mnemonic1);
}

TEST_F(KeyringControllerUnitTest, CreateDefaultKeyring) {
  std::string salt;
  std::string nonce;
  std::string mnemonic;
  {
    KeyringController controller(GetPrefs());
    EXPECT_EQ(controller.CreateDefaultKeyring(""), nullptr);
    EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletPasswordEncryptorSalt));
    EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletPasswordEncryptorNonce));
    EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletEncryptedMnemonic));
    HDKeyring* keyring = controller.CreateDefaultKeyring("brave1");
    EXPECT_EQ(keyring->type(), HDKeyring::Type::kDefault);
    keyring->AddAccounts(1);
    const std::string address1 = keyring->GetAddress(0);
    EXPECT_FALSE(address1.empty());
    EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletPasswordEncryptorSalt));
    EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletPasswordEncryptorNonce));
    EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletEncryptedMnemonic));

    // default keyring will be overwritten
    keyring = controller.CreateDefaultKeyring("brave2");
    keyring->AddAccounts(1);
    const std::string address2 = keyring->GetAddress(0);
    EXPECT_FALSE(address2.empty());
    EXPECT_NE(address1, address2);

    salt = GetPrefs()->GetString(kBraveWalletPasswordEncryptorSalt);
    nonce = GetPrefs()->GetString(kBraveWalletPasswordEncryptorNonce);
    mnemonic = GetPrefs()->GetString(kBraveWalletEncryptedMnemonic);
  }

  // mnemonic, salt and account number don't get clear unless Reset() is called
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletPasswordEncryptorSalt));
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletPasswordEncryptorNonce));
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletEncryptedMnemonic));
  EXPECT_EQ(GetPrefs()->GetString(kBraveWalletPasswordEncryptorSalt), salt);
  EXPECT_EQ(GetPrefs()->GetString(kBraveWalletPasswordEncryptorNonce), nonce);
  EXPECT_EQ(GetPrefs()->GetString(kBraveWalletEncryptedMnemonic), mnemonic);
  EXPECT_EQ(GetPrefs()->GetInteger(kBraveWalletDefaultKeyringAccountNum), 1);
}

TEST_F(KeyringControllerUnitTest, RestoreDefaultKeyring) {
  KeyringController controller(GetPrefs());
  HDKeyring* keyring = controller.CreateDefaultKeyring("brave");
  keyring->AddAccounts(1);
  const std::string salt =
      GetPrefs()->GetString(kBraveWalletPasswordEncryptorSalt);
  const std::string mnemonic =
      GetPrefs()->GetString(kBraveWalletEncryptedMnemonic);

  const std::string seed_phrase =
      "divide cruise upon flag harsh carbon filter merit once advice bright "
      "drive";
  // default keyring will be overwritten by new seed which will be encrypted by
  // new key even though the passphrase is same.
  keyring = controller.RestoreDefaultKeyring(seed_phrase, "brave");
  EXPECT_NE(GetPrefs()->GetString(kBraveWalletEncryptedMnemonic), mnemonic);
  // salt is regenerated and account num is cleared
  EXPECT_NE(GetPrefs()->GetString(kBraveWalletPasswordEncryptorSalt), salt);
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletDefaultKeyringAccountNum));
  keyring->AddAccounts(1);
  EXPECT_EQ(keyring->GetAddress(0),
            "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db");

  EXPECT_EQ(controller.RestoreDefaultKeyring(seed_phrase, ""), nullptr);
}

TEST_F(KeyringControllerUnitTest, UnlockResumesDefaultKeyring) {
  std::string salt;
  std::string mnemonic;
  std::string nonce;
  {
    KeyringController controller(GetPrefs());
    HDKeyring* keyring = controller.CreateDefaultKeyring("brave");
    keyring->AddAccounts(2);
    salt = GetPrefs()->GetString(kBraveWalletPasswordEncryptorSalt);
    nonce = GetPrefs()->GetString(kBraveWalletPasswordEncryptorNonce);
    mnemonic = GetPrefs()->GetString(kBraveWalletEncryptedMnemonic);
  }
  {
    // KeyringController is now destructed, simlulating relaunch
    KeyringController controller(GetPrefs());
    ASSERT_TRUE(controller.Unlock("brave"));
    HDKeyring* keyring = controller.GetDefaultKeyring();
    EXPECT_EQ(GetPrefs()->GetString(kBraveWalletPasswordEncryptorSalt), salt);
    EXPECT_EQ(GetPrefs()->GetString(kBraveWalletPasswordEncryptorNonce), nonce);
    EXPECT_EQ(GetPrefs()->GetString(kBraveWalletEncryptedMnemonic), mnemonic);
    ASSERT_NE(keyring, nullptr);
    EXPECT_EQ(keyring->GetAccounts().size(), 2u);
  }
  {
    KeyringController controller(GetPrefs());
    // wrong password
    ASSERT_FALSE(controller.Unlock("brave123"));
    // empty password
    ASSERT_FALSE(controller.Unlock(""));
  }
}

TEST_F(KeyringControllerUnitTest, GetMnemonicForDefaultKeyring) {
  const std::string mnemonic =
      "divide cruise upon flag harsh carbon filter merit once advice bright "
      "drive";
  KeyringController controller(GetPrefs());
  ASSERT_TRUE(controller.CreateEncryptor("brave"));

  // no pref exists yet
  EXPECT_TRUE(controller.GetMnemonicForDefaultKeyring().empty());

  ASSERT_TRUE(controller.CreateDefaultKeyringInternal(mnemonic));
  EXPECT_EQ(controller.GetMnemonicForDefaultKeyring(), mnemonic);

  // Lock controller
  controller.Lock();
  EXPECT_TRUE(controller.GetMnemonicForDefaultKeyring().empty());

  // unlock with wrong password
  EXPECT_FALSE(controller.Unlock("brave123"));
  EXPECT_TRUE(controller.GetMnemonicForDefaultKeyring().empty());

  EXPECT_TRUE(controller.Unlock("brave"));
  EXPECT_EQ(controller.GetMnemonicForDefaultKeyring(), mnemonic);
}

TEST_F(KeyringControllerUnitTest, GetDefaultKeyring) {
  KeyringController controller(GetPrefs());
  EXPECT_EQ(controller.GetDefaultKeyring(), nullptr);
  HDKeyring* keyring = controller.CreateDefaultKeyring("brave");
  ASSERT_NE(keyring, nullptr);
  EXPECT_EQ(controller.GetDefaultKeyring(), keyring);
  keyring->AddAccounts(1);
  const std::string address = keyring->GetAddress(0);

  controller.Lock();
  EXPECT_EQ(controller.GetDefaultKeyring(), nullptr);

  EXPECT_TRUE(controller.Unlock("brave"));
  ASSERT_NE(controller.GetDefaultKeyring(), nullptr);
  EXPECT_EQ(controller.GetDefaultKeyring()->GetAddress(0), address);
}

TEST_F(KeyringControllerUnitTest, LockAndUnlock) {
  {
    KeyringController controller(GetPrefs());
    // No encryptor
    controller.Lock();
    EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletDefaultKeyringAccountNum));
    EXPECT_TRUE(controller.IsLocked());
    EXPECT_TRUE(controller.CreateEncryptor("123"));
    EXPECT_FALSE(controller.IsLocked());
    // No default keyring
    controller.Lock();
    EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletDefaultKeyringAccountNum));
  }
  {
    KeyringController controller(GetPrefs());
    ASSERT_NE(controller.CreateDefaultKeyring("brave"), nullptr);
    controller.default_keyring_->AddAccounts(1);
    EXPECT_FALSE(controller.IsLocked());
    EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletDefaultKeyringAccountNum));

    controller.Lock();
    EXPECT_TRUE(controller.IsLocked());
    EXPECT_EQ(GetPrefs()->GetInteger(kBraveWalletDefaultKeyringAccountNum), 1);
    EXPECT_TRUE(controller.default_keyring_->empty());

    EXPECT_FALSE(controller.Unlock("abc"));
    EXPECT_TRUE(controller.IsLocked());

    EXPECT_TRUE(controller.Unlock("brave"));
    EXPECT_FALSE(controller.IsLocked());
    controller.default_keyring_->AddAccounts(1);

    controller.Lock();
    EXPECT_TRUE(controller.IsLocked());
    EXPECT_EQ(GetPrefs()->GetInteger(kBraveWalletDefaultKeyringAccountNum), 2);
    EXPECT_TRUE(controller.default_keyring_->empty());

    // Simulate unlock shutdown
    EXPECT_TRUE(controller.Unlock("brave"));
    controller.default_keyring_->AddAccounts(1);
  }
  EXPECT_EQ(GetPrefs()->GetInteger(kBraveWalletDefaultKeyringAccountNum), 3);
}

TEST_F(KeyringControllerUnitTest, Reset) {
  KeyringController controller(GetPrefs());
  HDKeyring* keyring = controller.CreateDefaultKeyring("brave");
  keyring->AddAccounts();
  // Trigger account number saving
  controller.Lock();

  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletPasswordEncryptorSalt));
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletPasswordEncryptorNonce));
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletEncryptedMnemonic));
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletDefaultKeyringAccountNum));

  controller.Reset();
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletPasswordEncryptorSalt));
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletPasswordEncryptorNonce));
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletEncryptedMnemonic));
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletDefaultKeyringAccountNum));
  EXPECT_EQ(controller.default_keyring_, nullptr);
  EXPECT_EQ(controller.encryptor_, nullptr);
}

}  // namespace brave_wallet
