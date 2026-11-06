/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/types/expected.h"
#include "brave/components/brave_sync/features.h"
#include "chrome/browser/password_manager/factories/password_store_backend_factory.h"
#include "components/affiliations/core/browser/fake_affiliation_service.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "components/password_manager/core/browser/password_form.h"
#include "components/password_manager/core/browser/password_store/login_database.h"
#include "components/password_manager/core/browser/password_store/password_form_converters.h"
#include "components/password_manager/core/browser/password_store/password_store_backend.h"
#include "components/password_manager/core/browser/password_store/password_store_backend_error.h"
#include "components/password_manager/core/browser/password_store/password_store_interface.h"
#include "components/password_manager/core/browser/password_store/stored_credential.h"
#include "components/password_manager/core/browser/password_store_factory_util.h"
#include "components/password_manager/core/browser/password_string.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync/base/data_type.h"
#include "components/sync/model/data_type_controller_delegate.h"
#include "components/sync/protocol/data_type_state.pb.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_password_manager {

namespace {

using password_manager::IsAccountStore;
using password_manager::PasswordForm;
using password_manager::PasswordStoreBackend;
using password_manager::PasswordStoreBackendError;
using password_manager::StoredCredential;

scoped_refptr<os_crypt_async::Encryptor> GetEncryptorSync(
    os_crypt_async::OSCryptAsync* os_crypt_async) {
  base::test::TestFuture<scoped_refptr<os_crypt_async::Encryptor>> future;
  os_crypt_async->GetInstance(future.GetCallback());
  return future.Take();
}

}  // namespace

// The wipe behavior a backend was created with is private to it, so the only
// way to tell which one CreatePasswordStoreBackend picked is to disable sync on
// the backend it returns and look at what happened to the credentials.
class BravePasswordStoreBackendFactoryTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    prefs_.registry()->RegisterBooleanPref(
        password_manager::prefs::kClearingUndecryptablePasswords, false);
    prefs_.registry()->RegisterIntegerPref(
        password_manager::prefs::kPasswordRemovalReasonForAccount, 0);
    prefs_.registry()->RegisterIntegerPref(
        password_manager::prefs::kPasswordRemovalReasonForProfile, 0);
  }

  void TearDown() override {
    if (!backend_) {
      return;
    }
    base::test::TestFuture<void> shutdown;
    backend_->Shutdown(shutdown.GetCallback());
    ASSERT_TRUE(shutdown.Wait());
    backend_.reset();
  }

 protected:
  // Leaves an account store database behind that looks like the one of a user
  // who has been syncing: a credential plus the sync metadata that makes the
  // sync machinery consider the initial sync done. Without tracked metadata
  // nothing would ask the bridge to act on sync being disabled and the test
  // would pass whatever behavior the backend was created with.
  void SeedSyncedAccountStore() {
    std::unique_ptr<password_manager::LoginDatabase> login_db =
        password_manager::CreateLoginDatabase(IsAccountStore(true),
                                              temp_dir_.GetPath(), &prefs_);
    ASSERT_TRUE(login_db->Init(base::DoNothing(),
                               GetEncryptorSync(os_crypt_async_.get())));

    PasswordForm form;
    form.url = GURL("https://example.com/");
    form.signon_realm = "https://example.com/";
    form.username_value = u"username";
    form.password_value = password_manager::PasswordString(u"password");
    ASSERT_FALSE(
        login_db->AddLogin(password_manager::FromPasswordForm(form)).empty());

    sync_pb::DataTypeState data_type_state;
    data_type_state.set_initial_sync_state(
        sync_pb::DataTypeState_InitialSyncState_INITIAL_SYNC_DONE);
    // Otherwise PasswordSyncBridge::Init() drops the metadata to re-download
    // password notes.
    data_type_state.set_notes_enabled_before_initial_sync_for_passwords(true);
    ASSERT_TRUE(login_db->password_sync_metadata_store().UpdateDataTypeState(
        syncer::PASSWORDS, data_type_state));
  }

  void CreateAccountStoreBackend() {
    backend_ = CreatePasswordStoreBackend(
        IsAccountStore(true), temp_dir_.GetPath(), &prefs_,
        os_crypt_async_.get(), &affiliation_service_);
    ASSERT_TRUE(backend_);

    base::test::TestFuture<bool> init;
    // The backend runs the sync enabled/disabled callback on its own sequence.
    backend_->InitBackend(
        /*remote_form_changes_received=*/base::DoNothing(),
        sync_disabled_.GetSequenceBoundRepeatingCallback(), init.GetCallback());
    ASSERT_TRUE(init.Get());
  }

  // Mimics what the sync machinery does once the user has left the sync chain:
  // it ends up in PasswordSyncBridge::ApplyDisableSyncChanges(), which is where
  // the wipe behavior is acted upon.
  void DisableSync() {
    std::unique_ptr<syncer::DataTypeControllerDelegate> delegate =
        backend_->CreateSyncControllerDelegate();
    delegate->ClearMetadataIfStopped();
    ASSERT_TRUE(sync_disabled_.Wait());
  }

  std::vector<StoredCredential> GetAllLogins() {
    base::test::TestFuture<base::expected<std::vector<StoredCredential>,
                                          PasswordStoreBackendError>>
        logins;
    backend_->GetAllLoginsAsync(logins.GetCallback());
    base::expected<std::vector<StoredCredential>, PasswordStoreBackendError>
        result = logins.Take();
    if (!result.has_value()) {
      ADD_FAILURE() << "Reading the account store failed";
      return {};
    }
    return std::move(result).value();
  }

  base::test::ScopedFeatureList feature_list_;

 private:
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  TestingPrefServiceSimple prefs_;
  affiliations::FakeAffiliationService affiliation_service_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_async_ =
      os_crypt_async::GetTestOSCryptAsyncForTesting(
          /*is_sync_for_unittests=*/true);
  base::test::TestFuture<void> sync_disabled_;
  std::unique_ptr<PasswordStoreBackend> backend_;
};

// Without the feature the account store is the synced one and upstream wipes it
// along with the sync chain.
TEST_F(BravePasswordStoreBackendFactoryTest,
       AccountStoreIsWipedWhenFeatureDisabled) {
  feature_list_.InitAndDisableFeature(
      brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore);

  SeedSyncedAccountStore();
  CreateAccountStoreBackend();
  DisableSync();

  EXPECT_TRUE(GetAllLogins().empty());
}

// With the feature the profile store is the synced one, so the account store
// has to keep its credentials for the startup migrator to move them.
TEST_F(BravePasswordStoreBackendFactoryTest,
       AccountStoreIsKeptWhenFeatureEnabled) {
  feature_list_.InitAndEnableFeature(
      brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore);

  SeedSyncedAccountStore();
  CreateAccountStoreBackend();
  DisableSync();

  EXPECT_EQ(1u, GetAllLogins().size());
}

}  // namespace brave_password_manager
