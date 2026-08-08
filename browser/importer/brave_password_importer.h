/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_BRAVE_PASSWORD_IMPORTER_H_
#define BRAVE_BROWSER_IMPORTER_BRAVE_PASSWORD_IMPORTER_H_

#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"

class Profile;

namespace os_crypt_async {
class Encryptor;
}  // namespace os_crypt_async

namespace password_manager {
struct StoredCredential;
class PasswordStoreInterface;
}  // namespace password_manager

// Reads encrypted credentials from a source Brave profile's `Login Data`
// SQLite database and writes them to the destination profile's password
// store. The destination's OSCryptAsync instance is used to decrypt the
// source's data, which works for Brave-to-Brave imports on macOS and Linux
// because both installations share the same OS keychain entry. This is not
// supported on Windows, where each installation stores its own randomly
// generated key (DPAPI-encrypted) in its own Local State, so the destination
// key cannot decrypt the source's data.
class BravePasswordImporter {
 public:
  // Outcome of an import attempt, reported so callers can log the exact cause
  // rather than guessing from an empty result.
  enum class Result {
    kSuccess,
    kNoPasswordStore,
    kNoEncryptor,
    kLoginDataNotFound,
    kCopyFailed,
    kDatabaseInitFailed,
    kReadFailed,
  };

  // Callback fired on the UI thread with the outcome and the number of
  // credentials submitted to the password store (0 unless `result` is
  // `kSuccess`). This is the count handed to `AddLogins`; the store may reject
  // individual entries (e.g. duplicates), so it is an upper bound on the
  // number actually persisted, not an exact added count.
  using CompletionCallback =
      base::OnceCallback<void(Result result, size_t submitted)>;

  BravePasswordImporter();
  ~BravePasswordImporter();

  BravePasswordImporter(const BravePasswordImporter&) = delete;
  BravePasswordImporter& operator=(const BravePasswordImporter&) = delete;

  // `source_path` is the source Brave profile directory (containing the
  // `Login Data` file). `destination_profile` receives the imported
  // credentials.
  void StartImport(const base::FilePath& source_path,
                   Profile* destination_profile,
                   CompletionCallback callback);

 private:
  // Outcome of the background read of the source `Login Data`.
  struct ReadResult {
    ReadResult();
    ReadResult(ReadResult&&);
    ReadResult& operator=(ReadResult&&);
    ~ReadResult();

    Result result = Result::kReadFailed;
    std::vector<password_manager::StoredCredential> credentials;
  };

  // Runs on a background thread. Copies the source `Login Data`, opens it with
  // the destination's `encryptor`, and returns the outcome and credentials.
  static ReadResult ReadCredentialsFromLoginData(
      base::FilePath login_data_path,
      scoped_refptr<os_crypt_async::Encryptor> encryptor);

  // Posts `callback_` to the current sequence so early-failure paths complete
  // asynchronously, matching the async success path.
  void RunCallbackAsync(Result result, size_t submitted);

  void OnEncryptorReady(scoped_refptr<os_crypt_async::Encryptor> encryptor);
  void OnCredentialsRead(ReadResult read_result);

  base::FilePath source_path_;
  scoped_refptr<password_manager::PasswordStoreInterface> password_store_;
  CompletionCallback callback_;
  base::WeakPtrFactory<BravePasswordImporter> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_IMPORTER_BRAVE_PASSWORD_IMPORTER_H_
