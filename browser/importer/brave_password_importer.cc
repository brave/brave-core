/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_password_importer.h"

#include <cstddef>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "brave/common/importer/scoped_copy_file.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/password_manager/factories/profile_password_store_factory.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/password_manager/core/browser/password_store/login_database.h"
#include "components/password_manager/core/browser/password_store/password_store_interface.h"
#include "components/password_manager/core/browser/password_store/stored_credential.h"

BravePasswordImporter::ReadResult::ReadResult() = default;
BravePasswordImporter::ReadResult::ReadResult(ReadResult&&) = default;
BravePasswordImporter::ReadResult& BravePasswordImporter::ReadResult::operator=(
    ReadResult&&) = default;
BravePasswordImporter::ReadResult::~ReadResult() = default;

// static
BravePasswordImporter::ReadResult
BravePasswordImporter::ReadCredentialsFromLoginData(
    base::FilePath login_data_path,
    scoped_refptr<os_crypt_async::Encryptor> encryptor) {
  ReadResult read_result;
  if (!base::PathExists(login_data_path)) {
    read_result.result = BravePasswordImporter::Result::kLoginDataNotFound;
    return read_result;
  }

  ScopedCopyFile copy_login_data(login_data_path);
  if (!copy_login_data.copy_success()) {
    LOG(ERROR) << "Failed to copy Login Data for password import";
    read_result.result = BravePasswordImporter::Result::kCopyFailed;
    return read_result;
  }

  password_manager::LoginDatabase database(
      copy_login_data.copied_file_path(),
      password_manager::IsAccountStore(false));
  if (!database.Init(
          /*on_undecryptable_passwords_removed=*/base::NullCallback(),
          std::move(encryptor))) {
    LOG(ERROR) << "LoginDatabase Init() failed for password import";
    read_result.result = BravePasswordImporter::Result::kDatabaseInitFailed;
    return read_result;
  }

  // GetAllLogins returns a FormRetrievalResult, letting us distinguish a
  // decryption/key failure from an empty database. This is the same call the
  // password store backend uses.
  password_manager::FormRetrievalResult retrieval_result =
      database.GetAllLogins(&read_result.credentials);
  if (retrieval_result != password_manager::FormRetrievalResult::kSuccess &&
      retrieval_result != password_manager::FormRetrievalResult::
                              kEncryptionServiceFailureWithPartialData) {
    LOG(ERROR) << "Failed to read logins for password import, result="
               << static_cast<int>(retrieval_result);
    read_result.credentials.clear();
    read_result.result = BravePasswordImporter::Result::kReadFailed;
    return read_result;
  }

  // PasswordStore::AddLogins CHECKs that blocklisted entries carry no username
  // or password. A source `Login Data` may violate that, so normalize here.
  for (auto& credential : read_result.credentials) {
    if (credential.blocked_by_user) {
      credential.username_value.clear();
      credential.password_value.clear();
    }
  }

  read_result.result = BravePasswordImporter::Result::kSuccess;
  return read_result;
}

BravePasswordImporter::BravePasswordImporter() = default;
BravePasswordImporter::~BravePasswordImporter() = default;

void BravePasswordImporter::StartImport(const base::FilePath& source_path,
                                        Profile* destination_profile,
                                        CompletionCallback callback) {
  CHECK(destination_profile);
  CHECK(callback);
  source_path_ = source_path;
  callback_ = std::move(callback);
  password_store_ = ProfilePasswordStoreFactory::GetForProfile(
      destination_profile, ServiceAccessType::EXPLICIT_ACCESS);
  if (!password_store_) {
    RunCallbackAsync(Result::kNoPasswordStore, 0);
    return;
  }

  auto* os_crypt = g_browser_process->os_crypt_async();
  if (!os_crypt) {
    RunCallbackAsync(Result::kNoEncryptor, 0);
    return;
  }
  os_crypt->GetInstance(base::BindOnce(&BravePasswordImporter::OnEncryptorReady,
                                       weak_factory_.GetWeakPtr()));
}

void BravePasswordImporter::RunCallbackAsync(Result result, size_t submitted) {
  // Always complete asynchronously so callers never see the callback run
  // within StartImport()'s stack frame.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback_), result, submitted));
}

void BravePasswordImporter::OnEncryptorReady(
    scoped_refptr<os_crypt_async::Encryptor> encryptor) {
  base::FilePath login_data_path = source_path_.Append(
      base::FilePath::StringType(FILE_PATH_LITERAL("Login Data")));
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&BravePasswordImporter::ReadCredentialsFromLoginData,
                     login_data_path, std::move(encryptor)),
      base::BindOnce(&BravePasswordImporter::OnCredentialsRead,
                     weak_factory_.GetWeakPtr()));
}

void BravePasswordImporter::OnCredentialsRead(ReadResult read_result) {
  if (read_result.result != Result::kSuccess || !password_store_) {
    std::move(callback_).Run(read_result.result, 0);
    return;
  }
  if (read_result.credentials.empty()) {
    std::move(callback_).Run(Result::kSuccess, 0);
    return;
  }
  const size_t count = read_result.credentials.size();
  password_store_->AddLogins(
      std::move(read_result.credentials),
      base::BindOnce(std::move(callback_), Result::kSuccess, count));
}
