/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/features.h"

#include "base/feature_list.h"

namespace brave_sync::features {

BASE_FEATURE(kBraveSync, "BraveSync", base::FEATURE_ENABLED_BY_DEFAULT);
BASE_FEATURE(kBraveSyncDefaultPasswords,
             "BraveSyncDefaultPasswords",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace brave_sync::features
