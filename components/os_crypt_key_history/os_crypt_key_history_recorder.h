/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OS_CRYPT_KEY_HISTORY_OS_CRYPT_KEY_HISTORY_RECORDER_H_
#define BRAVE_COMPONENTS_OS_CRYPT_KEY_HISTORY_OS_CRYPT_KEY_HISTORY_RECORDER_H_

#include <map>
#include <string>
#include <string_view>

#include "base/files/file_path.h"
#include "base/functional/callback.h"

namespace brave::os_crypt_key_history_recorder {

// Writes the encryption key currently in use into the key history, but only
// once something has been seen decrypting data that was already on disk.
//
// The timing is the whole point. A key that merely unwraps proves nothing: a
// key that has replaced the right one unwraps perfectly well, it just does not
// match anything the user saved. Only a successful decrypt of stored data
// shows that the key still matches. See ../README.md.
//
// The browser layer supplies the keys and the means to compare them, both of
// which are platform specific. Callers that decrypt need to know none of that.

// Whether two wrapped keys hold the same key material. Wrapping is not
// deterministic, so their bytes cannot be compared directly.
using SameKeyComparator =
    base::RepeatingCallback<bool(std::string_view current,
                                 std::string_view stored)>;

// Called once from the browser layer during startup, before anything can
// decrypt. `current_keys` maps a provider name to the wrapped key that
// `Local State` holds for it right now.
void Initialize(base::FilePath history_path,
                std::map<std::string, std::string> current_keys,
                SameKeyComparator same_key);

// Called by anything that has just decrypted stored user data. Safe to call
// from any sequence and any number of times; only the first call in a session
// does any work, and the work happens off the calling sequence.
void NotifyDecryptedStoredData();

void ResetForTesting();

}  // namespace brave::os_crypt_key_history_recorder

#endif  // BRAVE_COMPONENTS_OS_CRYPT_KEY_HISTORY_OS_CRYPT_KEY_HISTORY_RECORDER_H_
