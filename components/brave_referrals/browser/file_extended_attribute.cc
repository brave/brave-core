/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_referrals/browser/file_extended_attribute.h"

#include <sys/xattr.h>

#include "base/logging.h"

namespace brave {

int GetFileExtendedAttribute(const base::FilePath& path,
                             const char* name,
                             std::vector<char>* value) {
  // Pass nullptr for value to retrieve the length needed.
  ssize_t expected_length =
      getxattr(path.value().c_str(), name, /*value=*/nullptr,
               /*size =*/0, /*position=*/0, /*options=*/0);
  if (expected_length < 0) {
    return errno;
  }

  value->resize(expected_length);
  ssize_t length = getxattr(path.value().c_str(), name, value->data(),
                            expected_length, /*position=*/0,
                            /*options=*/0);
  if (length < 0) {
    return errno;
  } else if (length != expected_length) {
    VLOG(0) << "Failed to retrieve extended attribute " << name << " from file "
            << path << ". The expected data length (" << expected_length
            << ") and actual data length (" << length << ") do not match.";
    return ENOTRECOVERABLE;
  }

  return 0;
}

}  // namespace brave
