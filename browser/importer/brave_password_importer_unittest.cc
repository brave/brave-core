/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_password_importer.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/test_future.h"
#include "chrome/browser/password_manager/factories/profile_password_store_factory.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "components/password_manager/core/browser/password_manager_test_utils.h"
#include "components/password_manager/core/browser/password_store/login_database.h"
#include "components/password_manager/core/browser/password_store/password_store_interface.h"
#include "components/password_manager/core/browser/password_store/stored_credential.h"
#include "components/password_manager/core/browser/password_store/test_password_store.h"
#include "content/public/browser/browser_context.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

// Blocks until `g_browser_process->os_crypt_async()` produces an Encryptor.
scoped_refptr<os_crypt_async::Encryptor> GetEncryptorSync() {
  base::test::TestFuture<scoped_refptr<os_crypt_async::Encryptor>> future;
  TestingBrowserProcess::GetGlobal()->os_crypt_async()->GetInstance(
      future.GetCallback());
  return future.Take();
}

password_manager::StoredCredential MakeCredential(
    const std::string& signon_realm,
    const std::u16string& username,
    const std::u16string& password) {
  password_manager::StoredCredential cred;
  cred.url = GURL(signon_realm);
  cred.signon_realm = signon_realm;
  cred.username_value = username;
  cred.password_value = password;
  cred.in_store = password_manager::PasswordForm::Store::kProfileStore;
  return cred;
}

}  // namespace

class BravePasswordImporterTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    // The source profile directory holds the `Login Data` file we will import
    // from.
    source_path_ = temp_dir_.GetPath().AppendASCII("source");
    ASSERT_TRUE(base::CreateDirectory(source_path_));

    profile_ = TestingProfile::Builder().Build();
    ProfilePasswordStoreFactory::GetInstance()->SetTestingFactoryAndUse(
        profile_.get(),
        base::BindRepeating(
            &password_manager::BuildPasswordStore<
                content::BrowserContext, password_manager::TestPasswordStore>));
    ASSERT_TRUE(password_store());
  }

  // Fetches the store from the factory on demand rather than caching a raw
  // pointer. The store is a RefcountedKeyedService owned by the profile; the
  // factory keeps it alive, and letting `profile_` destruct before
  // `task_environment_` drains its backend without a dangling pointer.
  password_manager::TestPasswordStore* password_store() {
    return static_cast<password_manager::TestPasswordStore*>(
        ProfilePasswordStoreFactory::GetForProfile(
            profile_.get(), ServiceAccessType::EXPLICIT_ACCESS)
            .get());
  }

  // Writes `credentials` into a `Login Data` SQLite store inside the source
  // profile directory. By default it encrypts with the same OSCryptAsync
  // instance the importer will later decrypt with; pass a different
  // `encryptor` to simulate a key mismatch between the source and destination
  // installations.
  void WriteLoginData(
      const std::vector<password_manager::StoredCredential>& credentials,
      scoped_refptr<os_crypt_async::Encryptor> encryptor = nullptr) {
    if (!encryptor) {
      encryptor = GetEncryptorSync();
    }
    base::FilePath login_data_path =
        source_path_.Append(FILE_PATH_LITERAL("Login Data"));
    password_manager::LoginDatabase database(
        login_data_path, password_manager::IsAccountStore(false));
    ASSERT_TRUE(database.Init(base::NullCallback(), std::move(encryptor)));
    for (const auto& cred : credentials) {
      ASSERT_FALSE(
          database.AddLogin(password_manager::CloneStoredCredential(cred))
              .empty());
    }
  }

  // Runs an import and returns the (result, submitted-count) pair.
  std::pair<BravePasswordImporter::Result, size_t> RunImport() {
    BravePasswordImporter importer;
    base::test::TestFuture<BravePasswordImporter::Result, size_t> future;
    importer.StartImport(source_path_, profile_.get(), future.GetCallback());
    return {future.Get<0>(), future.Get<1>()};
  }

  content::BrowserTaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  base::FilePath source_path_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(BravePasswordImporterTest, ImportsAutofillableCredentials) {
  std::vector<password_manager::StoredCredential> credentials;
  credentials.push_back(
      MakeCredential("https://example.com/", u"alice", u"secret1"));
  credentials.push_back(
      MakeCredential("https://test.org/", u"bob", u"secret2"));
  ASSERT_NO_FATAL_FAILURE(WriteLoginData(credentials));

  EXPECT_EQ(RunImport(),
            std::make_pair(BravePasswordImporter::Result::kSuccess, 2u));

  auto stored = password_manager::GetAllLoginsSync(password_store());
  EXPECT_EQ(2u, stored.size());
  ASSERT_TRUE(stored.contains("https://example.com/"));
  EXPECT_EQ(u"alice", stored.at("https://example.com/")[0].username_value);
  EXPECT_EQ(u"secret1", stored.at("https://example.com/")[0].password_value);
  ASSERT_TRUE(stored.contains("https://test.org/"));
  EXPECT_EQ(u"bob", stored.at("https://test.org/")[0].username_value);
}

