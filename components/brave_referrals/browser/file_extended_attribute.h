/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REFERRALS_BROWSER_FILE_EXTENDED_ATTRIBUTE_H_
#define BRAVE_COMPONENTS_BRAVE_REFERRALS_BROWSER_FILE_EXTENDED_ATTRIBUTE_H_

#include <vector>

#include "base/files/file_path.h"

namespace brave {

// Extracts the value of an extended file attribute specified by |name| from the
// file specified by the |path|. The result is placed in the |value|. On
// success returns 0. Otherwise, returns ERRNO value. Error values of
// interest:
// ENOATTR - the attribute with the given |name| was not found.
// ENOTSUP - the file system doesn't support (or disabled) extended attributes.
// ENOTRECOVERABLE - the value could not be retrieved.
int GetFileExtendedAttribute(const base::FilePath& path,
                             const char* name,
                             std::vector<char>* value);

}  // namespace brave

#endif  // BRAVE_COMPONENTS_BRAVE_REFERRALS_BROWSER_FILE_EXTENDED_ATTRIBUTE_H_
