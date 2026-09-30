/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_OS_CRYPT_OS_CRYPT_KEY_BACKUP_COM_BRIDGE_H_
#define BRAVE_BROWSER_OS_CRYPT_OS_CRYPT_KEY_BACKUP_COM_BRIDGE_H_

#include <optional>
#include <string_view>
#include <vector>

#include "base/containers/span.h"

namespace brave {

// Unwraps `wrapped_key` (the bytes stored in the `os_crypt.encrypted_key`
// pref, base64-decoded, header and all) via DPAPI's `CryptUnprotectData`.
// Validates and strips the "DPAPI" header first, returning nullopt if it's
// missing. DPAPI has no COM/apartment requirement, so this is safe to call
// on any thread. Duplicated from the anonymous-namespace
// `DecryptKeyWithDPAPI` in
// components/os_crypt/async/browser/dpapi_key_provider.cc, which has no
// public API of its own - see os_crypt_key_backup_com_bridge.cc for why this
// is duplicated rather than exposed via a patch.
std::optional<std::vector<uint8_t>> UnwrapWithDPAPI(
    base::span<const uint8_t> wrapped_key);

// Unwraps `wrapped_key` (the bytes stored in the
// `os_crypt.app_bound_encrypted_key` pref, base64-decoded, header and all)
// by blocking on a round trip to the elevation service. Validates and strips
// the "APPB" header first, returning nullopt if it's missing. That call must
// run on a COM Single-Threaded Apartment, so this dispatches to a dedicated
// COM-STA background thread and blocks the calling sequence until it
// completes - there is no result to return to otherwise, since this is
// called both from a background task (the write path) and from
// `PreCreateMainMessageLoop`, before any message loop or
// `SequencedTaskRunner::CurrentDefaultHandle` exists to bridge asynchronously
// (the restore path).
//
// `step_description` is logged via LOG(INFO) before dispatching and after
// completion, so a slow elevation-service round trip during startup is
// explained in the logs rather than appearing as unexplained delay.
//
// Bounded by a timeout: if the elevation service does not respond in time,
// returns nullopt rather than blocking startup indefinitely.
std::optional<std::vector<uint8_t>> BlockingUnwrapAppBoundKeyOnCOMSTA(
    base::span<const uint8_t> wrapped_key,
    std::string_view step_description);

}  // namespace brave

#endif  // BRAVE_BROWSER_OS_CRYPT_OS_CRYPT_KEY_BACKUP_COM_BRIDGE_H_
