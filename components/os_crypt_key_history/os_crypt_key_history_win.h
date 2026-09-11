/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OS_CRYPT_KEY_HISTORY_OS_CRYPT_KEY_HISTORY_WIN_H_
#define BRAVE_COMPONENTS_OS_CRYPT_KEY_HISTORY_OS_CRYPT_KEY_HISTORY_WIN_H_

#include "base/files/file_path.h"

class PrefService;

namespace brave {

// Name of the key history file, kept beside `Local State` in the user data
// directory so that it outlives `Local State` being replaced.
inline constexpr base::FilePath::CharType kOSCryptKeyHistoryFileName[] =
    FILE_PATH_LITERAL("OSCrypt Key History");

// Points the key history recorder at this installation's DPAPI key. Called
// once during startup, after OSCrypt has been initialized.
void InitializeOSCryptKeyHistoryRecorder(const base::FilePath& user_data_dir,
                                         PrefService* local_state);

}  // namespace brave

#endif  // BRAVE_COMPONENTS_OS_CRYPT_KEY_HISTORY_OS_CRYPT_KEY_HISTORY_WIN_H_