TEST_F(BravePasswordImporterTest, ImportsBlocklistedEntries) {
  password_manager::StoredCredential blocked = MakeCredential(
      "https://noprompt.com/", std::u16string(), std::u16string());
  blocked.blocked_by_user = true;
  std::vector<password_manager::StoredCredential> credentials;
  credentials.push_back(std::move(blocked));
  WriteLoginData(credentials);

  EXPECT_EQ(RunImport(),
            std::make_pair(BravePasswordImporter::Result::kSuccess, 1u));
  EXPECT_TRUE(password_manager::GetAllLoginsSync(password_store())
                  .contains("https://noprompt.com/"));
}

// A blocklisted entry in the source `Login Data` may carry a username and/or
// password, but PasswordStore::AddLogins CHECKs that blocklisted entries have
// neither. The importer must normalize them to empty before adding.
TEST_F(BravePasswordImporterTest, BlocklistedEntryWithCredentialsIsNormalized) {
  password_manager::StoredCredential blocked =
      MakeCredential("https://noprompt.com/", u"leftover", u"leftover");
  blocked.blocked_by_user = true;
  std::vector<password_manager::StoredCredential> credentials;
  credentials.push_back(std::move(blocked));
  WriteLoginData(credentials);

  // Must not crash the AddLogins CHECK, and the stored blocklist entry must
  // have empty username/password.
  EXPECT_EQ(RunImport(),
            std::make_pair(BravePasswordImporter::Result::kSuccess, 1u));
  auto stored = password_manager::GetAllLoginsSync(password_store());
  ASSERT_TRUE(stored.contains("https://noprompt.com/"));
  const auto& entry = stored.at("https://noprompt.com/")[0];
  EXPECT_TRUE(entry.blocked_by_user);
  EXPECT_TRUE(entry.username_value.empty());
  EXPECT_TRUE(entry.password_value.empty());
}

TEST_F(BravePasswordImporterTest, MissingLoginDataReportsNotFound) {
  // No `Login Data` file was written to the source path.
  EXPECT_EQ(
      RunImport(),
      std::make_pair(BravePasswordImporter::Result::kLoginDataNotFound, 0u));
  EXPECT_TRUE(password_manager::GetAllLoginsSync(password_store()).empty());
}

TEST_F(BravePasswordImporterTest, EmptyLoginDataReportsZero) {
  WriteLoginData({});
  EXPECT_EQ(RunImport(),
            std::make_pair(BravePasswordImporter::Result::kSuccess, 0u));
  EXPECT_TRUE(password_manager::GetAllLoginsSync(password_store()).empty());
}

// When the source `Login Data` was encrypted with a different key than the
// destination's (e.g. a keychain item was deleted and regenerated, or the
// profile was copied from another machine), decryption must fail and no
// credentials may leak into the destination store.
TEST_F(BravePasswordImporterTest, KeyMismatchImportsNothing) {
  std::vector<password_manager::StoredCredential> credentials;
  credentials.push_back(
      MakeCredential("https://example.com/", u"alice", u"secret1"));
  // A fresh test Encryptor uses a random key distinct from the destination's.
  ASSERT_NO_FATAL_FAILURE(WriteLoginData(
      credentials, os_crypt_async::GetTestEncryptorForTesting()));

  auto [result, submitted] = RunImport();
  // The undecryptable row is dropped on read, so the import succeeds with
  // nothing decrypted and no credential is submitted to or leaked into the
  // store.
  EXPECT_EQ(BravePasswordImporter::Result::kSuccess, result);
  EXPECT_EQ(0u, submitted);
  EXPECT_TRUE(password_manager::GetAllLoginsSync(password_store()).empty());
}
