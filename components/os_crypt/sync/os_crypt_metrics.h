// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_OS_CRYPT_SYNC_OS_CRYPT_METRICS_H_
#define BRAVE_COMPONENTS_OS_CRYPT_SYNC_OS_CRYPT_METRICS_H_

namespace os_crypt {

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class EncryptionPrefixVersion {
  // Data did not have any version header and so was returned to the caller
  // without being decrypted.
  kNoVersion = 0,
  // Data starts with `v10`. This is supported on all platforms.
  kVersion10 = 1,
  // Data starts with `v11`. This is only supported on Linux.
  kVersion11 = 2,
  kMaxValue = kVersion11,
};

void LogEncryptionVersion(EncryptionPrefixVersion version);

}  // namespace os_crypt

#endif  // BRAVE_COMPONENTS_OS_CRYPT_SYNC_OS_CRYPT_METRICS_H_
