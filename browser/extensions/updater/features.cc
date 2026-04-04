// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/extensions/updater/features.h"

namespace brave_extensions::features {

BASE_FEATURE(kBraveExtensionAutoUpdate,
             "BraveExtensionAutoUpdate",
             base::FEATURE_ENABLED_BY_DEFAULT);

}  // namespace brave_extensions::features
