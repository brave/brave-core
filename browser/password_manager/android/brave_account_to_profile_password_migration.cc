/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/password_manager/android/brave_account_to_profile_password_migration.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/barrier_closure.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/supports_user_data.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "brave/components/brave_sync/features.h"
#include "chrome/browser/password_manager/factories/account_password_store_factory.h"
#include "chrome/browser/password_manager/factories/profile_password_store_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/browser/password_store/password_form_converters.h"
#include "components/password_manager/core/browser/password_store/password_store_backend_error.h"
#include "components/password_manager/core/browser/password_store/password_store_consumer.h"
#include "components/password_manager/core/browser/password_store/password_store_interface.h"
#include "components/password_manager/core/browser/password_store/stored_credential.h"

namespace brave_password_manager {

namespace {

using password_manager::PasswordStoreBackendError;
using password_manager::PasswordStoreInterface;
using password_manager::StoredCredential;

// Key under which the profile owns the running migrator. Only the address is
// used.
const char kMigratorUserDataKey[] = "BraveAccountToProfilePasswordMigrator";

// Runs `on_complete` from a fresh task, so that the early returns below finish
// asynchronously like the migration itself does and callers see one ordering.
void FinishAsync(base::OnceClosure on_complete) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, std::move(on_complete));
}

// The most recent of a credential's last-used, password-modified and creation
// times. Used to decide which side wins when the same credential exists in both
// stores with different passwords (mirrors the logic in
// PasswordLocalDataBatchUploader).
base::Time LatestTimestamp(const StoredCredential& credential) {
  return std::ranges::max({credential.date_last_used,
                           credential.date_password_modified,
                           credential.date_created});
}

// True if the profile-store copy already reflects `account_credential`, i.e. it
// holds
// the same password or a more recently changed one. This is the condition under
// which the account copy needs no write, and equally the condition under which
// it is safe to drop from the account store: a same-key profile copy with an
// older, different password means the write did not land.
// Both credentials are expected to have equal unique keys; callers establish
// that with AreStoredCredentialUniqueKeysEqual before asking.
bool ProfileCopyIsUpToDate(const StoredCredential& profile_credential,
                           const StoredCredential& account_credential) {
  return profile_credential.password_value ==
             account_credential.password_value ||
         LatestTimestamp(account_credential) <=
             LatestTimestamp(profile_credential);
}

// Copies an account-store credential for the profile store, clearing the
// username/password of blocklisted ("never save") entries. The profile store
// CHECKs that blocklisted credentials have empty username and password
// (PasswordStore::AddLogins / UpdateLogins), so a malformed synced entry could
// otherwise trip that CHECK during migration.
StoredCredential ToProfileStoreCredential(const StoredCredential& credential) {
  StoredCredential sanitized =
      password_manager::CloneStoredCredential(credential);
  if (sanitized.blocked_by_user) {
    sanitized.username_value.clear();
    sanitized.password_value.clear();
  }
  return sanitized;
}

// Reads all logins from a single store and runs `done_callback` when finished.
// Owned by the migrator, which keeps the fetched credentials from outliving it
// if the store drops the read. Mirrors
// PasswordLocalDataBatchUploader::PasswordFetchRequest, except that one keeps
// itself alive through its own `done_callback_`. On a read error the results
// are left empty and succeeded() gives false.
class PasswordFetchRequest : public password_manager::PasswordStoreConsumer {
 public:
  PasswordFetchRequest() = default;
  PasswordFetchRequest(const PasswordFetchRequest&) = delete;
  PasswordFetchRequest& operator=(const PasswordFetchRequest&) = delete;
  ~PasswordFetchRequest() override = default;

  void Run(PasswordStoreInterface* store, base::OnceClosure done_callback) {
    done_callback_ = std::move(done_callback);
    store->GetAllLogins(weak_ptr_factory_.GetWeakPtr());
  }

  std::vector<StoredCredential> TakeResults() { return std::move(results_); }
  bool succeeded() const { return succeeded_; }

 private:
  // PasswordStoreConsumer:
  void OnGetPasswordStoreResultsOrErrorFrom(
      PasswordStoreInterface* store,
      base::expected<std::vector<StoredCredential>, PasswordStoreBackendError>
          results_or_error) override {
    if (results_or_error.has_value()) {
      results_ = std::move(*results_or_error);
    } else {
      succeeded_ = false;
    }
    std::move(done_callback_).Run();
    // `this` may be deleted now; do not touch any member below.
  }

  bool succeeded_ = true;
  std::vector<StoredCredential> results_;
  base::OnceClosure done_callback_;
  base::WeakPtrFactory<PasswordFetchRequest> weak_ptr_factory_{this};
};

