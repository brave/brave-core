// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/password_manager/android/password_ui_view_android.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/android/callback_android.h"
#include "base/android/int_string_callback.h"
#include "base/android/jni_string.h"
#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/numerics/safe_conversions.h"
#include "base/task/thread_pool.h"
#include "base/threading/scoped_blocking_call.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "components/password_manager/core/browser/export/password_csv_writer.h"
#include "components/password_manager/core/browser/form_parsing/form_data_parser.h"
#include "components/password_manager/core/browser/import/password_importer.h"
#include "components/password_manager/core/browser/leak_detection/leak_detection_check_impl.h"
#include "components/password_manager/core/browser/password_form.h"
#include "components/password_manager/core/browser/password_ui_utils.h"
#include "components/password_manager/core/browser/ui/credential_provider_interface.h"
#include "components/password_manager/core/browser/ui/credential_ui_entry.h"
#include "components/password_manager/core/common/password_manager_constants.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

// Must come after other includes, because FromJniType() uses Profile.
#include "chrome/browser/password_manager/android/jni_headers/PasswordUiView_jni.h"

namespace {

using base::android::JavaParamRef;
using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;
using IsInsecureCredential = CredentialEditBridge::IsInsecureCredential;

PasswordUiViewAndroid::SerializationResult SerializePasswords(
    const base::FilePath& target_directory,
    std::vector<password_manager::CredentialUIEntry> credentials) {
  // The UI should not trigger serialization if there are no passwords.
  base::UmaHistogramBoolean(
      "PasswordManager.ExportAndroid.MoreThanZeroPasswords",
      credentials.size() > 0);

  // Creating a file will block the execution on I/O.
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);

  // Ensure that the target directory exists.
  base::File::Error error = base::File::FILE_OK;
  if (!base::CreateDirectoryAndGetError(target_directory, &error)) {
    base::UmaHistogramExactLinear(
        "PasswordManager.ExportAndroid.CreateDirectoryError", -error,
        -base::File::Error::FILE_ERROR_MAX);
    return {0, std::string(), base::File::ErrorToString(error)};
  }

  // Create a temporary file in the target directory to hold the serialized
  // passwords.
  base::FilePath export_file;
  if (!base::CreateTemporaryFileInDir(target_directory, &export_file)) {
    logging::SystemErrorCode error_code = logging::GetLastSystemErrorCode();
    base::UmaHistogramExactLinear(
        "PasswordManager.ExportAndroid.CreateTempFileError",
        -base::File::OSErrorToFileError(error_code),
        -base::File::Error::FILE_ERROR_MAX);
    return {0, std::string(), logging::SystemErrorCodeToString(error_code)};
  }

  // Write the serialized data in CSV.
  std::string data =
      password_manager::PasswordCSVWriter::SerializePasswords(credentials);
  if (!base::WriteFile(export_file, data)) {
    logging::SystemErrorCode error_code = logging::GetLastSystemErrorCode();
    base::UmaHistogramExactLinear(
        "PasswordManager.ExportAndroid.WriteToTempFileError",
        -base::File::OSErrorToFileError(error_code),
        -base::File::Error::FILE_ERROR_MAX);
    return {0, std::string(), logging::SystemErrorCodeToString(error_code)};
  }

  return {static_cast<int>(credentials.size()), export_file.value(),
          std::string()};
}

}  // namespace

PasswordUiViewAndroid::PasswordUiViewAndroid(
    JNIEnv* env,
    const jni_zero::JavaRef<jobject>& obj,
    Profile* profile)
    : profile_(profile),
      profile_store_(ProfilePasswordStoreFactory::GetForProfile(
          profile,
          ServiceAccessType::EXPLICIT_ACCESS)),
      saved_passwords_presenter_(
          AffiliationServiceFactory::GetForProfile(profile),
          profile_store_,
          AccountPasswordStoreFactory::GetForProfile(
              profile,
              ServiceAccessType::EXPLICIT_ACCESS)),
      weak_java_ui_controller_(env, obj) {
  saved_passwords_presenter_.AddObserver(this);
  saved_passwords_presenter_.Init();
}

PasswordUiViewAndroid::~PasswordUiViewAndroid() {
  saved_passwords_presenter_.RemoveObserver(this);
}

void PasswordUiViewAndroid::Destroy(JNIEnv* env) {
  switch (state_) {
    case State::ALIVE:
      delete this;
      break;
    case State::ALIVE_SERIALIZATION_PENDING:
      // Postpone the deletion until the pending tasks are completed, so that
      // the tasks do not attempt a use after free while reading data from
      // |this|.
      state_ = State::DELETION_PENDING;
      break;
    case State::DELETION_PENDING:
      NOTREACHED();
  }
}

void PasswordUiViewAndroid::InsertPasswordEntryForTesting(
    JNIEnv* env,
    const std::u16string& origin,
    const std::u16string& username,
    const std::u16string& password) {
  password_manager::PasswordForm form;
  form.url = GURL(origin);
  form.signon_realm = password_manager::GetSignonRealm(form.url);
  form.username_value = username;
  form.password_value = password;

  profile_store_->AddLogin(form);
}

