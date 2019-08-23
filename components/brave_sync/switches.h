/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_SWITCHES_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_SWITCHES_H_

namespace brave_sync {
namespace switches {

// This function can be called from any thread, and the implementation doesn't
// assume it's running on the UI thread.
bool IsBraveSyncAllowedByFlag();

extern const char kDisableBraveSync[];

}  // namespace switches
}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_SWITCHES_H_