// Moves passwords from the account store to the profile store. A credential is
// removed from the account store only after it is confirmed present in the
// profile store (copy/merge -> verify -> delete). All logins are migrated,
// including blocklisted ("never save") entries. When the same credential
// exists in both stores with different passwords, the most recently changed
// one wins, so an account-only newer password is never dropped.
//
// Owned by the profile as user data, so that it is destroyed with the profile
// even if a step never completes: every `PasswordStore` method returns early
// once the store starts shutting down, dropping the callback without running
// it. Callbacks are bound with weak pointers, so a dropped step simply stops
// the chain.
class AccountToProfilePasswordMigrator : public base::SupportsUserData::Data {
 public:
  AccountToProfilePasswordMigrator(const AccountToProfilePasswordMigrator&) =
      delete;
  AccountToProfilePasswordMigrator& operator=(
      const AccountToProfilePasswordMigrator&) = delete;
  ~AccountToProfilePasswordMigrator() override = default;

  // Hands the migrator to `profile` to own, and starts it. Callers must have
  // checked that `profile` holds no migrator yet.
  static void CreateAndStart(
      Profile* profile,
      scoped_refptr<PasswordStoreInterface> account_store,
      scoped_refptr<PasswordStoreInterface> profile_store,
      base::OnceClosure on_complete) {
    auto migrator = base::WrapUnique(new AccountToProfilePasswordMigrator(
        profile, std::move(account_store), std::move(profile_store),
        std::move(on_complete)));
    AccountToProfilePasswordMigrator* migrator_ptr = migrator.get();
    profile->SetUserData(kMigratorUserDataKey, std::move(migrator));
    migrator_ptr->Start();
  }

 private:
  AccountToProfilePasswordMigrator(
      Profile* profile,
      scoped_refptr<PasswordStoreInterface> account_store,
      scoped_refptr<PasswordStoreInterface> profile_store,
      base::OnceClosure on_complete)
      : profile_(profile),
        account_store_(std::move(account_store)),
        profile_store_(std::move(profile_store)),
        on_complete_(std::move(on_complete)) {}

  void Start() {
    // Step 1: read the account store.
    ReadStore(account_store_.get(),
              base::BindOnce(&AccountToProfilePasswordMigrator::OnAccountLogins,
                             weak_factory_.GetWeakPtr()));
  }

  // Reads all logins from `store` into `pending_request_`, running `on_results`
  // when done. The request is a member rather than owned by its own callback,
  // so a read the store never answers does not keep the fetched credentials
  // alive on their own.
  void ReadStore(PasswordStoreInterface* store, base::OnceClosure on_results) {
    pending_request_ = std::make_unique<PasswordFetchRequest>();
    pending_request_->Run(store, std::move(on_results));
  }

  // Each step below starts by taking the finished request, so that it is
  // destroyed when the step returns rather than when the next read replaces it.
  std::unique_ptr<PasswordFetchRequest> TakePendingRequest() {
    return std::move(pending_request_);
  }

  void OnAccountLogins() {
    std::unique_ptr<PasswordFetchRequest> request = TakePendingRequest();
    account_credentials_ = request->TakeResults();
    if (account_credentials_.empty()) {
      Finish();  // Nothing to migrate, either no records or failed to read.
      return;
    }
    // Step 2: read the profile store before deciding how to merge each
    // credential.
    ReadStore(profile_store_.get(),
              base::BindOnce(
                  &AccountToProfilePasswordMigrator::OnProfileLoginsForMerge,
                  weak_factory_.GetWeakPtr()));
  }

  void OnProfileLoginsForMerge() {
    std::unique_ptr<PasswordFetchRequest> request = TakePendingRequest();
    if (!request->succeeded()) {
      // Abort rather than treat a failed read as an empty profile store, which
      // would add every account credential and overwrite newer profile-store
      // copies. The next launch retries.
      Finish();
      return;
    }

    std::vector<StoredCredential> profile_credentials = request->TakeResults();

    // Copy/merge: add credentials missing from the profile store, and overwrite
    // ones whose account copy has a newer, different password.
    std::vector<StoredCredential> to_add;
    std::vector<StoredCredential> to_update;
    for (const StoredCredential& account_credential : account_credentials_) {
      auto it = std::ranges::find_if(
          profile_credentials,
          [&account_credential](const StoredCredential& profile_credential) {
            return password_manager::AreStoredCredentialUniqueKeysEqual(
                profile_credential, account_credential);
          });
      if (it == profile_credentials.end()) {
        to_add.push_back(ToProfileStoreCredential(account_credential));
      } else if (!ProfileCopyIsUpToDate(*it, account_credential)) {
        to_update.push_back(ToProfileStoreCredential(account_credential));
      }
      // Otherwise the profile copy already wins; it will still be drained from
      // the account store in the verify step below.
    }

    if (to_add.empty() && to_update.empty()) {
      // Every account credential already has an equal-or-newer copy in the
      // profile store; skip straight to draining the account store.
      VerifyAndDrainAccountStore();
      return;
    }

    const int barrier_count =
        (to_add.empty() ? 0 : 1) + (to_update.empty() ? 0 : 1);
    base::RepeatingClosure barrier = base::BarrierClosure(
        barrier_count,
        base::BindOnce(
            &AccountToProfilePasswordMigrator::VerifyAndDrainAccountStore,
            weak_factory_.GetWeakPtr()));
    if (!to_add.empty()) {
      profile_store_->AddLogins(std::move(to_add), barrier);
    }
    if (!to_update.empty()) {
      profile_store_->UpdateLogins(std::move(to_update), barrier);
    }
  }

