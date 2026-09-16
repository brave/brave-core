/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/password_manager/android/brave_account_to_profile_password_migration.h"

#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/components/brave_sync/features.h"
#include "chrome/browser/password_manager/factories/profile_password_store_factory.h"
#include "chrome/browser/password_manager/password_manager_test_util.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/password_manager/core/browser/password_form.h"
#include "components/password_manager/core/browser/password_manager_test_utils.h"
#include "components/password_manager/core/browser/password_store/password_form_converters.h"
#include "components/password_manager/core/browser/password_store/password_store_backend_error.h"
#include "components/password_manager/core/browser/password_store/password_store_consumer.h"
#include "components/password_manager/core/browser/password_store/test_password_store.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_password_manager {

namespace {

using password_manager::PasswordForm;
using password_manager::PasswordStoreBackendError;
using password_manager::PasswordStoreBackendErrorType;
using password_manager::PasswordStoreConsumer;
using password_manager::TestPasswordStore;

// A store whose reads fail while `fail_reads_` is set. Writes keep working, so
// a caller that mistakes a failed read for an empty store can still do damage
// and be caught. TestPasswordStore::ReturnErrorOnRequest fails the writes too,
// which would mask exactly that.
class FailingReadPasswordStore : public TestPasswordStore {
 public:
  FailingReadPasswordStore() = default;

  void set_fail_reads(bool fail_reads) { fail_reads_ = fail_reads; }

  void GetAllLogins(base::WeakPtr<PasswordStoreConsumer> consumer) override {
    if (!fail_reads_) {
      TestPasswordStore::GetAllLogins(std::move(consumer));
      return;
    }
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&FailingReadPasswordStore::ReplyWithError,
                       base::WrapRefCounted(this), std::move(consumer)));
  }

 protected:
  ~FailingReadPasswordStore() override = default;

 private:
  void ReplyWithError(base::WeakPtr<PasswordStoreConsumer> consumer) {
    if (consumer) {
      consumer->OnGetPasswordStoreResultsOrErrorFrom(
          this, PasswordStoreBackendError(
                    PasswordStoreBackendErrorType::kUncategorized));
    }
  }

  bool fail_reads_ = false;
};

}  // namespace

class BraveAccountToProfilePasswordMigrationTest : public ::testing::Test {
 protected:
  BraveAccountToProfilePasswordMigrationTest()
      : testing_profile_manager_(TestingBrowserProcess::GetGlobal()) {}

  void SetUp() override {
    ASSERT_TRUE(testing_profile_manager_.SetUp());
    profile_ = testing_profile_manager_.CreateTestingProfile("TestProfile");
    profile_store_ = CreateProfileStore();
    account_store_ = CreateAndUseTestAccountPasswordStore(profile_);
    profile_store_->Init();
    account_store_->Init();
  }

  virtual scoped_refptr<TestPasswordStore> CreateProfileStore() {
    return CreateAndUseTestPasswordStore(profile_);
  }

  void TearDown() override {
    account_store_->ShutdownOnUIThread();
    profile_store_->ShutdownOnUIThread();
  }

  void EnableFeature() {
    feature_list_.InitAndEnableFeature(
        brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore);
  }

  void DisableFeature() {
    feature_list_.InitAndDisableFeature(
        brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore);
  }

  PasswordForm MakeForm(const std::string& signon_realm,
                        const std::string& username,
                        const std::string& password,
                        bool blocked = false,
                        base::Time modified = base::Time()) {
    PasswordForm form;
    form.url = GURL(signon_realm);
    form.signon_realm = signon_realm;
    form.username_value = base::ASCIIToUTF16(username);
    form.password_value = base::ASCIIToUTF16(password);
    form.blocked_by_user = blocked;
    form.date_created = modified;
    form.date_password_modified = modified;
    return form;
  }

  void AddLogin(TestPasswordStore* store, const PasswordForm& form) {
    base::test::TestFuture<void> future;
    store->AddLogin(password_manager::FromPasswordForm(form),
                    future.GetCallback());
    EXPECT_TRUE(future.Wait());
  }

  std::vector<PasswordForm> GetLogins(TestPasswordStore* store) {
    std::vector<PasswordForm> logins;
    for (const auto& [realm, forms] :
         password_manager::GetAllLoginsSync(store)) {
      logins.insert(logins.end(), forms.begin(), forms.end());
    }
    return logins;
  }

  void RunMigration() {
    base::test::TestFuture<void> future;
    MaybeMigrateAccountPasswordsToProfileStore(profile_, future.GetCallback());
    EXPECT_TRUE(future.Wait());
  }

  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  TestingProfileManager testing_profile_manager_;
  raw_ptr<TestingProfile> profile_ = nullptr;
  scoped_refptr<TestPasswordStore> profile_store_;
  scoped_refptr<TestPasswordStore> account_store_;
};

TEST_F(BraveAccountToProfilePasswordMigrationTest, MigratesAccountToProfile) {
  EnableFeature();
  AddLogin(account_store_.get(), MakeForm("https://a.com/", "u1", "p1"));
  AddLogin(account_store_.get(), MakeForm("https://b.com/", "u2", "p2"));

  RunMigration();

  EXPECT_EQ(2u, GetLogins(profile_store_.get()).size());
  EXPECT_EQ(0u, GetLogins(account_store_.get()).size());
}

