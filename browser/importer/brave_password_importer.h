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
struct PasswordForm;
class PasswordStoreInterface;
}  // namespace password_manager

// Reads encrypted credentials from a source Brave profile's `Login Data`
// SQLite database and writes them to the destination profile's password
// store. The destination's OSCryptAsync instance is used to decrypt the
// source's data, which works for Brave-to-Brave imports because both
// installations share the same OS keychain entry / DPAPI scope.
class BravePasswordImporter {
 public:
  // Callback fired on the UI thread with the number of credentials added.
  // A value of 0 indicates failure or that no credentials were found.
  using CompletionCallback = base::OnceCallback<void(size_t added)>;

  BravePasswordImporter();
  ~BravePasswordImporter();

  BravePasswordImporter(const BravePasswordImporter&) = delete;
  BravePasswordImporter& operator=(const BravePasswordImporter&) = delete;

  // `source_path` is the source Brave profile directory (containing the
  // `Login Data` file). `destination_profile` receives the imported
  // credentials.
  void Start(const base::FilePath& source_path,
             Profile* destination_profile,
             CompletionCallback callback);

 private:
  void OnEncryptorReady(os_crypt_async::Encryptor encryptor);
  void OnFormsRead(std::vector<password_manager::PasswordForm> forms);

  base::FilePath source_path_;
  scoped_refptr<password_manager::PasswordStoreInterface> password_store_;
  CompletionCallback callback_;
  base::WeakPtrFactory<BravePasswordImporter> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_IMPORTER_BRAVE_PASSWORD_IMPORTER_H_