  void VerifyAndDrainAccountStore() {
    // Step 3: re-read the profile store to confirm the writes landed before
    // deleting anything from the account store.
    ReadStore(profile_store_.get(),
              base::BindOnce(
                  &AccountToProfilePasswordMigrator::OnProfileLoginsForVerify,
                  weak_factory_.GetWeakPtr()));
  }

  void OnProfileLoginsForVerify() {
    std::unique_ptr<PasswordFetchRequest> request = TakePendingRequest();
    std::vector<StoredCredential> profile_credentials = request->TakeResults();
    // Remove from the account store only the credentials whose profile-store
    // copy is confirmed present and up to date. A failed write leaves the
    // profile copy stale, in which case the account copy is kept for the next
    // launch to retry.
    for (const StoredCredential& account_credential : account_credentials_) {
      const bool up_to_date_in_profile = std::ranges::any_of(
          profile_credentials,
          [&account_credential](const StoredCredential& profile_credential) {
            return password_manager::AreStoredCredentialUniqueKeysEqual(
                       profile_credential, account_credential) &&
                   ProfileCopyIsUpToDate(profile_credential,
                                         account_credential);
          });
      if (up_to_date_in_profile) {
        account_store_->RemoveLogin(FROM_HERE, account_credential);
      }
    }
    // Finish only after the removals above have been processed. RemoveLogin has
    // no completion callback, but store operations run in order, so a trailing
    // read completes after them.
    ReadStore(account_store_.get(),
              base::BindOnce(&AccountToProfilePasswordMigrator::OnFinished,
                             weak_factory_.GetWeakPtr()));
  }

  // The trailing read's results are not needed, it only orders this step after
  // the removals; `pending_request_` is destroyed together with `this`.
  void OnFinished() { Finish(); }

  // Runs the completion callback and deletes `this` by taking it back from the
  // profile. The owning pointer is kept in a local so `this` stays alive during
  // the callback and is destroyed when the local goes out of scope.
  void Finish() {
    std::unique_ptr<base::SupportsUserData::Data> self =
        profile_->TakeUserData(kMigratorUserDataKey);
    std::move(on_complete_).Run();
  }

  const raw_ptr<Profile> profile_;
  const scoped_refptr<PasswordStoreInterface> account_store_;
  const scoped_refptr<PasswordStoreInterface> profile_store_;
  std::vector<StoredCredential> account_credentials_;
  base::OnceClosure on_complete_;
  std::unique_ptr<PasswordFetchRequest> pending_request_;
  base::WeakPtrFactory<AccountToProfilePasswordMigrator> weak_factory_{this};
};

}  // namespace

void MaybeMigrateAccountPasswordsToProfileStore(Profile* profile,
                                                base::OnceClosure on_complete) {
  if (!base::FeatureList::IsEnabled(
          brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore)) {
    FinishAsync(std::move(on_complete));
    return;
  }
  if (profile->GetUserData(kMigratorUserDataKey)) {
    // A migration for this profile is already running; it owns the stores until
    // it finishes, so starting a second one would race it.
    FinishAsync(std::move(on_complete));
    return;
  }
  scoped_refptr<PasswordStoreInterface> account_store =
      AccountPasswordStoreFactory::GetForProfile(
          profile, ServiceAccessType::EXPLICIT_ACCESS);
  scoped_refptr<PasswordStoreInterface> profile_store =
      ProfilePasswordStoreFactory::GetForProfile(
          profile, ServiceAccessType::EXPLICIT_ACCESS);
  if (!account_store || !profile_store) {
    FinishAsync(std::move(on_complete));
    return;
  }
  AccountToProfilePasswordMigrator::CreateAndStart(
      profile, std::move(account_store), std::move(profile_store),
      std::move(on_complete));
}

}  // namespace brave_password_manager
