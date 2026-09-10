/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/password_manager/android/brave_account_to_profile_password_migration.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

#include "base/barrier_closure.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/location.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/components/brave_sync/features.h"
#include "chrome/browser/password_manager/factories/account_password_store_factory.h"
#include "chrome/browser/password_manager/factories/profile_password_store_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/browser/password_form.h"
#include "components/password_manager/core/browser/password_store/password_form_converters.h"
#include "components/password_manager/core/browser/password_store/password_store_consumer.h"
#include "components/password_manager/core/browser/password_store/password_store_interface.h"

namespace brave_password_manager {

namespace {

using password_manager::LoginsResult;
using password_manager::LoginsResultOrError;
using password_manager::PasswordForm;
using password_manager::PasswordStoreBackendError;
using password_manager::PasswordStoreInterface;
using password_manager::StoredCredential;

// The most recent of a form's last-used, password-modified and creation times.
// Used to decide which side wins when the same credential exists in both
// stores with different passwords (mirrors the logic in
// PasswordLocalDataBatchUploader).
base::Time LatestTimestamp(const PasswordForm& form) {
  return std::ranges::max(
      {form.date_last_used, form.date_password_modified, form.date_created});
}

// Reads all logins from a single store and runs `done_callback` when finished.
// Keeps itself alive by being moved into that callback, and is destroyed once
// it runs. Mirrors PasswordLocalDataBatchUploader::PasswordFetchRequest. On a
// read error the results are left empty (the migrator treats that as "nothing
// to copy / nothing verified", so it never deletes unverified data).
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

  std::vector<PasswordForm> TakeResults() { return std::move(results_); }

 private:
  // PasswordStoreConsumer:
  void OnGetPasswordStoreResultsOrErrorFrom(
      PasswordStoreInterface* store,
      LoginsResultOrError results_or_error) override {
    if (!std::holds_alternative<PasswordStoreBackendError>(results_or_error)) {
      results_ = password_manager::ToPasswordForms(
          std::get<LoginsResult>(std::move(results_or_error)));
    }
    std::move(done_callback_).Run();
    // `this` may be deleted now; do not touch any member below.
  }

  std::vector<PasswordForm> results_;
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
// Self-owned: it holds its own `unique_ptr` (`self_`) so it outlives the async
// store operations, and drops it to delete itself once the last step finishes.
// Callbacks are bound with weak pointers, so an in-flight step that is dropped
// (e.g. the store shuts down) simply stops the chain.
class AccountToProfilePasswordMigrator {
 public:
  AccountToProfilePasswordMigrator(
      scoped_refptr<PasswordStoreInterface> account_store,
      scoped_refptr<PasswordStoreInterface> profile_store,
      base::OnceClosure on_complete)
      : account_store_(std::move(account_store)),
        profile_store_(std::move(profile_store)),
        on_complete_(std::move(on_complete)) {}

  AccountToProfilePasswordMigrator(const AccountToProfilePasswordMigrator&) =
      delete;
  AccountToProfilePasswordMigrator& operator=(
      const AccountToProfilePasswordMigrator&) = delete;

  // Takes ownership of itself via `self` and starts the migration.
  void Start(std::unique_ptr<AccountToProfilePasswordMigrator> self) {
    self_ = std::move(self);
    // Step 1: read the account store.
    ReadStore(account_store_.get(),
              base::BindOnce(&AccountToProfilePasswordMigrator::OnAccountLogins,
                             weak_factory_.GetWeakPtr()));
  }

 private:
  // Reads all logins from `store`, passing the (owned) fetch request to
  // `on_results` when done.
  void ReadStore(PasswordStoreInterface* store,
                 base::OnceCallback<void(std::unique_ptr<PasswordFetchRequest>)>
                     on_results) {
    auto request = std::make_unique<PasswordFetchRequest>();
    PasswordFetchRequest* request_ptr = request.get();
    request_ptr->Run(store,
                     base::BindOnce(std::move(on_results), std::move(request)));
  }