TEST_F(BraveAccountToProfilePasswordMigrationTest, MigratesBlocklistEntry) {
  EnableFeature();
  AddLogin(account_store_.get(),
           MakeForm("https://blocked.com/", "", "", /*blocked=*/true));

  RunMigration();

  std::vector<PasswordForm> profile_logins = GetLogins(profile_store_.get());
  ASSERT_EQ(1u, profile_logins.size());
  EXPECT_TRUE(profile_logins[0].blocked_by_user);
  EXPECT_EQ(0u, GetLogins(account_store_.get()).size());
}

TEST_F(BraveAccountToProfilePasswordMigrationTest, NewerAccountPasswordWins) {
  EnableFeature();
  const base::Time older = base::Time::Now();
  const base::Time newer = older + base::Hours(1);
  AddLogin(profile_store_.get(),
           MakeForm("https://a.com/", "u", "old", /*blocked=*/false, older));
  AddLogin(account_store_.get(),
           MakeForm("https://a.com/", "u", "new", /*blocked=*/false, newer));

  RunMigration();

  std::vector<PasswordForm> profile_logins = GetLogins(profile_store_.get());
  ASSERT_EQ(1u, profile_logins.size());
  EXPECT_EQ(u"new", profile_logins[0].password_value);
  EXPECT_EQ(0u, GetLogins(account_store_.get()).size());
}

TEST_F(BraveAccountToProfilePasswordMigrationTest,
       OlderAccountPasswordDoesNotOverwrite) {
  EnableFeature();
  const base::Time older = base::Time::Now();
  const base::Time newer = older + base::Hours(1);
  AddLogin(profile_store_.get(), MakeForm("https://a.com/", "u", "profilenew",
                                          /*blocked=*/false, newer));
  AddLogin(account_store_.get(), MakeForm("https://a.com/", "u", "accountold",
                                          /*blocked=*/false, older));

  RunMigration();

  std::vector<PasswordForm> profile_logins = GetLogins(profile_store_.get());
  ASSERT_EQ(1u, profile_logins.size());
  EXPECT_EQ(u"profilenew", profile_logins[0].password_value);
  EXPECT_EQ(0u, GetLogins(account_store_.get()).size());
}

TEST_F(BraveAccountToProfilePasswordMigrationTest, IdenticalCredentialDedups) {
  EnableFeature();
  AddLogin(profile_store_.get(), MakeForm("https://a.com/", "u", "same"));
  AddLogin(account_store_.get(), MakeForm("https://a.com/", "u", "same"));

  RunMigration();

  EXPECT_EQ(1u, GetLogins(profile_store_.get()).size());
  EXPECT_EQ(0u, GetLogins(account_store_.get()).size());
}

TEST_F(BraveAccountToProfilePasswordMigrationTest, NoOpWhenAccountEmpty) {
  EnableFeature();
  AddLogin(profile_store_.get(), MakeForm("https://a.com/", "u", "p"));

  RunMigration();

  EXPECT_EQ(1u, GetLogins(profile_store_.get()).size());
  EXPECT_EQ(0u, GetLogins(account_store_.get()).size());
}

class BraveAccountToProfilePasswordMigrationReadErrorTest
    : public BraveAccountToProfilePasswordMigrationTest {
 protected:
  scoped_refptr<TestPasswordStore> CreateProfileStore() override {
    failing_profile_store_ =
        base::WrapRefCounted(static_cast<FailingReadPasswordStore*>(
            ProfilePasswordStoreFactory::GetInstance()
                ->SetTestingFactoryAndUse(
                    profile_,
                    base::BindRepeating(
                        &password_manager::BuildPasswordStore<
                            content::BrowserContext, FailingReadPasswordStore>))
                .get()));
    return failing_profile_store_;
  }

  scoped_refptr<FailingReadPasswordStore> failing_profile_store_;
};

TEST_F(BraveAccountToProfilePasswordMigrationReadErrorTest,
       ProfileStoreReadErrorAbortsMigration) {
  EnableFeature();
  const base::Time older = base::Time::Now();
  const base::Time newer = older + base::Hours(1);
  AddLogin(profile_store_.get(), MakeForm("https://a.com/", "u", "profilenew",
                                          /*blocked=*/false, newer));
  AddLogin(account_store_.get(), MakeForm("https://a.com/", "u", "accountold",
                                          /*blocked=*/false, older));

  failing_profile_store_->set_fail_reads(true);
  RunMigration();
  // Reads have to work again for the assertions below.
  failing_profile_store_->set_fail_reads(false);

  // The failed read must not pass for an empty profile store: the older account
  // password must not overwrite the newer profile one, and the account store
  // must keep its credential so the next launch can retry.
  std::vector<PasswordForm> profile_logins = GetLogins(profile_store_.get());
  ASSERT_EQ(1u, profile_logins.size());
  EXPECT_EQ(u"profilenew", profile_logins[0].password_value);
  EXPECT_EQ(1u, GetLogins(account_store_.get()).size());
}

TEST_F(BraveAccountToProfilePasswordMigrationTest,
       DoesNotMigrateWhenFlagDisabled) {
  DisableFeature();
  AddLogin(account_store_.get(), MakeForm("https://a.com/", "u1", "p1"));
  AddLogin(account_store_.get(), MakeForm("https://b.com/", "u2", "p2"));

  RunMigration();

  EXPECT_EQ(0u, GetLogins(profile_store_.get()).size());
  EXPECT_EQ(2u, GetLogins(account_store_.get()).size());
}

}  // namespace brave_password_manager