void PasswordUiViewAndroid::UpdatePasswordLists(JNIEnv* env) {
  DCHECK_EQ(State::ALIVE, state_);
  UpdatePasswordLists();
}

ScopedJavaLocalRef<jobject> PasswordUiViewAndroid::GetSavedPasswordEntry(
    JNIEnv* env,
    int index) {
  DCHECK_EQ(State::ALIVE, state_);
  if (static_cast<size_t>(index) >= passwords_.size()) {
    return Java_PasswordUiView_createSavedPasswordEntry(
        env, std::string(), std::u16string(), std::u16string());
  }
  return Java_PasswordUiView_createSavedPasswordEntry(
      env, password_manager::GetShownOrigin(passwords_[index]),
      passwords_[index].username, passwords_[index].password);
}

std::string PasswordUiViewAndroid::GetSavedPasswordException(JNIEnv* env,
                                                             int index) {
  DCHECK_EQ(State::ALIVE, state_);
  if (static_cast<size_t>(index) >= blocked_sites_.size()) {
    return "";
  }
  return password_manager::GetShownOrigin(blocked_sites_[index]);
}

void PasswordUiViewAndroid::HandleRemoveSavedPasswordEntry(JNIEnv* env,
                                                           int index) {
  DCHECK_EQ(State::ALIVE, state_);
  if (static_cast<size_t>(index) >= passwords_.size()) {
    return;
  }
  if (saved_passwords_presenter_.RemoveCredential(passwords_[index])) {
    base::RecordAction(
        base::UserMetricsAction("PasswordManager_RemoveSavedPassword"));
  }
}

void PasswordUiViewAndroid::HandleRemoveSavedPasswordException(JNIEnv* env,
                                                               int index) {
  DCHECK_EQ(State::ALIVE, state_);
  if (static_cast<size_t>(index) >= passwords_.size()) {
    return;
  }
  if (saved_passwords_presenter_.RemoveCredential(passwords_[index])) {
    base::RecordAction(
        base::UserMetricsAction("PasswordManager_RemovePasswordException"));
  }
}

void PasswordUiViewAndroid::HandleSerializePasswords(
    JNIEnv* env,
    const std::string& java_target_directory,
    const JavaRef<jobject>& success_callback,
    const JavaRef<jobject>& error_callback) {
  switch (state_) {
    case State::ALIVE:
      state_ = State::ALIVE_SERIALIZATION_PENDING;
      break;
    case State::ALIVE_SERIALIZATION_PENDING:
      // The UI should not allow the user to re-request export before finishing
      // or cancelling the pending one.
      NOTREACHED();
    case State::DELETION_PENDING:
      // The Java part should not first request destroying of |this| and then
      // ask |this| for serialized passwords.
      NOTREACHED();
  }
  std::vector<password_manager::CredentialUIEntry> credentials =
      saved_passwords_presenter_.GetSavedCredentials();
  std::erase_if(credentials, [](const auto& credential) {
    return credential.blocked_by_user;
  });

  // The tasks are posted with base::Unretained, because deletion is postponed
  // until the reply arrives (look for the occurrences of DELETION_PENDING in
  // this file). The background processing is not expected to take very long,
  // but still long enough not to block the UI thread. The main concern here is
  // not to avoid the background computation if |this| is about to be deleted
  // but to simply avoid use after free from the background task runner.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::TaskPriority::USER_VISIBLE, base::MayBlock()},
      base::BindOnce(&SerializePasswords, base::FilePath(java_target_directory),
                     std::move(credentials)),
      base::BindOnce(
          &PasswordUiViewAndroid::PostSerializedPasswords,
          base::Unretained(this),
          base::android::ScopedJavaGlobalRef<jobject>(env, success_callback),
          base::android::ScopedJavaGlobalRef<jobject>(env, error_callback)));
}

void PasswordUiViewAndroid::HandleShowPasswordEntryEditingView(
    JNIEnv* env,
    const JavaParamRef<jobject>& context,
    int index) {
  if (static_cast<size_t>(index) >= passwords_.size() ||
      credential_edit_bridge_) {
    return;
  }
  bool is_using_account_store = passwords_[index].stored_in.contains(
      password_manager::PasswordForm::Store::kAccountStore);
  credential_edit_bridge_ = CredentialEditBridge::MaybeCreate(
      passwords_[index], IsInsecureCredential(false),
      GetUsernamesForRealm(saved_passwords_presenter_.GetSavedCredentials(),
                           passwords_[index].GetFirstSignonRealm(),
                           is_using_account_store),
      &saved_passwords_presenter_,
      base::BindOnce(&PasswordUiViewAndroid::OnEditUIDismissed,
                     base::Unretained(this)),
      context);
}