  void OnAccountLogins(std::unique_ptr<PasswordFetchRequest> request) {
    account_forms_ = request->TakeResults();
    if (account_forms_.empty()) {
      Finish();  // Nothing to migrate.
      return;
    }
    // Step 2: read the profile store before deciding how to merge each
    // credential.
    ReadStore(profile_store_.get(),
              base::BindOnce(
                  &AccountToProfilePasswordMigrator::OnProfileLoginsForMerge,
                  weak_factory_.GetWeakPtr()));
  }

  void OnProfileLoginsForMerge(std::unique_ptr<PasswordFetchRequest> request) {
    std::vector<PasswordForm> profile_forms = request->TakeResults();

    // Copy/merge: add credentials missing from the profile store, and overwrite
    // ones whose account copy has a newer, different password.
    std::vector<StoredCredential> to_add;
    std::vector<StoredCredential> to_update;
    for (const PasswordForm& account_form : account_forms_) {
      auto it = std::ranges::find_if(
          profile_forms, [&account_form](const PasswordForm& profile_form) {
            return password_manager::ArePasswordFormUniqueKeysEqual(
                profile_form, account_form);
          });
      if (it == profile_forms.end()) {
        to_add.push_back(password_manager::FromPasswordForm(account_form));
      } else if (it->password_value != account_form.password_value &&
                 LatestTimestamp(*it) < LatestTimestamp(account_form)) {
        to_update.push_back(password_manager::FromPasswordForm(account_form));
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

  void OnProfileLoginsForVerify(std::unique_ptr<PasswordFetchRequest> request) {
    std::vector<PasswordForm> profile_forms = request->TakeResults();
    // Remove from the account store only the credentials that are now confirmed
    // present in the profile store.
    for (const PasswordForm& account_form : account_forms_) {
      const bool present_in_profile = std::ranges::any_of(
          profile_forms, [&account_form](const PasswordForm& profile_form) {
            return password_manager::ArePasswordFormUniqueKeysEqual(
                profile_form, account_form);
          });
      if (present_in_profile) {
        account_store_->RemoveLogin(
            FROM_HERE, password_manager::FromPasswordForm(account_form));
      }
    }
    // Finish only after the removals above have been processed. RemoveLogin has
    // no completion callback, but store operations run in order, so a trailing
    // read completes after them.
    ReadStore(account_store_.get(),
              base::BindOnce(&AccountToProfilePasswordMigrator::OnFinished,
                             weak_factory_.GetWeakPtr()));
  }

  void OnFinished(std::unique_ptr<PasswordFetchRequest> request) { Finish(); }

  // Runs the completion callback and deletes `this`. `self_` is moved into a
  // local first so `this` stays alive during the callback and is destroyed when
  // the local goes out of scope.
  void Finish() {
    std::unique_ptr<AccountToProfilePasswordMigrator> self = std::move(self_);
    std::move(on_complete_).Run();
  }

  const scoped_refptr<PasswordStoreInterface> account_store_;
  const scoped_refptr<PasswordStoreInterface> profile_store_;
  std::vector<PasswordForm> account_forms_;
  base::OnceClosure on_complete_;
  std::unique_ptr<AccountToProfilePasswordMigrator> self_;
  base::WeakPtrFactory<AccountToProfilePasswordMigrator> weak_factory_{this};
};

}  // namespace

void MaybeMigrateAccountPasswordsToProfileStore(Profile* profile,
                                                base::OnceClosure on_complete) {
  if (!base::FeatureList::IsEnabled(
          brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore)) {
    std::move(on_complete).Run();
    return;
  }
  scoped_refptr<PasswordStoreInterface> account_store =
      AccountPasswordStoreFactory::GetForProfile(
          profile, ServiceAccessType::EXPLICIT_ACCESS);
  scoped_refptr<PasswordStoreInterface> profile_store =
      ProfilePasswordStoreFactory::GetForProfile(
          profile, ServiceAccessType::EXPLICIT_ACCESS);
  if (!account_store || !profile_store) {
    std::move(on_complete).Run();
    return;
  }
  auto migrator = std::make_unique<AccountToProfilePasswordMigrator>(
      std::move(account_store), std::move(profile_store),
      std::move(on_complete));
  AccountToProfilePasswordMigrator* migrator_ptr = migrator.get();
  migrator_ptr->Start(std::move(migrator));
}

}  // namespace brave_password_manager
