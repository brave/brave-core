// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/os_crypt/sync/os_crypt_metrics.h"

#include "base/metrics/histogram_functions.h"

namespace os_crypt {

void LogEncryptionVersion(EncryptionPrefixVersion version) {
  base::UmaHistogramEnumeration("OSCrypt.EncryptionPrefixVersion", version);
}

}  // namespace os_crypt