void PasswordUiViewAndroid::HandleShowBlockedCredentialView(
    JNIEnv* env,
    const JavaParamRef<jobject>& context,
    int index) {
  if (static_cast<size_t>(index) >= blocked_sites_.size() ||
      credential_edit_bridge_) {
    return;
  }
  credential_edit_bridge_ = CredentialEditBridge::MaybeCreate(
      blocked_sites_[index], IsInsecureCredential(false),
      std::vector<std::u16string>(), &saved_passwords_presenter_,
      base::BindOnce(&PasswordUiViewAndroid::OnEditUIDismissed,
                     base::Unretained(this)),
      context);
}

void PasswordUiViewAndroid::OnEditUIDismissed() {
  credential_edit_bridge_.reset();
}

std::string JNI_PasswordUiView_GetAccountDashboardURL(JNIEnv* env) {
  return l10n_util::GetStringUTF8(IDS_PASSWORDS_WEB_LINK);
}

std::string JNI_PasswordUiView_GetTrustedVaultLearnMoreURL(JNIEnv* env) {
  return chrome::kSyncTrustedVaultLearnMoreURL;
}

jboolean PasswordUiViewAndroid::IsWaitingForPasswordStore(JNIEnv* env) {
  return saved_passwords_presenter_.IsWaitingForPasswordStore();
}

void PasswordUiViewAndroid::HandleImportPasswordsFromCsv(
    JNIEnv* env,
    const std::string& csv_content,
    const JavaRef<jobject>& success_callback,
    const JavaRef<jobject>& error_callback) {
  // Create a PasswordImporter instance
  auto importer = std::make_unique<password_manager::PasswordImporter>(
      &saved_passwords_presenter_);

  // Capture the importer as a raw pointer before moving it into the callback
  password_manager::PasswordImporter* importer_ptr = importer.get();

  // Create callbacks that will be called when import completes
  auto results_callback = base::BindOnce(
      [](const base::android::JavaRef<jobject>& success_callback,
         const base::android::JavaRef<jobject>& error_callback,
         std::unique_ptr<password_manager::PasswordImporter> importer,
         const password_manager::ImportResults& results) {
        if (results.status ==
            password_manager::ImportResults::Status::SUCCESS) {
          // Success - call success callback with number of imported passwords
          base::android::RunIntCallbackAndroid(success_callback,
                                               results.number_imported);
        } else {
          // Error - call error callback with error message
          base::android::RunIntCallbackAndroid(
              error_callback, static_cast<int32_t>(results.status));
        }
      },
      base::android::ScopedJavaGlobalRef<jobject>(env, success_callback),
      base::android::ScopedJavaGlobalRef<jobject>(env, error_callback),
      std::move(importer));

  // Start the import with the CSV content
  importer_ptr->Import(csv_content,
                       password_manager::PasswordForm::Store::kProfileStore,
                       std::move(results_callback));
}

jint JNI_PasswordUiView_GetMaxPasswordsPerCsvFile(JNIEnv* env) {
  return password_manager::constants::kMaxPasswordsPerCSVFile;
}

// static
static jlong JNI_PasswordUiView_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    Profile* profile) {
  PasswordUiViewAndroid* controller =
      new PasswordUiViewAndroid(env, obj, profile);
  return reinterpret_cast<intptr_t>(controller);
}

void PasswordUiViewAndroid::OnSavedPasswordsChanged(
    const password_manager::PasswordStoreChangeList& changes) {
  UpdatePasswordLists();
}

void PasswordUiViewAndroid::UpdatePasswordLists() {
  passwords_.clear();
  blocked_sites_.clear();
  for (auto& credential : saved_passwords_presenter_.GetSavedCredentials()) {
    if (credential.blocked_by_user) {
      blocked_sites_.push_back(std::move(credential));
    } else {
      passwords_.push_back(std::move(credential));
    }
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> ui_controller = weak_java_ui_controller_.get(env);
  if (!ui_controller.is_null()) {
    Java_PasswordUiView_passwordListAvailable(
        env, ui_controller, static_cast<int>(passwords_.size()));
    Java_PasswordUiView_passwordExceptionListAvailable(
        env, ui_controller, static_cast<int>(blocked_sites_.size()));
  }
}

void PasswordUiViewAndroid::PostSerializedPasswords(
    const base::android::JavaRef<jobject>& success_callback,
    const base::android::JavaRef<jobject>& error_callback,
    SerializationResult serialization_result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  switch (state_) {
    case State::ALIVE:
      NOTREACHED();
    case State::ALIVE_SERIALIZATION_PENDING: {
      state_ = State::ALIVE;
      if (export_target_for_testing_) {
        *export_target_for_testing_ = serialization_result;
      } else {
        if (serialization_result.entries_count) {
          base::android::RunIntStringCallbackAndroid(
              success_callback, serialization_result.entries_count,
              serialization_result.exported_file_path);
        } else {
          base::android::RunStringCallbackAndroid(error_callback,
                                                  serialization_result.error);
        }
      }
      break;
    }
    case State::DELETION_PENDING:
      delete this;
      break;
  }
}
