/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_FEATURES_H_

#include "base/feature_list.h"

namespace brave_sync {
namespace features {

BASE_DECLARE_FEATURE(kBraveSync);
BASE_DECLARE_FEATURE(kBraveSyncDefaultPasswords);
// Allows the user to set a Brave-specific display label for any device in the
// sync chain. The label travels through `BraveSpecificFields` and is always
// synced; this flag only gates exposing the rename UI.
BASE_DECLARE_FEATURE(kBraveSyncAllowRenameDeviceLabel);

}  // namespace features
}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_FEATURES_H_
