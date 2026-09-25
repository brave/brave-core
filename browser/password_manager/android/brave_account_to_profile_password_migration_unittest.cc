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
#include "chrome/browser/password_manager/factories/account_password_store_factory.h"
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
#include "components/password_manager/core/browser/password_store/password_store_interface.h"
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
using password_manager::StoredCredential;
using password_manager::TestPasswordStore;

// A store whose reads and writes fail independently, so a read failure can be
// simulated while writes still work and vice versa.
// TestPasswordStore::ReturnErrorOnRequest fails both at once and cannot be
// turned off again, which would mask the damage a caller does after
// misreading a failure as an empty store.
class FailingPasswordStore : public TestPasswordStore {
 public:
  explicit FailingPasswordStore(
      password_manager::IsAccountStore is_account_store =
          password_manager::IsAccountStore(false))
      : TestPasswordStore(is_account_store) {}

  void set_fail_reads(bool fail_reads) { fail_reads_ = fail_reads; }
  void set_fail_writes(bool fail_writes) { fail_writes_ = fail_writes; }

  // Starts failing reads as soon as a write has landed. That is how a caller
  // which re-reads to confirm its own writes hits an error at the verify step
  // and not before, without the test having to count reads.
  void set_fail_reads_after_write(bool fail_reads_after_write) {
    fail_reads_after_write_ = fail_reads_after_write;
  }

