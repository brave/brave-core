/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/password_manager/android/brave_account_to_profile_password_migration.h"

#include <algorithm>
#include <utility>
#include <variant>
#include <vector>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
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

// Self-owned. Moves passwords account store -> profile store, then self-deletes.
// Order is strictly copy -> verify -> delete: a credential is removed from the
// account store only after it is confirmed present in the profile store.
class AccountToProfilePasswordMigrator
    : public password_manager::PasswordStoreConsumer {
 public:
  AccountToProfilePasswordMigrator(
      scoped_refptr<PasswordStoreInterface> account_store,
      scoped_refptr<PasswordStoreInterface> profile_store)
      : account_store_(std::move(account_store)),
        profile_store_(std::move(profile_store)) {}

  void Start() {
    // Step 1: read the account store.
    account_store_->GetAutofillableLogins(weak_factory_.GetWeakPtr());
  }

 private:
  // PasswordStoreConsumer:
  void OnGetPasswordStoreResultsOrErrorFrom(
      PasswordStoreInterface* store,
      LoginsResultOrError results_or_error) override {
    if (std::holds_alternative<PasswordStoreBackendError>(results_or_error)) {
      // A read failed; keep everything as-is and abort safely.
      delete this;
      return;
    }
    std::vector<PasswordForm> forms = password_manager::ToPasswordForms(
        std::get<LoginsResult>(std::move(results_or_error)));
    if (store == account_store_.get()) {
      OnAccountLogins(std::move(forms));
    } else {
      OnProfileLoginsAfterAdd(std::move(forms));
    }
  }

  void OnAccountLogins(std::vector<PasswordForm> account_forms) {
    if (account_forms.empty()) {
      delete this;  // Nothing to migrate.
      return;
    }
    account_forms_ = std::move(account_forms);
    // Step 2 (copy): add every account credential to the profile store.
    std::vector<password_manager::StoredCredential> to_add;
    to_add.reserve(account_forms_.size());
    for (const PasswordForm& form : account_forms_) {
      to_add.push_back(password_manager::FromPasswordForm(form));
    }
    profile_store_->AddLogins(
        std::move(to_add),
        base::BindOnce(&AccountToProfilePasswordMigrator::OnAddedToProfile,
                       weak_factory_.GetWeakPtr()));
  }

  void OnAddedToProfile() {
    // Step 3 (verify): re-read the profile store before deleting anything.
    profile_store_->GetAutofillableLogins(weak_factory_.GetWeakPtr());
  }

  void OnProfileLoginsAfterAdd(std::vector<PasswordForm> profile_forms) {
    // Step 4 (delete): remove from the account store only the credentials that
    // are now confirmed present in the profile store.
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
    delete this;
  }

  scoped_refptr<PasswordStoreInterface> account_store_;
  scoped_refptr<PasswordStoreInterface> profile_store_;
  std::vector<PasswordForm> account_forms_;
  base::WeakPtrFactory<AccountToProfilePasswordMigrator> weak_factory_{this};
};

}  // namespace

void MaybeMigrateAccountPasswordsToProfileStore(Profile* profile) {
  if (!base::FeatureList::IsEnabled(
          brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore)) {
    return;
  }
  scoped_refptr<PasswordStoreInterface> account_store =
      AccountPasswordStoreFactory::GetForProfile(
          profile, ServiceAccessType::EXPLICIT_ACCESS);
  scoped_refptr<PasswordStoreInterface> profile_store =
      ProfilePasswordStoreFactory::GetForProfile(
          profile, ServiceAccessType::EXPLICIT_ACCESS);
  if (!account_store || !profile_store) {
    return;
  }
  // Self-owned; deletes itself when the migration finishes.
  (new AccountToProfilePasswordMigrator(std::move(account_store),
                                        std::move(profile_store)))
      ->Start();
}

}  // namespace brave_password_manager