  void GetAllLogins(base::WeakPtr<PasswordStoreConsumer> consumer) override {
    if (!fail_reads_) {
      TestPasswordStore::GetAllLogins(std::move(consumer));
      return;
    }
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&FailingPasswordStore::ReplyWithError,
                       base::WrapRefCounted(this), std::move(consumer)));
  }

  void AddLogins(std::vector<StoredCredential> forms,
                 base::OnceClosure completion) override {
    if (!fail_writes_) {
      TestPasswordStore::AddLogins(std::move(forms),
                                   WrapWriteCompletion(std::move(completion)));
      return;
    }
    DropWrite(std::move(completion));
  }

  void UpdateLogins(std::vector<StoredCredential> forms,
                    base::OnceClosure completion) override {
    if (!fail_writes_) {
      TestPasswordStore::UpdateLogins(
          std::move(forms), WrapWriteCompletion(std::move(completion)));
      return;
    }
    DropWrite(std::move(completion));
  }

 protected:
  ~FailingPasswordStore() override = default;

 private:
  void ReplyWithError(base::WeakPtr<PasswordStoreConsumer> consumer) {
    if (consumer) {
      consumer->OnGetPasswordStoreResultsOrErrorFrom(
          this, PasswordStoreBackendError(
                    PasswordStoreBackendErrorType::kUncategorized));
    }
  }

  // PasswordStore joins a backend error into the completion callback rather
  // than dropping it, so a caller sees a failed write as a finished one.
  void DropWrite(base::OnceClosure completion) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, std::move(completion));
  }

  // Flips `fail_reads_` before handing control back, so the read the caller
  // issues from its completion callback is the first one to fail.
  base::OnceClosure WrapWriteCompletion(base::OnceClosure completion) {
    if (!fail_reads_after_write_) {
      return completion;
    }
    return base::BindOnce(&FailingPasswordStore::FailReadsThen,
                          base::WrapRefCounted(this), std::move(completion));
  }

  void FailReadsThen(base::OnceClosure completion) {
    fail_reads_ = true;
    std::move(completion).Run();
  }

  bool fail_reads_ = false;
  bool fail_writes_ = false;
  bool fail_reads_after_write_ = false;
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
    account_store_ = CreateAccountStore();
    profile_store_->Init();
    account_store_->Init();
  }

  virtual scoped_refptr<TestPasswordStore> CreateProfileStore() {
    return CreateAndUseTestPasswordStore(profile_);
  }

  virtual scoped_refptr<TestPasswordStore> CreateAccountStore() {
    return CreateAndUseTestAccountPasswordStore(profile_);
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

TEST_F(BraveAccountToProfilePasswordMigrationTest,
       ConcurrentCallDoesNotStartSecondMigration) {
  EnableFeature();
  AddLogin(account_store_.get(), MakeForm("https://a.com/", "u1", "p1"));
  AddLogin(account_store_.get(), MakeForm("https://b.com/", "u2", "p2"));

  base::test::TestFuture<void> first;
  base::test::TestFuture<void> second;
  MaybeMigrateAccountPasswordsToProfileStore(profile_, first.GetCallback());
  // The profile already owns a running migrator, so this call must not start a
  // second one racing it over the same stores.
  MaybeMigrateAccountPasswordsToProfileStore(profile_, second.GetCallback());

  EXPECT_TRUE(first.Wait());
  EXPECT_TRUE(second.Wait());
  EXPECT_EQ(2u, GetLogins(profile_store_.get()).size());
  EXPECT_EQ(0u, GetLogins(account_store_.get()).size());
}

TEST_F(BraveAccountToProfilePasswordMigrationTest,
       MigrationCanRunAgainAfterFinishing) {
  EnableFeature();
  AddLogin(account_store_.get(), MakeForm("https://a.com/", "u1", "p1"));

  RunMigration();
  ASSERT_EQ(1u, GetLogins(profile_store_.get()).size());

  // The finished migration must have released its claim on the profile,
  // otherwise this one would be skipped as a duplicate.
  AddLogin(account_store_.get(), MakeForm("https://b.com/", "u2", "p2"));
  RunMigration();

  EXPECT_EQ(2u, GetLogins(profile_store_.get()).size());
  EXPECT_EQ(0u, GetLogins(account_store_.get()).size());
}

TEST_F(BraveAccountToProfilePasswordMigrationTest,
       StoreShutdownStopsMigrationWithoutCompleting) {
  EnableFeature();
  AddLogin(account_store_.get(), MakeForm("https://a.com/", "u", "p"));
  // Every PasswordStore method returns early once shutdown started, dropping
  // the request without running its callback.
  account_store_->ShutdownOnUIThread();

  base::test::TestFuture<void> future;
  MaybeMigrateAccountPasswordsToProfileStore(profile_, future.GetCallback());

  // The stopped migrator is still parked on the profile, which a second call
  // observes by short-circuiting instead of starting its own migration. Waiting
  // for that completion also gives the stopped chain the chance to run anything
  // it might still have queued, before the checks below.
  base::test::TestFuture<void> probe;
  MaybeMigrateAccountPasswordsToProfileStore(profile_, probe.GetCallback());
  ASSERT_TRUE(probe.Wait());

  // Nothing was copied, and the chain stops where the dropped callback would
  // have continued it. What keeps the migrator and the credentials it holds
  // from living until process exit is the profile owning it, not this callback
  // running.
  EXPECT_EQ(0u, GetLogins(profile_store_.get()).size());
  EXPECT_FALSE(future.IsReady());
}

class BraveAccountToProfilePasswordMigrationStoreErrorTest
    : public BraveAccountToProfilePasswordMigrationTest {
 protected:
  scoped_refptr<TestPasswordStore> CreateProfileStore() override {
    failing_profile_store_ =
        base::WrapRefCounted(static_cast<FailingPasswordStore*>(
            ProfilePasswordStoreFactory::GetInstance()
                ->SetTestingFactoryAndUse(
                    profile_,
                    base::BindRepeating(
                        &password_manager::BuildPasswordStore<
                            content::BrowserContext, FailingPasswordStore>))
                .get()));
    return failing_profile_store_;
  }

  scoped_refptr<TestPasswordStore> CreateAccountStore() override {
    failing_account_store_ =
        base::WrapRefCounted(static_cast<FailingPasswordStore*>(
            AccountPasswordStoreFactory::GetInstance()
                ->SetTestingFactoryAndUse(
                    profile_,
                    base::BindRepeating(
                        &password_manager::BuildPasswordStoreWithArgs<
                            content::BrowserContext, FailingPasswordStore,
                            password_manager::IsAccountStore>,
                        password_manager::IsAccountStore(true)))
                .get()));
    return failing_account_store_;
  }

  scoped_refptr<FailingPasswordStore> failing_profile_store_;
  scoped_refptr<FailingPasswordStore> failing_account_store_;
};

TEST_F(BraveAccountToProfilePasswordMigrationStoreErrorTest,
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

TEST_F(BraveAccountToProfilePasswordMigrationStoreErrorTest,
       AccountStoreReadErrorLeavesBothStoresIntact) {
  EnableFeature();
  AddLogin(profile_store_.get(), MakeForm("https://a.com/", "u1", "p1"));
  AddLogin(account_store_.get(), MakeForm("https://b.com/", "u2", "p2"));

  failing_account_store_->set_fail_reads(true);
  RunMigration();
  failing_account_store_->set_fail_reads(false);

  // Nothing is known about the account store, so nothing may be written or
  // removed; the next launch retries.
  EXPECT_EQ(1u, GetLogins(profile_store_.get()).size());
  EXPECT_EQ(1u, GetLogins(account_store_.get()).size());
}

TEST_F(BraveAccountToProfilePasswordMigrationStoreErrorTest,
       ProfileStoreAddFailureKeepsAccountCredential) {
  EnableFeature();
  AddLogin(account_store_.get(), MakeForm("https://a.com/", "u", "p"));

  failing_profile_store_->set_fail_writes(true);
  RunMigration();
  failing_profile_store_->set_fail_writes(false);

  // The credential never reached the profile store, so the account store must
  // keep it for the next launch instead of losing it.
  EXPECT_EQ(0u, GetLogins(profile_store_.get()).size());
  EXPECT_EQ(1u, GetLogins(account_store_.get()).size());
}

TEST_F(BraveAccountToProfilePasswordMigrationStoreErrorTest,
       ProfileStoreVerifyReadErrorKeepsAccountCredential) {
  EnableFeature();
  AddLogin(account_store_.get(), MakeForm("https://a.com/", "u", "p"));

  // The copy into the profile store lands, but the read that would confirm it
  // fails.
  failing_profile_store_->set_fail_reads_after_write(true);
  RunMigration();
  failing_profile_store_->set_fail_reads_after_write(false);
  failing_profile_store_->set_fail_reads(false);

  // An unconfirmed write must not be taken for a confirmed one. The credential
  // stays in both stores, and the next launch drains the now-identical account
  // copy.
  EXPECT_EQ(1u, GetLogins(profile_store_.get()).size());
  EXPECT_EQ(1u, GetLogins(account_store_.get()).size());
}

TEST_F(BraveAccountToProfilePasswordMigrationStoreErrorTest,
       ProfileStoreUpdateFailureKeepsAccountCredential) {
  EnableFeature();
  const base::Time older = base::Time::Now();
  const base::Time newer = older + base::Hours(1);
  AddLogin(profile_store_.get(), MakeForm("https://a.com/", "u", "profileold",
                                          /*blocked=*/false, older));
  AddLogin(account_store_.get(), MakeForm("https://a.com/", "u", "accountnew",
                                          /*blocked=*/false, newer));

  failing_profile_store_->set_fail_writes(true);
  RunMigration();
  failing_profile_store_->set_fail_writes(false);

  // The profile copy still holds the old password, so the newer account copy
  // must survive. Its unique key matches the stale profile entry, which is why
  // the verify step compares passwords and timestamps rather than keys alone.
  std::vector<PasswordForm> profile_logins = GetLogins(profile_store_.get());
  ASSERT_EQ(1u, profile_logins.size());
  EXPECT_EQ(u"profileold", profile_logins[0].password_value);
  std::vector<PasswordForm> account_logins = GetLogins(account_store_.get());
  ASSERT_EQ(1u, account_logins.size());
  EXPECT_EQ(u"accountnew", account_logins[0].password_value);
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
